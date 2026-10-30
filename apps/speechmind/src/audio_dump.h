#ifndef AMEBA_AUDIO_APPS_SPEECHMIND_AUDIO_DUMP_H
#define AMEBA_AUDIO_APPS_SPEECHMIND_AUDIO_DUMP_H

#include <stdint.h>

#include "platform_autoconf.h"

/* Set to 1 to enable compiling the Audio Dump feature */
#define SPEECHMIND_AUDIO_DUMP_EN 0

/* Dump raw mic/ref and afe output to RtkAudioRecorder over UART, see README.md. */
#if SPEECHMIND_AUDIO_DUMP_EN

void AudioDump_Init(void);
void AudioDump_Deinit(void);

/* one frame of 3ch interleaved capture data (mic1, mic2, ref) that aivoice accepted */
void AudioDump_FeedRaw(const int16_t *pcm);

/* one frame of 1ch afe output */
void AudioDump_FeedAfe(const int16_t *pcm);

#else

#define AudioDump_Init()        do {} while (0)
#define AudioDump_Deinit()      do {} while (0)
#define AudioDump_FeedRaw(pcm)  do {} while (0)
#define AudioDump_FeedAfe(pcm)  do {} while (0)

#endif

#endif /* AMEBA_AUDIO_APPS_SPEECHMIND_AUDIO_DUMP_H */
