#ifndef AMEBA_AUDIO_APPS_SPEECHMIND_AUDIO_CAPTURE_H
#define AMEBA_AUDIO_APPS_SPEECHMIND_AUDIO_CAPTURE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*AudioCaptureCallback)(const void *data, int len, void *ctx);

typedef struct AudioCapture AudioCapture;

AudioCapture *AudioCapture_Create(void);
void AudioCapture_Destroy(AudioCapture *ac);
void AudioCapture_SetCallback(AudioCapture *ac, AudioCaptureCallback callback, void *ctx);

/* Format is per-run: Stop() then Start() again to change it, no need to
 * recreate the capture. */
int AudioCapture_Start(AudioCapture *ac, uint32_t rate, uint32_t channels);
int AudioCapture_Stop(AudioCapture *ac);

#ifdef __cplusplus
}
#endif

#endif /* AMEBA_AUDIO_APPS_SPEECHMIND_AUDIO_CAPTURE_H */