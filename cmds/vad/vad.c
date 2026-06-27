/*
 * Copyright (c) 2026 Realtek, LLC.
 * All rights reserved.
 *
 * Licensed under the Realtek License, Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License from Realtek
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file vad.c
 * @brief CA32 (AP) command that drives the VAD wake-up -> AFE+KWS -> ASR pipeline.
 *
 * This file is a thin demo on top of the AudioVad framework
 * (component/audio/interfaces/audio/audio_vad.h). All HW-VAD interaction —
 * codec / AMIC bring-up, IRQ + wake semaphore, SRAM ring readback, re-arm —
 * lives in the framework + HAL; the demo only orchestrates aivoice and the
 * AudioRecord stage.
 *
 * Two-stage flow after a hardware VAD wake-up:
 *
 *   Stage 1  (AudioVad lookback buffer  -> aivoice AFE + KWS)
 *     During CG4 sleep AudioRecord/SPORT/GDMA are powered down. The HW VAD
 *     keeps writing 16 kHz / 16-bit samples into its SRAM ring while the AP
 *     sleeps. The microphone count follows the menuconfig AFE resource
 *     (VAD_AFE_MIC_NUM = 1/2/3); a multi-mic AFE records the mics interleaved.
 *     We pull one 16 ms AFE frame at a time (512 B per mic = VAD_AFE_FEED_BYTES)
 *     out of the ring via AudioVad_ReadLookback() (which delegates to fwlib's
 *     get_vad_data — first call locks ~400 ms pre-roll from hit_addr, then
 *     walks forward) and feed each interleaved frame straight to aivoice. The loop runs
 *     up to `stage1_frames` (`-n`/`--frames`) times; on hit aivoice raises
 *     AIVOICE_EVOUT_WAKEUP and Stage 2 starts immediately. If the budget is
 *     exhausted with no hit, we treat the wake-up as a false positive,
 *     skip Stage 2 and re-arm.
 *
 *   Stage 2  (AudioRecord -> aivoice ASR)
 *     AudioVad_PrepareForRecord() switches the ADC clock to the audio codec
 *     path, then AudioRecord brings up SPORT/GDMA. We keep feeding the same
 *     aivoice instance (already in its ASR window) until ASR_RESULT or
 *     timeout. Then AudioRecord is destroyed and AudioVad_Rearm() restores
 *     the codec / AMIC / VAD ADC routing for the next sleep cycle.
 *
 * Reference test sequence:
 *   AT+TICKPS=TYPE,CG       (only CG4 is supported)
 *   vad                     (this command)
 *   AT+TICKPS=R             (enter sleep, wait for wakeup)
 */

#define TAG "Vad"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "os_wrapper.h"

#include "audio/audio_vad.h"
#include "audio/audio_record.h"
#include "audio/audio_service.h"
#include "audio/audio_type.h"
#include "common/audio_errnos.h"

#include "audio_cmd_common.h"

#if defined(CONFIG_AIVOICE_EN)
#include "aivoice_interface.h"
#endif

/*============================================================================
 * Protocol-mandated constants
 *============================================================================*/

/* aivoice AFE frame on RTL8730E CA32: 256 samples * 2 bytes = 512 B = 16 ms
 * PER MICROPHONE (frame_size=256). Dictated by aivoice, not a test value. */
#define VAD_AFE_FRAME_BYTES         512U
#define VAD_AFE_FRAME_MS            16U

/* The microphone-array geometry is fixed at build time by menuconfig
 * (AI Config -> Select AFE Resource): afe_res_1mic / 2mic{30,50,70}mm / 3mic50mm.
 * The VAD SRAM lookback channel count, the Stage 2 AudioRecord channel count and
 * the aivoice AFE config MUST all follow that single choice — the customer does
 * NOT pick channels at runtime. A 2-mic / 3-mic AFE consumes its microphones
 * INTERLEAVED, so the VAD ring records N channels and we feed the whole
 * interleaved frame straight to aivoice (no de-interleaving / channel pick). */
