#ifndef AMEBA_AUDIO_APPS_SPEECHMIND_SPEECH_TTS_H
#define AMEBA_AUDIO_APPS_SPEECHMIND_SPEECH_TTS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TtsInfo {
    int have_slot;
    const char *tts_name;
    const char *slot_name;
    const char *suffix;
} TtsInfo;

typedef struct IntentTtsInfo {
    int intent_id;
    int sub_tts_nums;
    TtsInfo tts[3];
} IntentTtsInfo;

const char *SpeechTts_GetNameByContent(const char *content);
const IntentTtsInfo *SpeechTts_GetIntentInfo(int id);

#ifdef __cplusplus
}
#endif

#endif /* AMEBA_AUDIO_APPS_SPEECHMIND_SPEECH_TTS_H */
