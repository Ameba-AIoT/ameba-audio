#ifndef AMEBA_AUDIO_APPS_SPEECHMIND_SPEECH_LOG_H
#define AMEBA_AUDIO_APPS_SPEECHMIND_SPEECH_LOG_H

#include <stdint.h>

#include "log.h"

#ifndef SM_LOG_TAG
#define SM_LOG_TAG "SpeechM"
#endif

#define SM_LOGE(fmt, ...) RTK_LOGS(SM_LOG_TAG, RTK_LOG_ERROR, fmt "\n", ##__VA_ARGS__)
#define SM_LOGW(fmt, ...) RTK_LOGS(SM_LOG_TAG, RTK_LOG_WARN,  fmt "\n", ##__VA_ARGS__)
#define SM_LOGI(fmt, ...) RTK_LOGS(SM_LOG_TAG, RTK_LOG_INFO,  fmt "\n", ##__VA_ARGS__)
#define SM_LOGD(fmt, ...) RTK_LOGS(SM_LOG_TAG, RTK_LOG_DEBUG, fmt "\n", ##__VA_ARGS__)

#endif /* AMEBA_AUDIO_APPS_SPEECHMIND_SPEECH_LOG_H */
