#ifndef AMEBA_AUDIO_APPS_SPEECHMIND_SPEECH_UTILS_H
#define AMEBA_AUDIO_APPS_SPEECHMIND_SPEECH_UTILS_H

/* Mark a parameter or variable as deliberately unused.
 * basic_types.h already defines this on some SoCs, so only fill the gap. */
#ifndef UNUSED
#define UNUSED(x) ((void)(x))
#endif

#define FRAME_MS 16
#define IN_CHANNEL 3
#define SAMPLE_RATE 16000

#define VAD_TIMEOUT_MS 10000

/* osal_thread_param.stack_size is in BYTES (osal_thread.c divides by
 * sizeof(StackType_t) before calling xTaskCreate). Sizes below were measured on
 * 0914 via a stack high-water probe (recordLoop peaked at 488/20480 B, the
 * speechmind init task at 976/8192 B) and set to roughly 2x that peak. */
#define AUDIO_CAPTURE_STACK_SIZE    1024
#define SPEECHMIND_INIT_STACK_SIZE  2048

#endif /* AMEBA_AUDIO_APPS_SPEECHMIND_SPEECH_UTILS_H */
