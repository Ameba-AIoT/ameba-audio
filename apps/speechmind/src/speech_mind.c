#define SM_LOG_TAG "SpeechMind"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "common/audio_errnos.h"

#include "audio/audio_service.h"
#include "audio/audio_control.h"

#include "audio_capture.h"
#include "aivoice_manager.h"
#include "playlist_parser.h"
#include "audio_dump.h"

#include "speech_log.h"
#include "speech_utils.h"

#include "speech_mind.h"

#define ENABLE_SSL_DOA 1
#define AMPLIFIER_VOLUME 0.8f

typedef enum SpeechMindState {
    SPEECH_IDLE,       /* nothing acquired */
    SPEECH_READY,      /* pipeline loaded, not listening */
    SPEECH_RUNNING,    /* listening */
} SpeechMindState;

typedef struct SpeechMind {
    SpeechMindState state;
    AIVoiceManager *aivoice;
    AudioCapture *audio_capture;
    PlaylistParser *parser;
    SpeechMindCallback *callback;
} SpeechMind;

static SpeechMind g_speech;

// workaround function for AmebaLite compilation issue
__attribute__((weak)) afe_ns_mode_e AFE_NS_SIGNAL_SET(void) {
    return AFE_NS_SIGNAL;
}

static void OnAIVoiceEvent(enum aivoice_out_event_type event_type,
                           const void *msg, int len, void *usr_data)
{
    UNUSED(usr_data);

    struct aivoice_evout_vad *vad_out;
    struct aivoice_evout_afe *afe_out;
    static bool wakeup_triggered = false;
    /* Cached once: the app may leave any member NULL, so each dispatch
     * below checks the specific slot rather than just the table. */
    SpeechMindCallback *cb = g_speech.callback;

    switch (event_type) {
        case AIVOICE_EVOUT_VAD:
            vad_out = (struct aivoice_evout_vad *)msg;
            SM_LOGI("[user] vad. status = %d, offset = %d", vad_out->status, vad_out->offset_ms);
            if (!vad_out->status && vad_out->offset_ms >= VAD_TIMEOUT_MS) {
                SM_LOGI("[user] vad timeout, reset");
                }

            if (cb && cb->OnVad) {
                VadInfo info;
                info.status = vad_out->status;
                info.offset_ms = vad_out->offset_ms;
                cb->OnVad(cb, &info);
            }

            break;

        case AIVOICE_EVOUT_WAKEUP:
            SM_LOGI("wake up");
                wakeup_triggered = true;
            /* Barge-in: preempt current playback and play the wake-up prompt. */
            PlaylistParser_WakeUp(g_speech.parser);

            if (cb && cb->OnWakeUp) {
                WakeUpInfo info;
                info.len = len;
                info.wakeup_words = msg;
                cb->OnWakeUp(cb, &info);
            }
            break;

        case AIVOICE_EVOUT_ASR_RESULT:
            PlaylistParser_AsrMsg(g_speech.parser, (const char *)msg);
            if (cb && cb->OnAsr) {
                AsrInfo info;
                info.len = len;
                info.result_json = msg;
                cb->OnAsr(cb, &info);
            }
            break;

        case AIVOICE_EVOUT_AFE:
            afe_out = (struct aivoice_evout_afe *)msg;
            AudioDump_FeedAfe(afe_out->data);

            // afe will output audio each frame.
            // in this example, we only print it once to make log clear
            if (wakeup_triggered) {
                wakeup_triggered = false;
                SM_LOGI("[user] afe output %d channels raw audio, others: %s",
                           afe_out->ch_num, afe_out->out_others_json?afe_out->out_others_json:"null");
            }

            if (cb && cb->OnAfe) {
                AfeInfo info;
                info.len = len;
                info.data = (uint8_t*)msg;
                cb->OnAfe(cb, &info);
            }

            break;

        case AIVOICE_EVOUT_ASR_REC_TIMEOUT:
            PlaylistParser_VadTimeout(g_speech.parser);
            if (cb && cb->OnAsrRecTimeout) {
                cb->OnAsrRecTimeout(cb);
            }
            break;

        default:
            break;
    }
}

static void OnAudioCaptureData(const void *data, int len, void *user_data) {
    UNUSED(user_data);
    if (g_speech.aivoice) {
        AIVoiceManager_Feed(g_speech.aivoice, data, len);
        /* only frames accepted by aivoice produce afe output, dump them to keep raw and afe paired */
        AudioDump_FeedRaw((const int16_t *)data);
    }
}

