#define SM_LOG_TAG "AppE"
#include <stdio.h>

#include "speech_log.h"
#include "common/audio_errnos.h"
#include "os_wrapper.h"

#include "speech_mind.h"
#include "speech_utils.h"

void OnVad(SpeechMindCallback *callback, VadInfo *info)
{
    UNUSED(callback);
    SM_LOGI("vad info status %d offset_ms %u", info->status, info->offset_ms);
}

void OnWakeUp(SpeechMindCallback *callback, WakeUpInfo *info)
{
    UNUSED(callback);
    SM_LOGI("wake up info len %d words %s", info->len, info->wakeup_words);
}

void OnAfe(SpeechMindCallback *callback, AfeInfo *info)
{
    UNUSED(callback);
    UNUSED(info);
}

void OnAsr(SpeechMindCallback *callback, AsrInfo *info)
{
    UNUSED(callback);
    SM_LOGI("asr result len %d json %s", info->len, info->result_json ? info->result_json : "null");
}

void OnAsrRecTimeout(SpeechMindCallback *callback)
{
    UNUSED(callback);
    SM_LOGI("asr timeout");
}

/* Callback table is a fixed set of function pointers stored by SpeechMind for
 * its whole lifetime, so keep it static instead of heap-allocating and leaking. */
static SpeechMindCallback g_callback = {
    .OnVad = OnVad,
    .OnWakeUp = OnWakeUp,
    .OnAfe = OnAfe,
    .OnAsr = OnAsr,
    .OnAsrRecTimeout = OnAsrRecTimeout,
};

static void SpeechMind_task(void *param)
{
    UNUSED(param);

    SpeechMind_Init();
    SpeechMind_SetCallback(&g_callback);
    SpeechMind_Start();
    rtos_task_delete(NULL);
}

void app_example(void)
{
    if (rtos_task_create(NULL, "speechmind_init", SpeechMind_task, NULL, SPEECHMIND_INIT_STACK_SIZE, 3) != 0) {
        SM_LOGE("create speechmind init task fail!");
    }
}