#if defined(CONFIG_AFE_RES_CIRCLE3MIC50MM)
#  define VAD_AFE_MIC_NUM           3U
#elif defined(CONFIG_AFE_RES_2MIC30MM) || defined(CONFIG_AFE_RES_2MIC50MM) || defined(CONFIG_AFE_RES_2MIC70MM)
#  define VAD_AFE_MIC_NUM           2U
#else  /* CONFIG_AFE_RES_1MIC, or aivoice disabled */
#  define VAD_AFE_MIC_NUM           1U
#endif

/* One full interleaved AFE feed unit = per-mic quantum * mic count. */
#define VAD_AFE_FEED_BYTES          (VAD_AFE_FRAME_BYTES * VAD_AFE_MIC_NUM)

/* Default Stage 1 budget: 100 AFE frames. The first frame
 * picks up ~400 ms of pre-roll relative to hit_addr (fwlib PRE_READ_NUM_BLOCK),
 * so the effective audio window is roughly 400 ms historical + N*16 ms going
 * forward. */
#define VAD_DEFAULT_STAGE1_FRAMES   100

/*============================================================================
 * Type Definitions
 *============================================================================*/

typedef struct {
    int         rate;           /* recording sample rate (Hz)                  */
    int         bits;           /* recording bits per sample (16/24/32)        */
    int         duration;       /* upper bound recording seconds after wake-up */
    int         mic;            /* DEVICE_IN_MIC / DMIC_REF_AMIC / I2S         */
    int         amic_index;     /* AMIC1=1..AMIC4=4 (0 = HAL default = AMIC1)  */
    int         dmic_index;     /* DMIC_ZERO=0, 0-based                        */
    int         stage1_frames;  /* Stage 1 max # of 16 ms AFE frames to feed   */
    const char *kws_keyword;    /* aivoice KWS keyword string                  */
    int         verbose;
    /* NOTE: the channel count is intentionally NOT a field — it is fixed by
     * the menuconfig AFE resource (VAD_AFE_MIC_NUM) and cannot be overridden
     * at runtime, so VAD lookback and AudioRecord always match the AFE. */
} vad_params_t;

static const vad_params_t VAD_DEFAULT_PARAMS = {
    .rate          = 16000,
    .bits          = 16,
    .duration      = 8,
    .mic           = DEVICE_IN_MIC,
    .amic_index    = 1,
    .dmic_index    = 0,
    .stage1_frames = VAD_DEFAULT_STAGE1_FRAMES,
    .kws_keyword   = "ni-hao-xiao-qiang",
    .verbose       = 0,
};

/*============================================================================
 * Private State
 *============================================================================*/

static rtos_sema_t      s_listener_sema = NULL;  /* keeps vad cmd from re-entering */
static vad_params_t     s_vad_params;

#if defined(CONFIG_AIVOICE_EN)
static void               *s_aivoice_handle    = NULL;
static volatile int        s_kws_woke          = 0;
static volatile int        s_asr_session_done  = 0;
#endif

/*============================================================================
 * AIVoice Layer  (AFE + KWS + VAD + ASR via aivoice_iface_full_flow_v1)
 *============================================================================*/

#if defined(CONFIG_AIVOICE_EN)

