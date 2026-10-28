#define SM_LOG_TAG "SpeechT"

#include <stdio.h>
#include <string.h>

#include "../speech_log.h"
#include "../speech_utils.h"
#include "common/audio_errnos.h"
#include "osal_c/osal_mem.h"
#include "osal_c/osal_time.h"
#include "osal_c/osal_thread.h"
#include "media/parcel.h"

#include "cutils/ring_buffer.h"
#include "aivoice_interface.h"

#include "vfs.h"

#include "aivoice_manager.h"

struct AIVoiceManager {
    void *ai_voice_handle;
    const struct rtk_aivoice_iface *aivoice;
    AIVoiceManagerCallback callback;
    void* usr_data;
};

int AIVoiceManager_AIVoiceCallbackProcess(void *userdata,
        enum aivoice_out_event_type event_type,
        const void *msg, int len)
{
    AIVoiceManager *manager = (AIVoiceManager *)userdata;
    if (manager && manager->callback) {
        manager->callback(event_type, msg, len, manager->usr_data);
    }

    return 0;
}

AIVoiceManager *AIVoiceManager_Create(AIVoiceFlowType type, struct aivoice_config *config)
{
    AIVoiceManager *manager = osal_malloc(sizeof(AIVoiceManager));
    if (!manager) {
        SM_LOGE("malloc aivoice manager fail");
        return NULL;
    }

    manager->callback = NULL;

    UNUSED(type);

    // Trimmed code: removed switch, only keep FULL_FLOW_V1 type
    manager->aivoice = &aivoice_iface_full_flow_v1;

    // switch (type) {
    // case FULL_FLOW_V1:
    //     manager->aivoice = &aivoice_iface_full_flow_v1;
    //     break;

    // case AFE_KWS_V1:
    //     manager->aivoice = &aivoice_iface_afe_kws_v1;
    //     break;

    // case AFE_KWS_VAD_V1:
    //     manager->aivoice = &aivoice_iface_afe_kws_vad_v1;
    //     break;

    // case AFE_V1:
    //     manager->aivoice = &aivoice_iface_afe_v1;
    //     break;

    // case VAD_V1:
    //     manager->aivoice = &aivoice_iface_vad_v1;
    //     break;

    // case KWS_V1:
    //     manager->aivoice = &aivoice_iface_kws_v1;
    //     break;

    // case ASR_V1:
    //     manager->aivoice = &aivoice_iface_asr_v1;
    //     break;

    // default:
    //     manager->aivoice = &aivoice_iface_full_flow_v1;
    //     break;
    // }

    manager->ai_voice_handle = manager->aivoice->create(config);

    rtk_aivoice_register_callback(manager->ai_voice_handle, AIVoiceManager_AIVoiceCallbackProcess, manager);

    return manager;
}


int AIVoiceManager_Feed(AIVoiceManager *manager, const void* data, int len)
{
    if (!manager) {
        SM_LOGE("manager is NULL");
        return 0;
    }

    int res = manager->aivoice->feed(manager->ai_voice_handle,
                                         (char *)data, len);

    return res;
}

int AIVoiceManager_Destroy(AIVoiceManager *manager)
{
    if (!manager) {
        return AUDIO_ERR_INVALID_PARAM;
    }

    return AUDIO_OK;
}

int AIVoiceManager_SetCallback(AIVoiceManager *manager, AIVoiceManagerCallback callback, void* usr_data)
{
    if (!manager) {
        return AUDIO_ERR_INVALID_PARAM;
    }

    manager->callback = callback;
    manager->usr_data = usr_data;

    return AUDIO_OK;
}
