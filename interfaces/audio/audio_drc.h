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
 * @file audio_drc.h
 *
 * @brief Provides APIs of the audio DRC (Dynamic Range Compression) effect.
 *
 * DRC sits in the render effect chain right after the equalizer and right
 * before the spectrum analyzer, so the analyzer observes the fully processed
 * post-DRC PCM. Two modes are provided:
 *   - single-band DRC (default): one broadband compressor with up to four
 *     knee points, per-attack/release time, makeup gain;
 *   - three-band DRC: splits the signal into low / mid / high bands using
 *     two crossover frequencies and applies an independent compressor to
 *     each band.
 *
 * @since 1.0
 * @version 1.0
 */

#ifndef AMEBA_AUDIO_INTERFACES_AUDIO_AUDIO_DRC_H
#define AMEBA_AUDIO_INTERFACES_AUDIO_AUDIO_DRC_H

#include <stdint.h>
#include <stdbool.h>

#include "audio/audio_type.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup AudioDrc_Types AudioDrc Types
 * @{
 */

struct AudioDrc;

/** Maximum number of knee points a single DRC band supports. */
#define AUDIO_DRC_KNEE_POINT_NUM_MAX  4

/**
 * @brief One knee point of a DRC transfer curve.
 *
 * The struct layout intentionally matches the underlying DSP library's
 * ::knee_point_t so the framework can pass the config blob through without
 * per-field marshalling.
 *
 * @since 1.0
 * @version 1.0
 */
struct AudioDrcKneePoint {
    /** threshold (dB) */
    float threshold_db;
    /** ratio */
    float ratio;
};

/**
 * @brief Single-band DRC config.
 *
 * @since 1.0
 * @version 1.0
 */
struct AudioDrcConfig {
    /** number of active knee points, range [1, AUDIO_DRC_KNEE_POINT_NUM_MAX] */
    int32_t num_of_knee_point;
    /** attack time (sec) */
    float attack_time;
    /** release time (sec) */
    float release_time;
    /** makeup gain (dB) */
    float makeup_gain;
    /** knee points of DRC */
    struct AudioDrcKneePoint knee_point[AUDIO_DRC_KNEE_POINT_NUM_MAX];
};

/**
 * @brief Crossover frequencies for three-band DRC.
 *
 * @since 1.0
 * @version 1.0
 */
struct AudioDrcSplitFreq {
    /** low band upper crossover (Hz) */
    int32_t fc_lo;
    /** high band lower crossover (Hz) */
    int32_t fc_hi;
};

/**
 * @brief Three-band DRC config.
 *
 * @since 1.0
 * @version 1.0
 */
struct AudioMultiBandsDrcConfig {
    /** crossover frequencies dividing low/mid/high bands */
    struct AudioDrcSplitFreq split_freq;
    /** compressor config for the low band */
    struct AudioDrcConfig drc_band_low;
    /** compressor config for the mid band */
    struct AudioDrcConfig drc_band_mid;
    /** compressor config for the high band */
    struct AudioDrcConfig drc_band_high;
};

/**
 * @brief Defines the DRC processing modes.
 *
 * @since 1.0
 * @version 1.0
 */
enum {
    /** one broadband compressor (default). */
    AUDIO_DRC_MODE_SINGLE_BAND = 0x0,
    /** three-band compressor (low / mid / high). */
    AUDIO_DRC_MODE_MULTI_BANDS = 0x1,
};

/** @} End of AudioDrc_Types group */

/**
 * @defgroup AudioDrc_Functions AudioDrc Functions
 * @{
 */

/**
 * @brief Create AudioDrc instance.
 * @return Returns the instance pointer of AudioDrc.
 * @since 1.0
 * @version 1.0
 */
struct AudioDrc *AudioDrc_Create(void);

/**
 * @brief Release AudioDrc.
 * @param drc is the pointer of struct AudioDrc.
 * @since 1.0
 * @version 1.0
 */
void AudioDrc_Destroy(struct AudioDrc *drc);

/**
 * @brief Init AudioDrc and attach it to an audio session.
 *
 * DRC is inserted into the render effect chain between the equalizer and the
 * spectrum analyzer, so it always operates on the post-equalizer PCM and its
 * output is what the spectrum analyzer will see.
 *
 * @param drc is the pointer of struct AudioDrc.
 * @param priority designed for future use, now please set 0.
 * @param session designed for future use, now please set 0.
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | the operation is invalid.
 * AUDIO_ERR_INVALID_PARAM | the params are invalid.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioDrc_Init(struct AudioDrc *drc, int32_t priority, int32_t session);

/**
 * @brief Set AudioDrc enable or disable.
 *
 * When disabled the DRC step is bypassed and the equalizer output goes
 * straight to the spectrum analyzer / HAL.
 *
 * @param drc is the pointer of struct AudioDrc.
 * @param enabled true means enable, false means disable.
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | the operation is invalid.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioDrc_SetEnabled(struct AudioDrc *drc, bool enabled);

/**
 * @brief Select single-band or three-band DRC.
 *
 * Must be called before {@link AudioDrc_SetEnabled} enables the effect.
 * After enabling, mode is fixed and this call is rejected.
 *
 * @param drc is the pointer of struct AudioDrc.
 * @param mode one of AUDIO_DRC_MODE_SINGLE_BAND or AUDIO_DRC_MODE_MULTI_BANDS.
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | the operation is invalid.
 * AUDIO_ERR_INVALID_PARAM | the params are invalid.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioDrc_SetMode(struct AudioDrc *drc, int32_t mode);

/**
 * @brief Apply a full single-band DRC config.
 *
 * Only meaningful when the DRC mode is AUDIO_DRC_MODE_SINGLE_BAND.
 *
 * @param drc is the pointer of struct AudioDrc.
 * @param config is the single-band DRC config to apply. Must not be NULL.
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | the operation is invalid.
 * AUDIO_ERR_INVALID_PARAM | the params are invalid.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioDrc_SetConfig(struct AudioDrc *drc, const struct AudioDrcConfig *config);

/**
 * @brief Apply a full three-band DRC config.
 *
 * Only meaningful when the DRC mode is AUDIO_DRC_MODE_MULTI_BANDS.
 *
 * @param drc is the pointer of struct AudioDrc.
 * @param config is the three-band DRC config to apply. Must not be NULL.
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | the operation is invalid.
 * AUDIO_ERR_INVALID_PARAM | the params are invalid.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioDrc_SetMultiBandsConfig(struct AudioDrc *drc, const struct AudioMultiBandsDrcConfig *config);

/** @} End of AudioDrc_Functions group */

#ifdef __cplusplus
}
#endif

/** @} */

#endif // AMEBA_AUDIO_INTERFACES_AUDIO_AUDIO_DRC_H
