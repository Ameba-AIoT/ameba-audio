/*
 * Copyright (c) 2021 Realtek, LLC.
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

#include "ameba_soc.h"
#include "audio/audio_control.h"
#include "audio/audio_record.h"
#include "audio/audio_service.h"
#include "audio/audio_track.h"
#include "common/audio_errnos.h"
#include "os_wrapper.h"
#include "platform_stdlib.h"
#include "basic_types.h"

#include "audio_cmd_common.h"

static const char *const TAG = "arecord";

enum {
    MAX_CHANNEL_COUNT = 8,
    DUMP_FRAME        = 96000,
};

enum {
    EQLPF = 1,
    EQHPF = 2,
    EQBPF = 3,
    EQLSF = 4,
    EQHSF = 5,
    EQNF = 6,
    EQPF = 7,
};

typedef struct {
    unsigned int rate;
    unsigned int channels;
    unsigned int format;
    unsigned int bytes_one_time;
    unsigned int mode;
    unsigned int only_record;
    unsigned int noirq_test;
    unsigned int test_ref;
    unsigned int pressure_test;
    unsigned int mic_category;
    unsigned int channel_src[MAX_CHANNEL_COUNT];
    unsigned int hpf_fc;
    unsigned int eq_filter_type;
    unsigned int record_seconds;
    unsigned int dump_buffer;
    unsigned int test_timestamp;
} arecord_params_t;

static const arecord_params_t ARECORD_DEFAULT_PARAMS = {
    .rate = 16000,
    .channels = 2,
    .format = 16,
    .bytes_one_time = 8192,
    .mode = 0,
    .only_record = 0,
    .noirq_test = 0,
    .test_ref = 0,
    .pressure_test = 0,
    .mic_category = DEVICE_IN_MIC,
    .channel_src = {AUDIO_AMIC1, AUDIO_AMIC2, AUDIO_AMIC3, 0, 0, 0, 0, 0},
    .hpf_fc = 3,
    .eq_filter_type = 0,
    .record_seconds = 600,
    .dump_buffer = 0,
    .test_timestamp = 0,
};

static void arecord_help(void);

static struct AudioRecord *g_arecord = NULL;
static unsigned int  g_only_record = 0;
static unsigned int  g_noirq_test = 0;
static unsigned int  g_pressure_test = 0;
static unsigned int  g_record_rate = 16000;
static unsigned int  g_record_channel = 2;
static unsigned int  g_record_mode = 0;
static unsigned int  g_record_format = 16;
static unsigned int  g_record_bytes_one_time = 8192;
static unsigned int  g_record_mic_category = DEVICE_IN_I2S;
static unsigned int  g_record_channel_src[MAX_CHANNEL_COUNT] = {AUDIO_AMIC1};
static unsigned int  g_hpf_fc = 3;
static unsigned int  g_eq_filter_type = 0;
static unsigned int  g_record_seconds = 600;
static unsigned int  g_dump_buffer = 0;
static unsigned int  g_test_timestamp = 0;
static unsigned int  record_sample(void);

static int GetFormatForBits(void)
{
    int format;
    switch (g_record_format) {
    case 16:
        format = AUDIO_FORMAT_PCM_16_BIT;
        break;
    case 24:
        format = AUDIO_FORMAT_PCM_24_BIT;
        break;
    case 32:
        format = AUDIO_FORMAT_PCM_32_BIT;
        break;
    default:
        RTK_LOGE(TAG, "format:%d not supportd \n", g_record_format);
        return AUDIO_FORMAT_INVALID;
    }

    return format;
}

static void FillEqFilterCoef(EqFilterCoef *coef) {
    if (g_eq_filter_type == EQLPF) {
        /* coefficient of low pass filter */
        coef->H0_Q = 0x0070BD5C;
        coef->B1_Q = 0x04000000;
        coef->B2_Q = 0x02000000;
        coef->A1_Q = 0x009D7957;
        coef->A2_Q = 0x1F9F9139;
    } else if (g_eq_filter_type == EQHPF) {
        /* coefficient of high pass filter */
        coef->H0_Q = 0x01FB4865;
        coef->B1_Q = 0x1C000000;
        coef->B2_Q = 0x02000000;
        coef->A1_Q = 0x03F685A9;
        coef->A2_Q = 0x1E096416;
    } else if (g_eq_filter_type == EQBPF) {
        /* coefficient of band pass filter */
        coef->H0_Q = 0x0035FA76;
        coef->B1_Q = 0x00000000;
        coef->B2_Q = 0x1E000000;
        coef->A1_Q = 0x0287BD8B;
        coef->A2_Q = 0x1E6BF4ED;
    } else if (g_eq_filter_type == EQLSF) {
        /* coefficient of low shelving filter */
        coef->H0_Q = 0x0218161A;
        coef->B1_Q = 0x1C5E3906;
        coef->B2_Q = 0x01A9BEB6;
        coef->A1_Q = 0x03D0A40E;
        coef->A2_Q = 0x1E2D437C;
    } else if (g_eq_filter_type == EQHSF) {
        /* coefficient of high shelving filter */
        coef->H0_Q = 0x03C8147B;
        coef->B1_Q = 0x1C85F1AF;
        coef->B2_Q = 0x0189A7A4;
        coef->A1_Q = 0x034417AE;
        coef->A2_Q = 0x1E9E69D1;
    } else if (g_eq_filter_type == EQNF) {
        /* coefficient of notch filter */
        coef->H0_Q = 0x01E0E26E;
        coef->B1_Q = 0x1C5F00A5;
        coef->B2_Q = 0x02000000;
        coef->A1_Q = 0x03A0FF5B;
        coef->A2_Q = 0x1E3E3B25;
    } else if (g_eq_filter_type == EQPF) {
        /* coefficient of peak filter */
        coef->H0_Q = 0x01ED0FD0;
        coef->B1_Q = 0x1C15D75D;
        coef->B2_Q = 0x01F2CE6C;
        coef->A1_Q = 0x03C51715;
        coef->A2_Q = 0x1E329506;
    }
}

