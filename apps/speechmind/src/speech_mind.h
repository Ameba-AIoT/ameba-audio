#ifndef AMEBA_AUDIO_APPS_SPEECHMIND_SPEECH_MIND_H
#define AMEBA_AUDIO_APPS_SPEECHMIND_SPEECH_MIND_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SpeechMindCallback SpeechMindCallback;

typedef struct VadInfo {
    int status;                     /*  0: vad is changed from speech to silence,
                                            indicating the end point of a speech segment
                                        1: vad is changed from silence to speech,
                                           indicating the start point of a speech segment */
    unsigned int offset_ms;         /* time offset relative to reset point. */
} VadInfo;

typedef struct WakeUpInfo {
    int len;
    const char *wakeup_words;
} WakeUpInfo;

typedef struct AfeInfo {
    int len;
    uint8_t *data;
} AfeInfo;

typedef struct AsrInfo {
    int len;
    const char *result_json;        /* recognition result, JSON text */
} AsrInfo;

struct SpeechMindCallback {
    void (*OnVad)(SpeechMindCallback *callback, VadInfo *info);
    void (*OnWakeUp)(SpeechMindCallback *callback, WakeUpInfo *info);
    void (*OnAfe)(SpeechMindCallback *callback, AfeInfo *info);
    void (*OnAsr)(SpeechMindCallback *callback, AsrInfo *info);
    void (*OnAsrRecTimeout)(SpeechMindCallback *callback);
};

/**
 * @brief Acquire everything the pipeline needs: audio capture and the aivoice
 *        instance (which loads its resources). Call once before SpeechMind_Start().
 *
 * @return AUDIO_OK on success, AUDIO_ERR_INVALID_OPERATION if already inited,
 *         or a negative AUDIO_ERR_* on failure.
 */
int SpeechMind_Init(void);

/**
 * @brief Register the event callback table. May be called while running to
 *        swap callbacks, or with NULL to stop receiving events.
 *
 * @param[in] callback Kept by reference, so it must outlive the pipeline.
 *                     Unused members must be NULL.
 * @return AUDIO_OK on success, AUDIO_ERR_INVALID_OPERATION if not inited.
 */
int SpeechMind_SetCallback(SpeechMindCallback *callback);

/**
 * @brief Start listening. Cheap enough to pair with SpeechMind_Stop()
 *        repeatedly; the aivoice resources stay loaded in between.
 *
 * @return AUDIO_OK on success, AUDIO_ERR_INVALID_OPERATION if not inited or
 *         already started.
 */
int SpeechMind_Start(void);

/**
 * @brief Stop listening but keep the pipeline loaded, so SpeechMind_Start()
 *        can resume without reloading aivoice resources.
 *
 * @return AUDIO_OK on success, AUDIO_ERR_INVALID_OPERATION if not started.
 */
int SpeechMind_Stop(void);

/**
 * @brief Release everything acquired by SpeechMind_Init(). Stops the pipeline
 *        first if it is still running.
 */
void SpeechMind_Deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* AMEBA_AUDIO_APPS_SPEECHMIND_SPEECH_MIND_H */