static int vad_aivoice_callback(void *userdata,
                                enum aivoice_out_event_type event_type,
                                const void *msg, int len)
{
    (void)userdata;
    struct aivoice_evout_vad *vad_out;

    switch (event_type) {
    case AIVOICE_EVOUT_VAD:
        vad_out = (struct aivoice_evout_vad *)msg;
        RTK_LOGS(TAG, RTK_LOG_ALWAYS, "[aivoice] vad status=%d offset=%dms\n",
                 vad_out->status, vad_out->offset_ms);
        break;

    case AIVOICE_EVOUT_WAKEUP:
        s_kws_woke = 1;
        RTK_LOGS(TAG, RTK_LOG_ALWAYS, "[aivoice] wakeup: %.*s\n", len, (const char *)msg);
        break;

    case AIVOICE_EVOUT_ASR_RESULT:
        s_asr_session_done = 1;
        RTK_LOGS(TAG, RTK_LOG_ALWAYS, "[aivoice] asr result: %.*s\n", len, (const char *)msg);
        break;

    case AIVOICE_EVOUT_ASR_REC_TIMEOUT:
        s_asr_session_done = 1;
        RTK_LOGS(TAG, RTK_LOG_ALWAYS, "[aivoice] asr timeout\n");
        break;

    case AIVOICE_EVOUT_AFE:
        break;

    default:
        break;
    }
    return 0;
}

static int vad_aivoice_init(const char *kws_keyword)
{
    static struct afe_config afe_param;
    static struct kws_config kws_param;
    struct aivoice_config config;

    /* Pick the AFE config that matches the menuconfig AFE resource. mic_array
     * MUST match the linked resource library, so this selection is compile-time
     * (CONFIG_AFE_RES_*), never a runtime knob. */
#if defined(CONFIG_AFE_RES_CIRCLE3MIC50MM)
    afe_param = (struct afe_config)AFE_CONFIG_ASR_DEFAULT_3MIC();
#elif defined(CONFIG_AFE_RES_2MIC30MM)
    afe_param = (struct afe_config)AFE_CONFIG_ASR_DEFAULT_2MIC30MM();
#elif defined(CONFIG_AFE_RES_2MIC50MM)
    afe_param = (struct afe_config)AFE_CONFIG_ASR_DEFAULT_2MIC50MM();
#elif defined(CONFIG_AFE_RES_2MIC70MM)
    afe_param = (struct afe_config)AFE_CONFIG_ASR_DEFAULT_2MIC70MM();
#else  /* CONFIG_AFE_RES_1MIC (default) */
    afe_param = (struct afe_config)AFE_CONFIG_ASR_DEFAULT_1MIC();
#endif
    /* VAD has no playback reference, so AEC stays off regardless of mic count;
     * the AFE input is then exactly VAD_AFE_MIC_NUM interleaved channels. */
    afe_param.ref_num    = 0;
    afe_param.enable_aec = false;

    kws_param = (struct kws_config)KWS_CONFIG_DEFAULT();
    memset(kws_param.keywords, 0, sizeof(kws_param.keywords));
    kws_param.keywords[0]   = kws_keyword;
    kws_param.thresholds[0] = 0.0f;
    kws_param.sensitivity   = KWS_SENSITIVITY_MID;
    kws_param.mode          = KWS_SINGLE_MODE;

    memset(&config, 0, sizeof(config));
    config.afe = &afe_param;
    config.kws = &kws_param;

    s_aivoice_handle = aivoice_iface_full_flow_v1.create(&config);
    if (!s_aivoice_handle) {
        RTK_LOGS(TAG, RTK_LOG_ALWAYS, "aivoice create failed\n");
        return -1;
    }
    rtk_aivoice_register_callback(s_aivoice_handle, vad_aivoice_callback, NULL);
    RTK_LOGS(TAG, RTK_LOG_ALWAYS, "aivoice ready, kws=[\"%s\"] afe_mics=%u\n",
             kws_keyword, (unsigned)VAD_AFE_MIC_NUM);
    return 0;
}

static void vad_aivoice_deinit(void)
{
    if (s_aivoice_handle) {
        aivoice_iface_full_flow_v1.destroy(s_aivoice_handle);
        s_aivoice_handle = NULL;
    }
}

static void vad_aivoice_reset_session(void)
{
    s_kws_woke         = 0;
    s_asr_session_done = 0;
    if (s_aivoice_handle) {
        aivoice_iface_full_flow_v1.reset(s_aivoice_handle);
    }
}