static unsigned int record_sample()
{
    char *buffer;
    char *dump_buffer = NULL;
    unsigned int size;
    ssize_t size_read;
    int dumped_size = 0;
    uint32_t flags = AUDIO_OUTPUT_FLAG_NONE;
    uint32_t record_flags = AUDIO_INPUT_FLAG_NONE;
    int64_t bytes_read = 0;
    unsigned int frames = 0;
    int format = GetFormatForBits();
    int64_t record_size = (int64_t)g_record_rate * (int64_t)g_record_seconds * (int64_t)g_record_channel * (int64_t)g_record_format / (int64_t)8;
    int track_buf_size = 4096;

    if (format == (int)AUDIO_FORMAT_INVALID) {
        RTK_LOGE(TAG, "invalid record format bits:%u \n", g_record_format);
        return 0;
    }

    if (g_noirq_test) {
        flags |= AUDIO_OUTPUT_FLAG_NOIRQ;
        record_flags |= AUDIO_INPUT_FLAG_NOIRQ;
    }

    struct AudioRecord *arecord;
    arecord = AudioRecord_Create();
    if (!arecord) {
        RTK_LOGE(TAG, "record create failed \n");
        return 0;
    }

    AudioRecordConfig record_config;
    record_config.sample_rate = g_record_rate;
    record_config.format = format;
    record_config.channel_count = g_record_channel;
    record_config.device = g_record_mic_category;
    record_config.buffer_bytes = g_record_bytes_one_time; //0 means using default period bytes
    if (g_noirq_test) {
        record_config.buffer_bytes = g_record_bytes_one_time;
    }
    AudioRecord_Init(arecord, &record_config, flags);

    struct AudioTrack *audio_track;
    if (!g_only_record) {
        AudioService_Init();
        audio_track = AudioTrack_Create();
        if (!audio_track) {
            RTK_LOGE(TAG, "new AudioTrack failed, destroy record \n");
            AudioRecord_Destroy(arecord);
            return 0;
        }

        track_buf_size = AudioTrack_GetMinBufferBytes(audio_track, AUDIO_CATEGORY_MEDIA, g_record_rate, format, g_record_channel) * 4;

        AudioTrackConfig  track_config;
        track_config.category_type = AUDIO_CATEGORY_MEDIA;
        track_config.sample_rate = g_record_rate;
        track_config.format = format;
        track_config.channel_count = g_record_channel;
        if (g_noirq_test) {
            track_config.buffer_bytes = g_record_bytes_one_time;
        } else {
            track_config.buffer_bytes = track_buf_size;
        }
        AudioTrack_Init(audio_track, &track_config, flags);
    }

    AudioRecord_Start(arecord);
    if (!g_only_record) {
        AudioTrack_Start(audio_track);
    }
    g_arecord = arecord;

    for (unsigned int i = 0; i < MAX_CHANNEL_COUNT; i++) {
        AudioControl_SetChannelMicCategory(i, g_record_channel_src[i]);
    }

    AudioControl_SetCaptureVolume(4, 0x2f);
    AudioControl_SetMicBstGain(AUDIO_AMIC2, MICBST_GAIN_30DB);
    AudioControl_SetCaptureHpfFc(0, g_hpf_fc);
    int32_t ch0_hpf_fc = AudioControl_GetCaptureHpfFc(0);
    RTK_LOGI(TAG, "hpf fc for channel 0 is:%ld \n", ch0_hpf_fc);

    if (g_eq_filter_type) {
        AudioControl_SetCaptureEqEnable(0, true);
        EqFilterCoef coef;
        FillEqFilterCoef(&coef);
        AudioControl_SetCaptureEqFilter(0, 0, &coef);
        AudioControl_SetCaptureEqBand(0, 0, true);
    }

    switch (g_record_mode) {
    case 0:
        AudioRecord_SetParameters(arecord, "cap_mode=no_afe_pure_data");
        break;
    case 1:
        AudioRecord_SetParameters(arecord, "cap_mode=no_afe_all_data");
        break;
    default:
        break;
    }

    if (g_noirq_test) {
        AudioControl_AdjustPLLClock(g_record_rate, 0, AUDIO_PLL_AUTO);
    }

    size = g_record_bytes_one_time;

    buffer = (char *) malloc(size);
    if (!buffer) {
        RTK_LOGE(TAG, "failed to malloc buffer \n");
        return 0;
    }

    if (g_dump_buffer) {
        dump_buffer = (char *) malloc(DUMP_FRAME * g_record_channel * g_record_format / 8);
        if (!dump_buffer) {
            RTK_LOGE(TAG, "failed to malloc dump_buffer \n");
            free(buffer);
            return 0;
        }
    }

    RTK_LOGI(TAG, "Capturing sample: %u ch, %u hz, record bytes one time:%d, dump_buffer:%p \n",
             g_record_channel, g_record_rate, g_record_bytes_one_time, dump_buffer);
    do {
        size_read = AudioRecord_Read(arecord, buffer, size, true);
        if ((unsigned int)size_read != size) {
            RTK_LOGI(TAG, "opps size wanted:%d, size actually read:%d \n", size, size_read);
        }

        if (g_dump_buffer) {
            if (dumped_size + (int)size <= (int)(DUMP_FRAME * g_record_channel * g_record_format / 8)) {
                memcpy(dump_buffer + dumped_size, buffer, size);
                /* when dump, d2 needs invalidate, lite doesn't need invalidate */
#ifdef CONFIG_AMEBASMART
                DCache_Invalidate((u32)(dump_buffer + dumped_size), size);
#endif
                dumped_size += size;
            }
        }

        //drop first 100ms data of record, and instead send 0, because record need some time to be stable, it's normal.
        if (!g_only_record && bytes_read >= 100 * g_record_rate * g_record_channel * g_record_format / 8 / 1000) {
            AudioTrack_Write(audio_track, buffer, size, true);
        } else if (!g_only_record && !g_noirq_test) {
#ifndef CONFIG_AUDIO_MIXER
            memset(buffer, 0, size);
            AudioTrack_Write(audio_track, buffer, size, true);
            //To give another 0 buf at beginning, in case of xrun. For real case, please add ringbuffer between record and track.
            AudioTrack_Write(audio_track, buffer, size, true);
#endif
        }

        bytes_read += size;

        if (!g_pressure_test) {
            if (bytes_read >= record_size) {
                break;
            }
        }

    } while (1);

    frames = bytes_read / (g_record_channel * g_record_format / 8);

    free(buffer);
    buffer = NULL;

    if (g_dump_buffer) {
        free(dump_buffer);
        dump_buffer = NULL;
    }

    AudioRecord_Stop(arecord);
    AudioRecord_Destroy(arecord);
    if (!g_only_record) {
        AudioTrack_Stop(audio_track);
        AudioTrack_Destroy(audio_track);
    }

    return frames;
}

