/*
 * Copyright (c) 2021 Realtek, LLC.
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
 * @file audio_equalizer.h
 *
 * @brief Provides APIs of the audio equalizer.
 *
 *
 * @since 1.0
 * @version 1.0
 */

#ifndef AMEBA_AUDIO_INTERFACES_AUDIO_AUDIO_EQUALIZER_H
#define AMEBA_AUDIO_INTERFACES_AUDIO_AUDIO_EQUALIZER_H

#include <stdint.h>
#include <stdbool.h>

#include "audio/audio_type.h"

#ifdef __cplusplus
extern "C" {
#endif

struct AudioEqualizer;
struct AudioEffect;

/**
 * @brief Create AudioEqualizer instance.
 * @return Returns the instance pointer of AudioEqualizer.
 * @since 1.0
 * @version 1.0
 */
struct AudioEqualizer *AudioEqualizer_Create(void);

/**
 * @brief Release AudioEqualizer.
 * @param equalizer is the pointer of struct AudioEqualizer.
 * @since 1.0
 * @version 1.0
 */
void AudioEqualizer_Destroy(struct AudioEqualizer *equalizer);

/**
 * @brief Init AudioEqualizer.
 * @param equalizer is the pointer of struct AudioEqualizer.
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
int32_t AudioEqualizer_Init(struct AudioEqualizer *equalizer, int32_t priority, int32_t session);

/**
 * @brief Set AudioEqualizer enable or disable.
 * @param equalizer is the pointer of struct AudioEqualizer.
 * @param enabled is the state of struct AudioEqualizer, true means enable, false means disable.
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | the operation is invalid.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioEqualizer_SetEnabled(struct AudioEqualizer *equalizer, bool enabled);

/**
 * @brief Get number of bands AudioEqualizer supports.
 * @param equalizer is the pointer of struct AudioEqualizer.
 * @param bands is the total bands.
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | the operation is invalid.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioEqualizer_SetNumberOfBands(struct AudioEqualizer *equalizer, uint32_t bands);

/**
 * @brief Get number of bands AudioEqualizer supports.
 * @param equalizer is the pointer of struct AudioEqualizer.
 * @return Returns the number of bands supported by the AudioEqualizer framework.
 * @since 1.0
 * @version 1.0
 */
int16_t AudioEqualizer_GetNumberOfBands(struct AudioEqualizer *equalizer);

/**
 * @brief Get band level range AudioEqualizer supports.
 *        This api only supports SW_EQ_VERSION_0_0 now.
 * @param equalizer is the pointer of struct AudioEqualizer.
 * @return Returns band level range supported by the AudioEqualizer framework.The return value
 * contains two int16_t integer, which is decibel * 100, for example (-1500, 1500), means (-15db, 15db).
 * @since 1.0
 * @version 1.0
 */
int16_t *AudioEqualizer_GetBandLevelRange(struct AudioEqualizer *equalizer);

/**
 * @brief Set AudioEqualizer band filter type.
 * This interface is only supported when kEqVersion = SW_EQ_VERSION_1_0, which is set in:
 * component/audio/configs/ameba_audio_mixer_usrcfg.c
 *
 * @param equalizer is the pointer of struct AudioEqualizer.
 * @param band is the band number, for example, which band level to set?
 * The band value range is [0, BAND_TATAL_NUM).BAND_TATAL_NUM is got from AudioEqualizer_GetNumberOfBands.
 * @param filter_type is the filter type of band to set, such as: AUDIO_EQUALIZER_TYPE_LOWPASS and so on,
 * which are defined in audio_type.h
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | the operation is invalid.
 * AUDIO_ERR_INVALID_PARAM | the params are invalid.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioEqualizer_SetBandFilterType(struct AudioEqualizer *equalizer, uint32_t band, uint32_t filter_type);

/**
 * @brief Get AudioEqualizer band filter type.
 * @param equalizer is the pointer of struct AudioEqualizer.
 * @param band is the band number, which band level to get?
 * The band value range is [0, BAND_TATAL_NUM).BAND_TATAL_NUM is got from AudioEqualizer_GetNumberOfBands.
 * @return Returns filter type, such as AUDIO_EQUALIZER_TYPE_LOWPASS and so on,
 * which are defined in audio_type.h
 * @since 1.0
 * @version 1.0
 */
int16_t AudioEqualizer_GetBandFilterType(struct AudioEqualizer *equalizer, uint32_t band);

/**
 * @brief Set AudioEqualizer band level.
 * @param equalizer is the pointer of struct AudioEqualizer.
 * @param band is the band number, which band level to set?
 * The band value range is [0, BAND_TATAL_NUM).BAND_TATAL_NUM is got from AudioEqualizer_GetNumberOfBands.
 * @param level is the level of band to set. The level value should be delta decibel * 100, and in the range framework
 * supports.See the {@link AudioEqualizer_GetBandLevelRange} for information
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | the operation is invalid.
 * AUDIO_ERR_INVALID_PARAM | the params are invalid.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioEqualizer_SetBandLevel(struct AudioEqualizer *equalizer, uint32_t band, uint32_t level);

/**
 * @brief Get AudioEqualizer band level.
 * @param equalizer is the pointer of struct AudioEqualizer.
 * @param band is the band number, which band level to get?
 * The band value range is [0, BAND_TATAL_NUM).BAND_TATAL_NUM is got from AudioEqualizer_GetNumberOfBands.
 * @return Returns a value of delta decibel * 100, and in the range framework supports.See the {@link
 * AudioEqualizer_GetBandLevelRange} for information
 * @since 1.0
 * @version 1.0
 */
int16_t AudioEqualizer_GetBandLevel(struct AudioEqualizer *equalizer, uint32_t band);

/**
 * @brief Set AudioEqualizer center frequency.
 * @param equalizer is the pointer of struct AudioEqualizer.
 * @param band is the band number.
 * @param freq is the new center frequency set to band in hz.
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | the operation is invalid.
 * AUDIO_ERR_INVALID_PARAM | the params are invalid.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioEqualizer_SetCenterFreq(struct AudioEqualizer *equalizer, uint32_t band, uint32_t freq);

/**
 * @brief Get AudioEqualizer center frequency.
 * @param equalizer is the pointer of struct AudioEqualizer.
 * @param band is the band number, which band frequency to get?
 * The band value range is [0, BAND_TATAL_NUM).BAND_TATAL_NUM is got from AudioEqualizer_GetNumberOfBands.
 * @return Returns a value of center frequency in hz.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioEqualizer_GetCenterFreq(struct AudioEqualizer *equalizer, uint32_t band);

/**
 * @brief Get AudioEqualizer frequency's band.
 * @param equalizer is the pointer of struct AudioEqualizer.
 * @param frequency is the frequency value.
 * @return Returns a value of band.
 * @since 1.0
 * @version 1.0
 */
int16_t AudioEqualizer_GetBand(struct AudioEqualizer *equalizer, uint32_t frequency);

/**
 * @brief Set AudioEqualizer Q factor.
 * @param equalizer is the pointer of struct AudioEqualizer.
 * @param band is the band number.
 * @param qfactor is the qfactor set to band, which equals to real qfactor * 100.
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | the operation is invalid.
 * AUDIO_ERR_INVALID_PARAM | the params are invalid.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioEqualizer_SetQfactor(struct AudioEqualizer *equalizer, uint32_t band, uint32_t qfactor);

/**
 * @brief Get AudioEqualizer Q factor.
 * @param equalizer is the pointer of struct AudioEqualizer.
 * @param band is the band number.
 * @return Returns qfactor of the band, which equals to real qfactor * 100.
 * @since 1.0
 * @version 1.0
 */
int16_t AudioEqualizer_GetQfactor(struct AudioEqualizer *equalizer, uint32_t band);

#ifdef __cplusplus
}
#endif

#endif