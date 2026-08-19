/*
 * Copyright (c) 2026 Realtek, LLC.
 * All rights reserved.
 *
 * Licensed under the Realtek License, Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License from Realtek
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @addtogroup Audio
 * @{
 *
 * @brief Declares APIs for audio framework.
 *
 *
 * @since 1.0
 * @version 1.0
 */

/**
 * @file audio_spectrum.h
 *
 * @brief Provides APIs of the audio spectrum analyzer.
 *
 * Unlike the audio equalizer, the spectrum analyzer does NOT modify the audio
 * stream and the caller does NOT feed PCM data to it. Instead, the caller binds
 * the analyzer to an audio session (for example the primary output). The audio
 * stream layer then feeds the final PCM of that session's thread to the
 * analyzer, and the caller pulls the frequency-domain result back with
 * {@link AudioSpectrum_GetFft}. This mirrors the design of Android's
 * android.media.audiofx.Visualizer.
 *
 * @since 1.0
 * @version 1.0
 */

#ifndef AMEBA_AUDIO_INTERFACES_AUDIO_AUDIO_SPECTRUM_H
#define AMEBA_AUDIO_INTERFACES_AUDIO_AUDIO_SPECTRUM_H

#include <stdint.h>
#include <stdbool.h>

#include "audio/audio_type.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup AudioSpectrum_Types AudioSpectrum Types
 * @{
 */

struct AudioSpectrum;

/**
 * @brief Defines the audio session a spectrum analyzer can be attached to.
 *
 * The session selects which stream the audio framework taps and feeds to the
 * analyzer. It does NOT mean the caller provides the data.
 */
enum {
    /** tap the final PCM of the primary output (speaker) mixing thread */
    AUDIO_SPECTRUM_SESSION_OUTPUT_PRIMARY = 0x0,
};

/** @} End of AudioSpectrum_Types group */

/**
 * @defgroup AudioSpectrum_Functions AudioSpectrum Functions
 * @{
 */

/**
 * @brief Create AudioSpectrum instance.
 * @return Returns the instance pointer of AudioSpectrum.
 * @since 1.0
 * @version 1.0
 */
struct AudioSpectrum *AudioSpectrum_Create(void);

/**
 * @brief Release AudioSpectrum.
 * @param spectrum is the pointer of struct AudioSpectrum.
 * @since 1.0
 * @version 1.0
 */
void AudioSpectrum_Destroy(struct AudioSpectrum *spectrum);

/**
 * @brief Init AudioSpectrum and bind it to an audio session.
 * @param spectrum is the pointer of struct AudioSpectrum.
 * @param priority designed for future use, now please set 0.
 * @param session selects the stream to analyze, see AUDIO_SPECTRUM_SESSION_*.
 * For example AUDIO_SPECTRUM_SESSION_OUTPUT_PRIMARY feeds the final PCM of the
 * primary output thread to the analyzer.
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | the operation is invalid.
 * AUDIO_ERR_INVALID_PARAM | the params are invalid.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioSpectrum_Init(struct AudioSpectrum *spectrum, int32_t priority, int32_t session);

/**
 * @brief Set AudioSpectrum enable or disable.
 * When enabled, the audio stream layer feeds the bound session's PCM to the
 * analyzer. When disabled, analysis stops and the last result is kept.
 * @param spectrum is the pointer of struct AudioSpectrum.
 * @param enabled true means enable, false means disable.
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | the operation is invalid.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioSpectrum_SetEnabled(struct AudioSpectrum *spectrum, bool enabled);

/**
 * @brief Set the FFT size (capture size) of the analyzer.
 * This must be called before {@link AudioSpectrum_SetEnabled} is enabled.
 * The value should be a power of two, for example 512 or 1024.
 * @param spectrum is the pointer of struct AudioSpectrum.
 * @param size is the FFT size in samples.
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | the operation is invalid.
 * AUDIO_ERR_INVALID_PARAM | the params are invalid.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioSpectrum_SetCaptureSize(struct AudioSpectrum *spectrum, uint32_t size);

/**
 * @brief Get the FFT size (capture size) of the analyzer.
 * @param spectrum is the pointer of struct AudioSpectrum.
 * @return Returns the FFT size in samples, or a negative value on error.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioSpectrum_GetCaptureSize(struct AudioSpectrum *spectrum);

/**
 * @brief Get the sampling rate of the analyzed stream, in Hz.
 * @param spectrum is the pointer of struct AudioSpectrum.
 * @return Returns the sampling rate in Hz, or a negative value on error.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioSpectrum_GetSamplingRate(struct AudioSpectrum *spectrum);

/**
 * @brief Get the latest frequency-domain (FFT) data of the bound session.
 *
 * The output is the complex FFT of the analyzed mono signal, stored as
 * interleaved real/imaginary float pairs: fft[2*k] is the real part and
 * fft[2*k+1] is the imaginary part of bin k, for k in [0, GetCaptureSize()/2].
 * The caller must therefore provide a buffer of at least (GetCaptureSize() + 2)
 * floats.
 *
 * @param spectrum is the pointer of struct AudioSpectrum.
 * @param fft is the caller-owned output buffer receiving the FFT data.
 * @param float_count is the number of floats the fft buffer can hold; it should
 * be at least GetCaptureSize() + 2.
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | the operation is invalid.
 * AUDIO_ERR_INVALID_PARAM | the params are invalid.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioSpectrum_GetFft(struct AudioSpectrum *spectrum, float *fft, uint32_t float_count);

/** @} End of AudioSpectrum_Functions group */

#ifdef __cplusplus
}
#endif

/** @} */

#endif // AMEBA_AUDIO_INTERFACES_AUDIO_AUDIO_SPECTRUM_H