static void example_arecord_thread(void *param)
{
    unsigned int frames;
    unsigned int heap_start;
    unsigned int heap_end;
    unsigned int heap_min_ever_free;
    (void) param;

    RTK_LOGI(TAG, "[Mem] mem debug info init \n");
    heap_start = rtos_mem_get_free_heap_size();

    frames = record_sample();
    rtos_time_delay_ms(2 * RTOS_TICK_RATE_HZ);

    heap_end = rtos_mem_get_free_heap_size();
    heap_min_ever_free = rtos_mem_get_minimum_ever_free_heap_size();
    RTK_LOGI(TAG, "[Mem] start (0x%x), end (0x%x), \n", heap_start, heap_end);
    RTK_LOGI(TAG, " diff (%d), peak (%d) \n", heap_start - heap_end, heap_start - heap_min_ever_free);

    RTK_LOGI(TAG, "Recorded %u frames", frames);
    free(param);
    rtos_task_delete(NULL);
}

static int64_t last_frames_captured_ns = 0;
static int64_t last_frames_captured_at_ns = 0;
static int64_t last_phase_captured_ns = 0;
static int64_t last_phase_captured_at_ns = 0;
static int32_t ppm_test_cnt = 0;

//to test ppm between system clock and audio clock, please remember
//to use pll for audio record in ameba_audio_hw_usrcfg.h.
static void example_audio_counter_time(void *param)
{
    rtos_time_delay_ms(2 * RTOS_TICK_RATE_HZ);
    RTK_LOGI(TAG, "arecord time begin");
    (void) param;

    AudioTimestamp tstamp;
    int32_t frames_captured = 0;
    int64_t frames_captured_ns = 0;
    int64_t frames_captured_at_ns = 0;

    int64_t phase_captured_ns = 0;
    int64_t phase_captured_at_ns = 0;

    while (1) {
        if (!ppm_test_cnt) {
            rtos_time_delay_ms(1 * RTOS_TICK_RATE_HZ);
        } else {
            rtos_time_delay_ms(10 * RTOS_TICK_RATE_HZ);
        }

        if (AudioRecord_GetTimestamp(g_arecord, &tstamp) == AUDIO_OK) {
            frames_captured = tstamp.position;
            frames_captured_at_ns = tstamp.time.tv_sec * 1000000000LL + tstamp.time.tv_nsec;
            frames_captured_ns = (int64_t)((double)frames_captured / (double)g_record_rate * (double)1000000000);
        }

        if (AudioRecord_GetPresentTime(g_arecord, &phase_captured_at_ns, &phase_captured_ns) != AUDIO_OK) {
            RTK_LOGE(TAG, "get present time fail");
        }

        if (ppm_test_cnt) {
            RTK_LOGI(TAG, "ppm:%.16f frames_captured:%ld, frames_captured_ns:%lld, frames_captured_at_ns:%lld, last_frames_captured_ns:%lld, last_frames_captured_at_ns:%lld",
                     (double)(frames_captured_ns - last_frames_captured_ns  - (frames_captured_at_ns - last_frames_captured_at_ns)) / (double)(
                         frames_captured_at_ns - last_frames_captured_at_ns) * (double)1000000,
                     frames_captured, frames_captured_ns, frames_captured_at_ns, last_frames_captured_ns, last_frames_captured_at_ns);

            RTK_LOGI(TAG, "phase ppm:%.16f phase_captured_ns:%lld, phase_captured_at_ns:%lld, last_phase_captured_ns:%lld, last_phase_captured_at_ns:%lld",
                     (double)(phase_captured_ns - last_phase_captured_ns  - (phase_captured_at_ns - last_phase_captured_at_ns)) / (double)(
                         phase_captured_at_ns - last_phase_captured_at_ns) * (double)1000000,
                     phase_captured_ns, phase_captured_at_ns, last_phase_captured_ns, last_phase_captured_at_ns);
        }

        last_frames_captured_ns = frames_captured_ns;
        last_frames_captured_at_ns = frames_captured_at_ns;

        last_phase_captured_ns = phase_captured_ns;
        last_phase_captured_at_ns = phase_captured_at_ns;

        ppm_test_cnt ++;
    }

    free(param);
    rtos_task_delete(NULL);
}