static int vad_aivoice_feed(const uint8_t *buf, size_t bytes)
{
    if (!s_aivoice_handle) {
        return -1;
    }
    /* aivoice expects one full interleaved AFE frame per feed:
     * VAD_AFE_MIC_NUM * 256 samples * 2 bytes. */
    size_t off = 0;
    while (off + VAD_AFE_FEED_BYTES <= bytes) {
        int ret = aivoice_iface_full_flow_v1.feed(s_aivoice_handle,
                                                  (char *)(buf + off),
                                                  VAD_AFE_FEED_BYTES);
        if (ret != 0) {
            return ret;
        }
        off += VAD_AFE_FEED_BYTES;
    }
    return 0;
}

#else  /* !CONFIG_AIVOICE_EN — keep file building without aivoice */

static int  vad_aivoice_init(const char *kws_keyword) { (void)kws_keyword; RTK_LOGS(TAG, RTK_LOG_ALWAYS, "aivoice disabled at build time\n"); return 0; }
static void vad_aivoice_deinit(void) { }
static void vad_aivoice_reset_session(void) { }
static int  vad_aivoice_feed(const uint8_t *buf, size_t bytes) { (void)buf; (void)bytes; return 0; }
static volatile int s_asr_session_done = 0;
static volatile int s_kws_woke         = 0;

#endif  /* CONFIG_AIVOICE_EN */

/*============================================================================
 * Helpers
 *============================================================================*/

static uint32_t vad_format_for_bits(int bits)
{
    switch (bits) {
    case 16: return AUDIO_FORMAT_PCM_16_BIT;
    case 24: return AUDIO_FORMAT_PCM_24_BIT;
    case 32: return AUDIO_FORMAT_PCM_32_BIT;
    default: return AUDIO_FORMAT_INVALID;
    }
}

/*============================================================================
 * Stage 1 — pull AFE frames from VAD ring, feed aivoice (AFE + KWS)
 *============================================================================*/

/* One AFE frame at a time. The framework's ReadLookback delegates to fwlib's
 * get_vad_data — first call after each wake locks ~400 ms of historical
 * pre-roll relative to hit_addr; subsequent calls walk forward and busy-wait
 * for live ADC data. We exit the moment KWS fires or the configured frame
 * budget is exhausted.
 *
 * Channel count follows the menuconfig AFE resource (VAD_AFE_MIC_NUM). When the
 * AFE is multi-mic the VAD ring records the mics INTERLEAVED, and ReadLookback
 * returns one 16 ms window as VAD_AFE_FEED_BYTES (= 512 * mic_num). That whole
 * interleaved frame is fed straight to aivoice, whose multi-mic AFE consumes
 * the channels directly — there is no de-interleaving or channel selection. */
static int vad_stage1_kws_from_lookback(const vad_params_t *p)
{
    uint8_t  frame_buf[VAD_AFE_FEED_BYTES];
    int      hit_kws = 0;
    int      fed_frames = 0;

    int max_frames = p->stage1_frames > 0 ? p->stage1_frames
                                          : VAD_DEFAULT_STAGE1_FRAMES;

    for (fed_frames = 0; fed_frames < max_frames; fed_frames++) {
        int32_t got = AudioVad_ReadLookback(frame_buf, VAD_AFE_FEED_BYTES);
        if (got <= 0) {
            RTK_LOGS(TAG, RTK_LOG_ALWAYS,
                     "stage1: AudioVad_ReadLookback returned %ld at frame %d\n",
                     (long)got, fed_frames);
            break;
        }
        if (vad_aivoice_feed(frame_buf, (size_t)got) != 0) {
            RTK_LOGS(TAG, RTK_LOG_ALWAYS, "stage1 aivoice feed failed at frame %d\n",
                     fed_frames);
            break;
        }
        if (s_kws_woke) {
            hit_kws = 1;
            fed_frames++;
            break;
        }
    }

    RTK_LOGI(TAG, "stage1: mics=%u fed=%d/%d frames (~%ums) hit_addr=0x%08lx kws_hit=%d\n",
                  (unsigned)VAD_AFE_MIC_NUM, fed_frames, max_frames,
                  (unsigned)(fed_frames * VAD_AFE_FRAME_MS),
                  (unsigned long)AudioVad_GetHitAddress(), hit_kws);
    return hit_kws;
}

