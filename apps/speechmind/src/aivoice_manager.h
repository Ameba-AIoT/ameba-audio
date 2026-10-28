#ifndef AMEBA_AUDIO_APPS_SPEECHMIND_AIVOICE_MANAGER_H
#define AMEBA_AUDIO_APPS_SPEECHMIND_AIVOICE_MANAGER_H

#include <stdint.h>
#include "aivoice_interface.h"

typedef struct AIVoiceManager AIVoiceManager;

typedef enum AIVoiceFlowType {
    FULL_FLOW_V1,
    AFE_KWS_V1,
    AFE_KWS_VAD_V1,
    AFE_V1,
    VAD_V1,
    KWS_V1,
    ASR_V1,
} AIVoiceFlowType;

typedef void (*AIVoiceManagerCallback)( enum aivoice_out_event_type event_type,
                                        const void *msg, int len, void* user_data);

AIVoiceManager *AIVoiceManager_Create(AIVoiceFlowType type, struct aivoice_config *config);
int AIVoiceManager_Feed(AIVoiceManager *manager, const void *data, int len);
int AIVoiceManager_Destroy(AIVoiceManager *manager);
int AIVoiceManager_SetCallback(AIVoiceManager *manager, AIVoiceManagerCallback callback, void* usr_data);

#endif /* AMEBA_AUDIO_APPS_SPEECHMIND_AIVOICE_MANAGER_H */