static void arecord_help(void)
{
    RTK_LOGI(TAG,
        "\nUsage: arecord [OPTION VALUE]...\n"
        "  -r      <rate>      sample rate in Hz                          (default 16000)\n"
        "  -c      <channels>  record channel count                      (default 2)\n"
        "  -f      <format>    sample bits: 16 / 24 / 32                  (default 16)\n"
        "  -b      <bytes>     bytes read one time                       (default 8192)\n"
        "  -m      <mode>      0:no_afe_pure_data 1:no_afe_all_data       (default 0)\n"
        "  -or     <0|1>       1:only record  0:record then play          (default 0)\n"
        "  -noirq  <0|1>       1:no irq mode  0:irq mode                  (default 0)\n"
        "  -ref    <0|1>       1:test ref     0:not test ref              (default 0)\n"
        "  -pres   <0|1>       1:pressure test 0:record for fixed time    (default 0)\n"
        "  -t      <seconds>   record duration when -pres is 0           (default 600)\n"
        "  -hpf    <fc>        capture high-pass filter cutoff index      (default 3)\n"
        "  -filter <type>      capture EQ 1:LPF 2:HPF 3:BPF 4:LSF\n"
        "                                 5:HSF 6:NF  7:PF                (default 0:off)\n"
        "  -dump   <0|1>       1:dump recorded data to memory            (default 0)\n"
        "  -ts     <0|1>       1:test timestamp/ppm                      (default 0)\n"
        "  -d      <dev>       0:amic  1:dmic+amic ref  2:i2s            (default 0:amic)\n"
        "  -cNs    <src>       mic source for channel N (N=0..3)         (default amic1..3)\n"
        "\nExamples:\n"
        "  arecord -r 16000 -b 8192\n"
        "  arecord -c 1 -b 256 -noirq 1 -r 16000    (noirq: -b should be 8ms of bytes)\n");
}