int SpeechMind_Init(void) {
    SM_LOGD("Init");

    if (g_speech.state != SPEECH_IDLE) {
        SM_LOGE("Already inited");
        return AUDIO_ERR_INVALID_OPERATION;
    }

    int err = AUDIO_OK;

    AudioService_Init();
    AudioControl_SetHardwareVolume(AMPLIFIER_VOLUME, AMPLIFIER_VOLUME);
    AudioDump_Init();

    // Create PlaylistParser, drives prompt and music playback
    g_speech.parser = PlaylistParser_Create();
    if (!g_speech.parser) {
        SM_LOGE("Create playlist parser failed");
        err = AUDIO_ERR_NO_MEMORY;
        goto cleanup;
    }

    // Create AudioCapture
    g_speech.audio_capture = AudioCapture_Create();
    if (!g_speech.audio_capture) {
        SM_LOGE("Create audio capture failed");
        err = AUDIO_ERR_NO_MEMORY;
        goto cleanup;
    }
    AudioCapture_SetCallback(g_speech.audio_capture, OnAudioCaptureData, NULL);

    // Create AIVoice
    struct aivoice_config config;
    memset(&config, 0, sizeof(config));

    struct afe_config afe_param = AFE_CONFIG_ASR_DEFAULT_2MIC50MM();
#if ENABLE_SSL_DOA
    afe_param.enable_ssl = true;
    afe_param.ssl_resolution = 15;
#else
    afe_param.enable_ssl = false;
#endif

    struct vad_config vad_param = VAD_CONFIG_DEFAULT();
    vad_param.left_margin = 300; // user can change the configure if needed
    struct kws_config kws_param = KWS_CONFIG_DEFAULT();
    struct asr_config asr_param = ASR_CONFIG_DEFAULT();
    struct aivoice_sdk_config aivoice_param = AIVOICE_SDK_CONFIG_DEFAULT();
    aivoice_param.timeout = 10;

    config.afe = &afe_param;
    config.vad = &vad_param;
    config.kws = &kws_param;
    config.asr = &asr_param;
    config.common = &aivoice_param;

    g_speech.aivoice = AIVoiceManager_Create(FULL_FLOW_V1, &config);
    if (!g_speech.aivoice) {
        SM_LOGE("Create aivoice failed");
        err = AUDIO_ERR_NO_MEMORY;
        goto cleanup;
    }
    AIVoiceManager_SetCallback(g_speech.aivoice, OnAIVoiceEvent, &g_speech);

    g_speech.state = SPEECH_READY;
    return AUDIO_OK;

cleanup:
    if (g_speech.aivoice) {
        AIVoiceManager_Destroy(g_speech.aivoice);
        g_speech.aivoice = NULL;
    }
    if (g_speech.audio_capture) {
        AudioCapture_Destroy(g_speech.audio_capture);
        g_speech.audio_capture = NULL;
    }
    if (g_speech.parser) {
        PlaylistParser_Destroy(g_speech.parser);
        g_speech.parser = NULL;
    }
    AudioDump_Deinit();
    return err;
}

int SpeechMind_SetCallback(SpeechMindCallback *callback) {
    if (g_speech.state == SPEECH_IDLE) {
        SM_LOGE("Not inited");
        return AUDIO_ERR_INVALID_OPERATION;
    }

    g_speech.callback = callback;
    return AUDIO_OK;
}

int SpeechMind_Start(void) {
    SM_LOGD("Start");

    if (g_speech.state != SPEECH_READY) {
        SM_LOGE("Not ready, state %d", g_speech.state);
        return AUDIO_ERR_INVALID_OPERATION;
    }

    int err = AudioCapture_Start(g_speech.audio_capture, SAMPLE_RATE, IN_CHANNEL);
    if (err) {
        SM_LOGE("Start audio capture failed");
        return err;
    }

    g_speech.state = SPEECH_RUNNING;
    return AUDIO_OK;
}

int SpeechMind_Stop(void) {
    SM_LOGD("Stop");

    if (g_speech.state != SPEECH_RUNNING) {
        SM_LOGE("Not started");
        return AUDIO_ERR_INVALID_OPERATION;
    }

    /* Stops the capture thread, so no more frames are fed to aivoice.
     * The aivoice instance stays alive so Start() can resume without
     * reloading its resources. */
    AudioCapture_Stop(g_speech.audio_capture);

    g_speech.state = SPEECH_READY;
    return AUDIO_OK;
}

void SpeechMind_Deinit(void) {
    SM_LOGD("Deinit");

    if (g_speech.state == SPEECH_IDLE) {
        return;
    }

    if (g_speech.state == SPEECH_RUNNING) {
        SpeechMind_Stop();
    }

    if (g_speech.aivoice) {
        AIVoiceManager_SetCallback(g_speech.aivoice, NULL, NULL);
        AIVoiceManager_Destroy(g_speech.aivoice);
        g_speech.aivoice = NULL;
    }

    if (g_speech.audio_capture) {
        AudioCapture_Destroy(g_speech.audio_capture);
        g_speech.audio_capture = NULL;
    }

    if (g_speech.parser) {
        PlaylistParser_Destroy(g_speech.parser);
        g_speech.parser = NULL;
    }

    AudioDump_Deinit();

    g_speech.callback = NULL;
    g_speech.state = SPEECH_IDLE;
}