/*============================================================================
 * Stage 2 — open AudioRecord and feed aivoice (ASR window)
 *============================================================================*/

static int vad_stage2_asr_from_audiorecord(const vad_params_t *p)
{
    struct AudioRecord *record = AudioRecord_Create();
    if (!record) {
        RTK_LOGS(TAG, RTK_LOG_ALWAYS, "AudioRecord_Create failed\n");
        return -1;
    }

    /* Record the same number of channels the AFE expects (VAD_AFE_MIC_NUM):
     * Stage 2 feeds the very same aivoice instance, now in its ASR window, so
     * the interleaved frame layout must match what Stage 1 fed. */
    AudioRecordConfig cfg = {
        .sample_rate   = (uint32_t)p->rate,
        .channel_count = VAD_AFE_MIC_NUM,
        .format        = vad_format_for_bits(p->bits),
        .device        = (uint32_t)p->mic,
        .buffer_bytes  = 0,
    };
    if (AudioRecord_Init(record, &cfg, AUDIO_INPUT_FLAG_NONE) != AUDIO_OK) {
        RTK_LOGS(TAG, RTK_LOG_ALWAYS, "AudioRecord_Init failed\n");
        AudioRecord_Destroy(record);
        return -1;
    }
    AudioRecord_Start(record);

    uint32_t frame_bytes = VAD_AFE_MIC_NUM * (uint32_t)p->bits / 8U;
    uint64_t total_bytes = (uint64_t)p->rate * (uint64_t)p->duration * (uint64_t)frame_bytes;

    uint8_t *buf = (uint8_t *)malloc(VAD_AFE_FEED_BYTES);
    if (!buf) {
        RTK_LOGS(TAG, RTK_LOG_ALWAYS, "stage2 malloc failed\n");
        AudioRecord_Stop(record);
        AudioRecord_Destroy(record);
        return -1;
    }

    uint64_t bytes_read = 0;
    while (bytes_read < total_bytes) {
        int32_t n = AudioRecord_Read(record, buf, VAD_AFE_FEED_BYTES, true);
        if (n <= 0) {
            break;
        }
        if (vad_aivoice_feed(buf, (size_t)n) != 0) {
            RTK_LOGS(TAG, RTK_LOG_ALWAYS, "stage2 aivoice feed failed\n");
            break;
        }
        bytes_read += (uint64_t)n;
        if (s_asr_session_done) {
            break;
        }
    }

    free(buf);
    AudioRecord_Stop(record);
    AudioRecord_Destroy(record);

    RTK_LOGS(TAG, RTK_LOG_ALWAYS, "stage2 end: asr_done=%d fed=%u bytes\n",
             (int)s_asr_session_done, (unsigned)bytes_read);
    return 0;
}

/*============================================================================
 * Worker Task
 *============================================================================*/