static void parse_arecord_params(cmd_params_t *params, arecord_params_t *p)
{
    *p = ARECORD_DEFAULT_PARAMS;

    CMD_PARSE_INT(p->rate, "-r", ARECORD_DEFAULT_PARAMS.rate);
    CMD_PARSE_INT(p->channels, "-c", ARECORD_DEFAULT_PARAMS.channels);
    CMD_PARSE_INT(p->format, "-f", ARECORD_DEFAULT_PARAMS.format);
    CMD_PARSE_INT(p->bytes_one_time, "-b", ARECORD_DEFAULT_PARAMS.bytes_one_time);
    CMD_PARSE_INT(p->mode, "-m", ARECORD_DEFAULT_PARAMS.mode);
    CMD_PARSE_INT(p->only_record, "-or", ARECORD_DEFAULT_PARAMS.only_record);
    CMD_PARSE_INT(p->noirq_test, "-noirq", ARECORD_DEFAULT_PARAMS.noirq_test);
    CMD_PARSE_INT(p->test_ref, "-ref", ARECORD_DEFAULT_PARAMS.test_ref);
    CMD_PARSE_INT(p->pressure_test, "-pres", ARECORD_DEFAULT_PARAMS.pressure_test);
    CMD_PARSE_INT(p->hpf_fc, "-hpf", ARECORD_DEFAULT_PARAMS.hpf_fc);
    CMD_PARSE_INT(p->eq_filter_type, "-filter", ARECORD_DEFAULT_PARAMS.eq_filter_type);
    CMD_PARSE_INT(p->record_seconds, "-t", ARECORD_DEFAULT_PARAMS.record_seconds);
    CMD_PARSE_INT(p->dump_buffer, "-dump", ARECORD_DEFAULT_PARAMS.dump_buffer);
    CMD_PARSE_INT(p->test_timestamp, "-ts", ARECORD_DEFAULT_PARAMS.test_timestamp);

    int mic_val = 0;
    CMD_PARSE_INT(mic_val, "-d", 0);
    if (mic_val == 1) {
        p->mic_category = DEVICE_IN_DMIC_REF_AMIC;
    } else if (mic_val == 2) {
        p->mic_category = DEVICE_IN_I2S;
    }

    CMD_PARSE_INT(p->channel_src[0], "-c0s", AUDIO_AMIC1);
    CMD_PARSE_INT(p->channel_src[1], "-c1s", AUDIO_AMIC2);
    CMD_PARSE_INT(p->channel_src[2], "-c2s", AUDIO_AMIC3);
    CMD_PARSE_INT(p->channel_src[3], "-c3s", 0);
}

