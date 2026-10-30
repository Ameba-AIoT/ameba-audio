#define SM_LOG_TAG "AudioCapture"

#include "platform_autoconf.h"

#include "common/audio_errnos.h"

#include "osal_c/osal_mem.h"
#include "osal_c/osal_time.h"
#include "osal_c/osal_thread.h"

#include "audio/audio_record.h"
#include "audio/audio_control.h"

#include "speech_log.h"
#include "speech_utils.h"

#include "audio_capture.h"

/* No 3ch TDM in driver: a 3ch request makes HAL malloc a 4ch staging stream_buf.
 * Request 4ch directly and drop ch4 here instead, saving that buffer. */
#define CAPTURE_HW_CHANNELS(ch) ((ch) == 3 ? 4 : (ch))

struct AudioCapture {
    struct AudioRecord *record;
    osal_thread_t thread;
    bool running;
    AudioCaptureCallback callback;
    void *ctx;
    uint32_t rate;
    uint32_t channels;
};

static void *AudioRecordLoop(void *param) {
    SM_LOGD("%s enter");

    AudioCapture *ac = (AudioCapture *)param;
    uint32_t ch = ac->channels;
    uint32_t hw_ch = CAPTURE_HW_CHANNELS(ch);
    int size = (ac->rate * ch * 2 * FRAME_MS) / 1000L;
    int hw_size = (ac->rate * hw_ch * 2 * FRAME_MS) / 1000L;
    uint8_t *data = osal_malloc(hw_size);
    if (!data) {
        SM_LOGE("alloc record buffer failed");
        return NULL;
    }

    while (ac->running) {
        AudioRecord_Read(ac->record, data, hw_size, true);
        if (hw_ch != ch) {
            /* In-place compact hw_ch -> ch (dst index never passes src index). */
            int16_t *pcm = (int16_t *)data;
            for (uint32_t i = 0; i < (uint32_t)hw_size / (hw_ch * 2); i++) {
                for (uint32_t c = 0; c < ch; c++) {
                    pcm[i * ch + c] = pcm[i * hw_ch + c];
                }
            }
        }
        if (ac->callback) {
            ac->callback(data, size, ac->ctx);
        }
    }

    osal_free(data);
    SM_LOGD("%s exit", __FUNCTION__);
    return NULL;
}

AudioCapture *AudioCapture_Create(void) {
    AudioCapture *ac = (AudioCapture *)osal_calloc(sizeof(AudioCapture));
    return ac;
}

void AudioCapture_Destroy(AudioCapture *ac) {
    if (ac) {
        AudioCapture_Stop(ac);
        osal_free(ac);
    }
}

void AudioCapture_SetCallback(AudioCapture *ac, AudioCaptureCallback callback, void *ctx) {
    if (ac) {
        ac->callback = callback;
        ac->ctx = ctx;
    }
}

int AudioCapture_Start(AudioCapture *ac, uint32_t rate, uint32_t channels) {
    if (!ac || !rate || !channels) {
        return AUDIO_ERR_INVALID_PARAM;
    }

    if (ac->thread) {
        SM_LOGE("already started");
        return AUDIO_ERR_INVALID_OPERATION;
    }

    ac->record = AudioRecord_Create();
    if (!ac->record) {
        SM_LOGE("record create failed");
        return AUDIO_ERR_NO_MEMORY;
    }

    AudioRecordConfig cfg;
    cfg.sample_rate = rate;
    cfg.format = AUDIO_FORMAT_PCM_16_BIT;
    cfg.channel_count = CAPTURE_HW_CHANNELS(channels);
    cfg.device = DEVICE_IN_MIC;
    /* period_size = buffer_bytes / frame_size(4ch*2B=8) = 256 samples = 16ms @16k,
     * matching the FRAME_MS read size. DMA ring = 8192 bytes (period_count 4, depth 64ms). */
    cfg.buffer_bytes = (rate * cfg.channel_count * 2 * FRAME_MS) / 1000L;
    AudioRecord_Init(ac->record, &cfg, AUDIO_INPUT_FLAG_NONE);
    AudioRecord_Start(ac->record);

    // Replace AUDIO_AMICS with your board's actual MIC category defines.
#if defined (CONFIG_AMEBALITE)
    AudioControl_SetChannelMicCategory(0, AUDIO_AMIC1);
    AudioControl_SetChannelMicCategory(1, AUDIO_AMIC2);
    AudioControl_SetChannelMicCategory(2, AUDIO_AMIC3);

    AudioControl_SetMicBstGain(AUDIO_AMIC1, AUDIO_MICBST_GAIN_15DB);
    AudioControl_SetMicBstGain(AUDIO_AMIC2, AUDIO_MICBST_GAIN_15DB);

#elif defined(CONFIG_AMEBASMART)
    AudioControl_SetChannelMicCategory(0, AUDIO_AMIC1);
    AudioControl_SetMicBstGain(AUDIO_AMIC1, AUDIO_MICBST_GAIN_15DB);
    AudioControl_SetChannelMicCategory(1, AUDIO_AMIC3); // EA
    AudioControl_SetMicBstGain(AUDIO_AMIC3, AUDIO_MICBST_GAIN_15DB); // EA
    // AudioControl_SetChannelMicCategory(1, AUDIO_AMIC4);  // EL
    // AudioControl_SetMicBstGain(AUDIO_AMIC4, AUDIO_MICBST_GAIN_0DB); // EL
    AudioControl_SetChannelMicCategory(2, AUDIO_AMIC5);
    AudioControl_SetMicBstGain(AUDIO_AMIC5, AUDIO_MICBST_GAIN_5DB);
#endif
    AudioRecord_SetParameters(ac->record, "cap_mode=no_afe_pure_data");

    ac->rate = rate;
    ac->channels = channels;
    ac->running = true;

    osal_thread_param param = {
        .priority = OSAL_THREAD_PRI_AUDIO,
        .stack_size = AUDIO_CAPTURE_STACK_SIZE,
        .joinable = true,
        .name = "audio_capture"
    };

    if (osal_thread_create(&ac->thread, AudioRecordLoop, ac, &param)) {
        SM_LOGE("Create audio record task fail");
        ac->running = false;
        AudioCapture_Stop(ac);
        return AUDIO_ERR_NO_MEMORY;
    }

    return AUDIO_OK;
}

int AudioCapture_Stop(AudioCapture *ac) {
    if (!ac) {
        return AUDIO_ERR_INVALID_PARAM;
    }

    if (ac->thread) {
        ac->running = false;
        osal_thread_join(ac->thread, NULL);
        ac->thread = NULL;
    }

    if (ac->record) {
        AudioRecord_Stop(ac->record);
        AudioRecord_Destroy(ac->record);
        ac->record = NULL;
    }

    return AUDIO_OK;
}
