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
 * @file audio_type.h
 *
 * @brief Provides definition of the audio stream types and formats.
 *
 *
 * @since 1.0
 * @version 1.0
 */

#ifndef AMEBA_AUDIO_INTERFACES_AUDIO_AUDIO_TYPE_H
#define AMEBA_AUDIO_INTERFACES_AUDIO_AUDIO_TYPE_H

#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup AudioType_Constants AudioType Constants
 * @{
 */

/**
 * @brief Defines all the audio playback usages.
 *
 * Pass to AudioTrack_SetCategory() before AudioTrack_Start().
 *
 * @since 1.0
 * @version 1.0
 */
enum {
    /** Category for general media playback. */
    AUDIO_CATEGORY_MEDIA         = 0,
    /** Category for voice calls. */
    AUDIO_CATEGORY_COMMUNICATION = 1,
    /** Category for text-to-speech / voice prompts. */
    AUDIO_CATEGORY_TTS           = 2,
    /** Category for short beep / key tone effects. */
    AUDIO_CATEGORY_BEEP          = 3,
    /** Total number of categories; do not pass as a real value. */
    AUDIO_CATEGORY_MAX_NUM       = 4,
};

/**
 * @brief Defines all the supported PCM sample formats.
 *
 * Each value describes the layout of **one sample on one channel**. A frame
 * contains channel_count samples laid out interleaved. Use
 * Audio_GetAudioBytesPerSample() to get the byte width of a sample.
 *
 * @since 1.0
 * @version 1.0
 */
enum {
    /** Invalid / unknown format, used as error sentinel. */
    AUDIO_FORMAT_INVALID           = 0xFFFFFFFFu,
    /** Signed 8-bit PCM, 1 byte per sample. */
    AUDIO_FORMAT_PCM_8_BIT         = 0x01u,
    /** Signed 16-bit PCM, 2 bytes per sample. Default format. */
    AUDIO_FORMAT_PCM_16_BIT        = 0x02u,
    /** Signed 32-bit PCM, 4 bytes per sample. */
    AUDIO_FORMAT_PCM_32_BIT        = 0x04u,
    /** 32-bit IEEE float PCM in [-1.0, 1.0], 4 bytes per sample. */
    AUDIO_FORMAT_PCM_FLOAT         = 0x08u,
    /** Signed 24-bit PCM packed (3 bytes per sample, no padding). */
    AUDIO_FORMAT_PCM_24_BIT        = 0x10u,
    /** 24-bit PCM stored in the high 24 bits of a 32-bit container, 4 bytes per sample. */
    AUDIO_FORMAT_PCM_8_24_BIT      = 0x20u,
};

/**
 * @brief Defines audio routing endpoints (devices) for output and input paths.
 *
 * Values are bitmask-friendly. Output devices occupy the low bits; input
 * devices set bit 27 (0x08000000) so a single uint32_t can carry direction
 * plus device. Use these with the audio control / patch APIs to select
 * routing.
 *
 * @since 1.0
 * @version 1.0
 */
enum {
    /** No device / unrouted. */
    DEVICE_NONE               = 0x0u,
    /** Built-in speaker output. */
    DEVICE_OUT_SPEAKER        = 0x1u,
    /** I2S output to an external codec / DSP. */
    DEVICE_OUT_I2S            = 0x2u,
    /** Wired headphone output. */
    DEVICE_OUT_HEADPHONE      = 0x4u,
    /** Bluetooth A2DP output. */
    DEVICE_OUT_A2DP           = 0x8u,
    /** USB audio output. */
    DEVICE_OUT_USB            = 0x10u,
    /** Analog microphone input (AMIC). */
    DEVICE_IN_MIC             = 0x8000001u,
    /** Digital microphone with analog mic reference (DMIC + AMIC ref). */
    DEVICE_IN_DMIC_REF_AMIC   = 0x8000002u,
    /** I2S capture input. */
    DEVICE_IN_I2S             = 0x8000004u,
};

/**
 * @brief Defines all the audio output flags.
 *
 * Bitmask passed at AudioTrack creation/configuration to choose the playback
 * path. NOIRQ selects a polling/no-DMA-IRQ low-latency path; otherwise the
 * default IRQ-driven path is used.
 *
 * @since 1.0
 * @version 1.0
 */
enum {
    /** Default output path (IRQ-driven DMA). */
    AUDIO_OUTPUT_FLAG_NONE         = 0x0u,
    /** Use the no-DMA-IRQ output path (lower latency, more CPU). */
    AUDIO_OUTPUT_FLAG_NOIRQ        = 0x1u,
};

/**
 * @brief Defines all the audio input flags.
 *
 * Bitmask passed at AudioRecord creation/configuration to choose the capture
 * path. NOIRQ selects a polling/no-DMA-IRQ low-latency path; otherwise the
 * default IRQ-driven path is used.
 *
 * @since 1.0
 * @version 1.0
 */
enum {
    /** Default input path (IRQ-driven DMA). */
    AUDIO_INPUT_FLAG_NONE         = 0x0u,
    /** Use the no-DMA-IRQ input path (lower latency). */
    AUDIO_INPUT_FLAG_NOIRQ        = 0x1u,
};

/**
 * @brief Defines all the audio effect types.
 *
 * @since 1.0
 * @version 1.0
 */
enum {
    /** invalid audio effect */
    AUDIO_EFFECT_INVALID           = 0xFFFFFFFFu,
    /** audio equalizer of audio effect */
    AUDIO_EFFECT_EQUALIZER         = 0x1u,
    /** audio spectrum analyzer of audio effect */
    AUDIO_EFFECT_SPECTRUM          = 0x2u,
};

/**
 * @brief Defines all the audio equalizer filter types.
 *
 * @since 1.0
 * @version 1.0
 */
enum {
    /** audio equalizer type: low pass */
    AUDIO_EQUALIZER_TYPE_LOWPASS      = 0x0u,
    /** audio equalizer type: high pass */
    AUDIO_EQUALIZER_TYPE_HIGHPASS     = 0x1u,
    /** audio equalizer type: band pass */
    AUDIO_EQUALIZER_TYPE_BANDPASS     = 0x2u,
    /** audio equalizer type: peaking */
    AUDIO_EQUALIZER_TYPE_PEAKING      = 0x3u,
    /** audio equalizer type: notch */
    AUDIO_EQUALIZER_TYPE_NOTCH        = 0x4u,
    /** audio equalizer type: low shelf */
    AUDIO_EQUALIZER_TYPE_LOW_SHELF    = 0x5u,
    /** audio equalizer type: high pass */
    AUDIO_EQUALIZER_TYPE_HIGH_SHELF   = 0x6u,
};

/**
 * @brief Defines all the audio effect param types.
 *
 * @since 1.0
 * @version 1.0
 */
enum {
    /** audio effect total bands */
    AUDIO_EFFECT_PARAM_NUM_BANDS       = 0x0u,
    /** audio effect level range */
    AUDIO_EFFECT_PARAM_LEVEL_RANGE     = 0x1u,
    /** audio effect band level */
    AUDIO_EFFECT_PARAM_BAND_LEVEL      = 0x2u,
    /** audio effect center frequency */
    AUDIO_EFFECT_PARAM_CENTER_FREQ     = 0x3u,
    /** audio effect band frequency range */
    AUDIO_EFFECT_PARAM_BAND_FREQ_RANGE = 0x4u,
    /** audio effect get band */
    AUDIO_EFFECT_PARAM_GET_BAND        = 0x5u,
    /** audio effect qfactor */
    AUDIO_EFFECT_PARAM_QFACTOR         = 0x6u,
    /** audio effect filter type */
    AUDIO_EFFECT_PARAM_FILTER_TYPE     = 0x7u,
};

/**
 * @brief Defines all the audio spectrum analyzer param types.
 *
 * @since 1.0
 * @version 1.0
 */
enum {
    /** spectrum FFT size (capture size), in samples */
    AUDIO_EFFECT_PARAM_SPECTRUM_CAPTURE_SIZE   = 0x0u,
    /** spectrum sampling rate of the analyzed stream, in Hz */
    AUDIO_EFFECT_PARAM_SPECTRUM_SAMPLING_RATE  = 0x1u,
    /** spectrum complex FFT data, interleaved real/imaginary floats */
    AUDIO_EFFECT_PARAM_SPECTRUM_FFT            = 0x2u,
};

/**
 * @brief Defines all the audio min frames stages, only for mixer to use.
 *
 * @since 1.0
 * @version 1.0
 */
enum {
    /** allows more data for service to write to HAL one time, default uing it */
    AUDIO_OUT_MIN_FRAMES_STAGE1  = 0,
    /** allows less data for service to write to HAL one time */
    AUDIO_OUT_MIN_FRAMES_STAGE2  = 1,
};

/**
 * @brief Defines the type of an audio patch endpoint node.
 *
 * An audio patch is a hardware-level routing connection between sources and
 * sinks. Each endpoint is either an internal port or an external
 * device. Used in AudioPatchConfig::type.
 *
 * @since 1.0
 * @version 1.0
 */
enum {
    /** Unset / invalid endpoint. */
    AUDIO_PATCH_NODE_NONE        = 0x0u,
    /** Endpoint is an internal port; AudioPatchConfig::node.port_index applies. */
    AUDIO_PATCH_NODE_PORT        = 0x1u,
    /** Endpoint is an external device; AudioPatchConfig::node.device applies. */
    AUDIO_PATCH_NODE_DEVICE      = 0x2u,
};

/** @} End of AudioType_Constants group */

/**
 * @defgroup AudioType_Types AudioType Types
 * @{
 */

/**
 * @brief Configuration of one source or sink endpoint of an audio patch.
 *
 * Pass arrays of these to AudioManager_CreateAudioPatch() to set up
 * source-to-sink hardware routing without going through the streaming path.
 * @c sample_rate / @c channel_count / @c format describe the PCM running on
 * that endpoint and must be supported by the underlying hardware. @c type
 * selects which member of @c node is meaningful.
 *
 * @since 1.0
 * @version 1.0
 */
struct AudioPatchConfig {
    /** Sample rate in Hz (e.g. 16000, 44100, 48000). */
    uint32_t sample_rate;
    /** Channel count (1 = mono, 2 = stereo, ...). */
    uint32_t channel_count;
    /** PCM format, one of @c AUDIO_FORMAT_PCM_*. */
    uint32_t format;
    /** Endpoint type: @c AUDIO_PATCH_NODE_PORT or @c AUDIO_PATCH_NODE_DEVICE. */
    uint32_t type;
    union {
        /** Port index when @c type == @c AUDIO_PATCH_NODE_PORT. */
        uint32_t  port_index;
        /** Device id (one of @c DEVICE_*) when @c type == @c AUDIO_PATCH_NODE_DEVICE. */
        uint32_t device;
    } node;
};

/**
 * @brief Returns the size in bytes of one PCM sample for a given format.
 *
 * @param[in] format One of the @c AUDIO_FORMAT_PCM_* values.
 * @return Sample size in bytes; 0 for @c AUDIO_FORMAT_INVALID or unknown values.
 * @note Multiply by channel_count to get the size of one frame.
 */
static inline size_t Audio_GetAudioBytesPerSample(int32_t format)
{
    size_t size = 0;

    switch (format) {
    case AUDIO_FORMAT_PCM_8_BIT:
        size = sizeof(uint8_t);
        break;
    case AUDIO_FORMAT_PCM_16_BIT:
        size = sizeof(int16_t);
        break;
    case AUDIO_FORMAT_PCM_24_BIT:
        size = sizeof(uint8_t) * 3;
        break;
    case AUDIO_FORMAT_PCM_8_24_BIT:
    case AUDIO_FORMAT_PCM_32_BIT:
        size = sizeof(int32_t);
        break;
    default:
        break;
    }
    return size;
}

/** @} End of AudioType_Types group */

#ifdef __cplusplus
}
#endif

/** @} */

#endif // AMEBA_AUDIO_INTERFACES_AUDIO_AUDIO_TYPE_H