static uint32_t arecord_handler(cmd_params_t *params)
{
    if (params->argc <= 1) {
        arecord_help();
        return TRUE;
    }

    arecord_params_t p;
    parse_arecord_params(params, &p);

    g_record_rate           = p.rate;
    g_record_channel        = p.channels;
    g_record_format         = p.format;
    g_record_bytes_one_time = p.bytes_one_time;
    g_record_mode           = p.mode;
    g_only_record           = p.only_record;
    g_noirq_test            = p.noirq_test;
    g_pressure_test         = p.pressure_test;
    g_record_mic_category   = p.mic_category;
    for (unsigned int i = 0; i < MAX_CHANNEL_COUNT; i++) {
        g_record_channel_src[i] = p.channel_src[i];
    }
    g_hpf_fc         = p.hpf_fc;
    g_eq_filter_type = p.eq_filter_type;
    g_record_seconds = p.record_seconds;
    g_dump_buffer    = p.dump_buffer;
    g_test_timestamp = p.test_timestamp;

    RTK_LOGI(TAG, "arecord params: rate=%u, channels=%u, format=%u, bytes=%u, or=%u, mic_cat=%u \n",
                p.rate, p.channels, p.format, p.bytes_one_time, p.only_record, p.mic_category);

    if (RTK_SUCCESS != rtos_task_create(NULL, ((const char *)"record_task"), example_arecord_thread, NULL, 5376, 5)) {
        RTK_LOGE(TAG, "rtos_task_create(record_task) failed \n");
    }

    if (g_test_timestamp) {
        if (rtos_task_create(NULL, ((const char *)"example_audio_counter_time"), example_audio_counter_time, NULL, 8192 * 4, 1) != RTK_SUCCESS) {
            RTK_LOGE(TAG, "error: rtos_task_create(example_audio_counter_time) failed");
        }
    }

    return TRUE;
}

DEFINE_CMD_WRAPPER(arecord, arecord_handler, 5)

CMD_TABLE_DATA_SECTION
const COMMAND_TABLE arecord_cmd_table[] = {
    {
        "arecord", arecord_cmd_thread
    },
};