static void example_vad_thread(void *param)
{
    vad_params_t *p = (vad_params_t *)param;

    RTK_LOGS(TAG, RTK_LOG_ALWAYS, "vad demo begin, free heap:%d\n",
             rtos_mem_get_free_heap_size());

    AudioService_Init();

    if (vad_aivoice_init(p->kws_keyword) != 0) {
        RTK_LOGS(TAG, RTK_LOG_ALWAYS, "aivoice init failed, abort\n");
        rtos_task_delete(NULL);
        return;
    }

    /* Bring up HW VAD via the framework (singleton). */
    AudioVadConfig vad_cfg = {
        .device        = (uint32_t)p->mic,
        .mic_index     = (p->mic == DEVICE_IN_DMIC_REF_AMIC) ? (uint32_t)p->dmic_index
                                                             : (uint32_t)p->amic_index,
        .channel_count = VAD_AFE_MIC_NUM,
        .det_mv_threshold = 10,
        .det_od_threshold = 15,
    };
    if (AudioVad_Init(&vad_cfg) != AUDIO_OK) {
        RTK_LOGS(TAG, RTK_LOG_ALWAYS, "AudioVad_Init failed\n");
        vad_aivoice_deinit();
        rtos_task_delete(NULL);
        return;
    }

    if (AudioVad_Start() != AUDIO_OK) {
        RTK_LOGS(TAG, RTK_LOG_ALWAYS, "AudioVad_Start failed\n");
        AudioVad_Deinit();
        vad_aivoice_deinit();
        rtos_task_delete(NULL);
        return;
    }

    RTK_LOGS(TAG, RTK_LOG_ALWAYS,
             "vad armed [%s mic_index=%u vad_channels=%u], run AT+TICKPS=R to enter sleep\n",
             (p->mic == DEVICE_IN_DMIC_REF_AMIC) ? "DMIC" : "AMIC",
             vad_cfg.mic_index, vad_cfg.channel_count);

    while (1) {
        if (AudioVad_WaitWakeup(0xFFFFFFFFU) != AUDIO_OK) {
            continue;
        }

        RTK_LOGI(TAG, "[CA32] woke up by VAD, hit_addr=0x%x\n",
                 (unsigned long)AudioVad_GetHitAddress());

        vad_aivoice_reset_session();

        /* No settle delay needed — get_vad_data busy-waits internally for any
         * live data the ADC has not finished pushing yet. */

        RTK_LOGS(TAG, RTK_LOG_ALWAYS, "[CA32] stage1 kws begin\n");
        /* ---------- Stage 1: lookback buffer -> aivoice AFE + KWS ---------- */
        int kws_hit = vad_stage1_kws_from_lookback(p);

        if (kws_hit) {
            /* ---------- Stage 2: AudioRecord -> aivoice ASR ---------- */
            AudioVad_PrepareForRecord();
            RTK_LOGS(TAG, RTK_LOG_ALWAYS, "[CA32] stage2 asr begin\n");
            vad_stage2_asr_from_audiorecord(p);
        } else {
            RTK_LOGS(TAG, RTK_LOG_ALWAYS,
                     "[CA32] no keyword in stage1, skip ASR and re-arm\n");
        }

        AudioVad_Rearm();
        RTK_LOGS(TAG, RTK_LOG_ALWAYS, "[CA32] vad re-armed, waiting for next utterance\n");
    }

    /* Unreached. */
    AudioVad_Stop();
    AudioVad_Deinit();
    vad_aivoice_deinit();
    rtos_task_delete(NULL);
}

/*============================================================================
 * Help / Params Parsing
 *============================================================================*/

static void vad_help(void)
{
    RTK_LOGS(TAG, RTK_LOG_ALWAYS, "vad [OPTION...]\n"
             "\t-h, --help              show this help message\n"
             "\t-r, --rate              record sample rate (default: 16000)\n"
             "\t-f, --format            record bits 16/24/32 (default: 16)\n"
             "\t-d, --duration          max recording seconds per wake-up (default: 8)\n"
             "\t-m, --mic               mic source: 0=AMIC, 1=DMIC_REF_AMIC, 2=I2S (default: 0)\n"
             "\t-n, --frames            stage1 max # of 16 ms AFE frames (default: 100 * 16 ms)\n"
             "\t    --amic-index        AMIC index 1..4 (default: 1)\n"
             "\t    --dmic-index        DMIC index, 0-based (default: 0)\n"
             "\t-V, --verbose           verbose log\n"
             "\n\tChannel count is NOT a CLI option: it follows the menuconfig AFE\n"
             "\tresource (afe_res_1mic/2mic/3mic). VAD lookback + AudioRecord both\n"
             "\ttrack the AFE mic count so the interleaved frames always match.\n"
             "\nFlow:\n"
             "\t1. AT+TICKPS=TYPE,CG    (only CG4 is supported)\n"
             "\t2. vad                  (init AudioVad and start listener)\n"
             "\t3. AT+TICKPS=R          (enter sleep, then say the wake word + command)\n"
             "\nExamples:\n"
             "\tvad\n"
             "\tvad -r 16000 -f 16 -d 8\n"
             "\tvad -n 96 -k ni-hao-xiao-qiang\n");
}

