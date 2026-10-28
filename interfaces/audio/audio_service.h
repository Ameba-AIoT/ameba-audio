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
 * @file audio_service.h
 *
 * @brief Provides APIs of the audio service.
 *
 * AudioService is the bring-up entry point for the audio framework. It
 * initialises the audio policy and the cross-stream coordinator that
 * AudioTrack / AudioRecord / AudioControl rely on, and it lets the
 * application notify the framework about device hot-plug events (e.g. USB
 * audio attached / detached, headphone inserted / removed).
 *
 * Typical bring-up:
 *   AudioService_Init();                                  // once at boot
 *   AudioService_SetDeviceState(DEVICE_OUT_USB,
 *                               AUDIO_DEVICE_STATE_AVAILABLE,
 *                               "usb_audio_0", &cfg);     // on hot-plug
 *   ... create AudioTrack / AudioRecord ...
 *
 * @since 1.0
 * @version 1.0
 */

#ifndef AMEBA_AUDIO_INTERFACES_AUDIO_AUDIO_SERVICE_H
#define AMEBA_AUDIO_INTERFACES_AUDIO_AUDIO_SERVICE_H

#include <stdint.h>

#include "audio_type.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup AudioService_Types AudioService Types
 * @{
 */

/**
 * @brief Connection state of an audio device known to the audio policy.
 *
 * Passed to AudioService_SetDeviceState() and returned by
 * AudioService_GetDeviceState() to report whether a routable endpoint
 * (speaker, headphone, USB, I2S, microphone, ...) is currently usable.
 *
 * @since 1.0
 * @version 1.0
 */
typedef enum AudioDeviceState {
    AUDIO_DEVICE_STATE_UNAVAILABLE,  ///< Device is not connected / not usable.
    AUDIO_DEVICE_STATE_AVAILABLE,    ///< Device is connected and ready for routing.
} AudioDeviceState;

/**
 * @brief PCM configuration advertised for a device when it becomes available.
 *
 * Supplied alongside AudioService_SetDeviceState() so the policy/HAL can
 * negotiate an appropriate streaming format for the newly attached endpoint
 * (most relevant for hot-pluggable devices such as USB audio). Fields that do
 * not apply to a particular device may be left zero; the policy will fall
 * back to the device's default capability.
 *
 * @since 1.0
 * @version 1.0
 */
typedef struct AudioDeviceConfig {
    int32_t rate;       ///< Sample rate in Hz (e.g. 16000, 44100, 48000).
    int32_t channels;   ///< Channel count (1 = mono, 2 = stereo, ...).
    int32_t format;     ///< PCM format, one of @c AUDIO_FORMAT_PCM_*.
} AudioDeviceConfig;

/** @} End of AudioService_Types group */

/**
 * @defgroup AudioService_Functions AudioService Functions
 * @{
 */

/**
 * @brief Initialise the audio service.
 *
 * Brings up the global AudioPolicy and the cross-stream coordinator. Must be
 * called once before any AudioTrack / AudioRecord / AudioControl /
 * AudioManager API is used. Re-invocation is safe and is a no-op once the
 * service is already up.
 *
 * @note  In the passthrough architecture this function only emits a log line;
 *        the policy is not used there.
 * @note  Call from the application boot thread, not from a callback or ISR.
 */
void AudioService_Init(void);

/**
 * @brief Notify the audio policy that a device became available or
 *        unavailable.
 *
 * Used to drive routing decisions in response to hot-plug or board events.
 * On AUDIO_DEVICE_STATE_AVAILABLE the policy registers the device, attaches
 * it to the matching hardware card and updates routing strategies; on
 * AUDIO_DEVICE_STATE_UNAVAILABLE it removes the device from the active set so
 * future tracks are routed elsewhere. Existing tracks may implicitly
 * migrated according to default device policy.
 *
 * @param[in] device       One of the @c DEVICE_OUT_* / @c DEVICE_IN_* values
 *                         from audio_type.h.
 * @param[in] state        New connection state, see @ref AudioDeviceState.
 * @param[in] device_name  Optional human-readable identifier used to
 *                         disambiguate multiple instances of the same device
 *                         class (e.g. several USB audio cards). May be NULL
 *                         if the device class is unique.
 * @param[in] config       Optional PCM configuration to advertise for the
 *                         device; may be NULL if the default capability is
 *                         acceptable.
 * @return 0 on success; negative OSAL error code on failure.
 * @retval OSAL_OK                     State updated.
 * @retval OSAL_ERR_INVALID_OPERATION  Device already in the requested state,
 *                                     or no hardware card backs @p device.
 * @retval OSAL_ERR_NO_MEMORY          Failed to register the device.
 * @note   Passthrough builds ignore the call and return 0.
 * @note   AudioService_Init() must have been called first.
 * @see    AudioService_GetDeviceState
 */
int32_t AudioService_SetDeviceState(int32_t device, AudioDeviceState state, const char *device_name, AudioDeviceConfig *config);

/**
 * @brief Query the current connection state of a device.
 *
 * @param[in] device  One of the @c DEVICE_OUT_* / @c DEVICE_IN_* values from
 *                    audio_type.h.
 * @return Current state of @p device.
 * @retval AUDIO_DEVICE_STATE_AVAILABLE    Device is connected.
 * @retval AUDIO_DEVICE_STATE_UNAVAILABLE  Device is not connected or unknown.
 * @note   Passthrough builds always return AUDIO_DEVICE_STATE_UNAVAILABLE.
 * @note   AudioService_Init() must have been called first.
 * @see    AudioService_SetDeviceState
 */
AudioDeviceState AudioService_GetDeviceState(int32_t device);

/** @} End of AudioService_Functions group */

#ifdef __cplusplus
}
#endif

/** @} */

#endif // AMEBA_AUDIO_INTERFACES_AUDIO_AUDIO_SERVICE_H