static void vad_parse_params(cmd_params_t *params, vad_params_t *p)
{
    *p = VAD_DEFAULT_PARAMS;

    cmd_parse_entry_t int_entries[] = {
        {"-r",            &p->rate,        VAD_DEFAULT_PARAMS.rate},
        {"--rate",        &p->rate,        VAD_DEFAULT_PARAMS.rate},
        {"-f",            &p->bits,        VAD_DEFAULT_PARAMS.bits},
        {"--format",      &p->bits,        VAD_DEFAULT_PARAMS.bits},
        {"-d",            &p->duration,      VAD_DEFAULT_PARAMS.duration},
        {"--duration",    &p->duration,      VAD_DEFAULT_PARAMS.duration},
        {"-n",            &p->stage1_frames, VAD_DEFAULT_PARAMS.stage1_frames},
        {"--frames",      &p->stage1_frames, VAD_DEFAULT_PARAMS.stage1_frames},
        {"--amic-index",  &p->amic_index,    VAD_DEFAULT_PARAMS.amic_index},
        {"--dmic-index",  &p->dmic_index,  VAD_DEFAULT_PARAMS.dmic_index},
        {"-V",            &p->verbose,     VAD_DEFAULT_PARAMS.verbose},
        {"--verbose",     &p->verbose,     VAD_DEFAULT_PARAMS.verbose},
    };
    cmd_parse_all_int(params, int_entries, sizeof(int_entries) / sizeof(int_entries[0]));

    int mic_val = 0;
    CMD_PARSE_INT(mic_val, "-m", 0);
    if (mic_val == 1) {
        p->mic = DEVICE_IN_DMIC_REF_AMIC;
    } else if (mic_val == 2) {
        p->mic = DEVICE_IN_I2S;
    } else {
        p->mic = DEVICE_IN_MIC;
    }
}

/*============================================================================
 * Command Entry
 *============================================================================*/

static uint32_t vad_handler(cmd_params_t *params)
{
    for (int i = 0; i < params->argc; i++) {
        if (strcmp(params->argv[i], "-h") == 0 || strcmp(params->argv[i], "--help") == 0) {
            vad_help();
            return TRUE;
        }
    }

    if (s_listener_sema != NULL) {
        RTK_LOGS(TAG, RTK_LOG_ALWAYS, "vad listener already running\n");
        return TRUE;
    }

    vad_parse_params(params, &s_vad_params);
    rtos_sema_create_binary(&s_listener_sema);

    if (rtos_task_create(NULL, "example_vad_thread", example_vad_thread,
                         &s_vad_params, 1024 * 16, 3) != RTK_SUCCESS) {
        RTK_LOGS(TAG, RTK_LOG_ALWAYS, "rtos_task_create(example_vad_thread) failed\n");
        rtos_sema_delete(s_listener_sema);
        s_listener_sema = NULL;
        return FALSE;
    }
    return TRUE;
}

DEFINE_CMD_WRAPPER(vad, vad_handler, 5);

CMD_TABLE_DATA_SECTION
const COMMAND_TABLE vad_cmd_table[] = {
    {"vad", vad_cmd_thread},
};
