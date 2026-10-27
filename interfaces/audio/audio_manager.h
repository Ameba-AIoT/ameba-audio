/*
 * Copyright (c) 2024 Realtek, LLC.
 * All rights reserved.
 *
 * Licensed under the Realtek License, Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License from PanKore
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
 * @file audio_manager.h
 *
 * @brief Provides APIs of the audio manager.
 *
 * AudioManager is a process-wide singleton that owns the primary audio
 * hardware card and exposes hardware-level routing (audio patch) between
 * sources and sinks without going through the streaming (AudioTrack /
 * AudioRecord) path. It is only meaningful in the passthrough architecture;
 * in the mixer architecture all functions are stubs and
 * AudioManager_GetInstance() returns NULL.
 *
 * Typical lifecycle:
 *   AudioManager_GetInstance()  // first call creates the singleton
 *   AudioManager_CreateAudioPatch(...)
 *   ... use the patch ...
 *   AudioManager_ReleaseAudioPatch(...)
 *   AudioManager_Destroy()      // last user tears the singleton down
 *
 * @since 1.0
 * @version 1.0
 */

#ifndef AMEBA_AUDIO_INTERFACES_AUDIO_AUDIO_MANAGER_H
#define AMEBA_AUDIO_INTERFACES_AUDIO_AUDIO_MANAGER_H

#include <stdint.h>

#include "audio/audio_type.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup AudioManager_Functions AudioManager Functions
 * @{
 */

/**
 * @brief Get (and lazily create) the AudioManager singleton.
 *
 * The first successful call allocates the singleton, brings up the underlying
 * AudioHwManager and opens the primary hardware card. Subsequent calls return
 * the same pointer without re-initialising. Creation is guarded by an atomic
 * CAS so concurrent first-callers see one instance.
 *
 * Pair every successful @c AudioManager_GetInstance() with one
 * @c AudioManager_Destroy() at shutdown.
 *
 * @return Pointer to the singleton on success.
 * @retval NULL Allocation failed, no primary hardware card was found, or the
 *              current build uses the mixer architecture (which does not
 *              support AudioManager).
 * @note  Thread-safe.
 * @see   AudioManager_Destroy
 */
struct AudioManager *AudioManager_GetInstance(void);

/**
 * @brief Release the AudioManager singleton.
 *
 * Closes the primary hardware card, tears down the AudioHwManager and frees
 * the singleton. Safe to call when no singleton exists. Any patches created
 * via AudioManager_CreateAudioPatch() should be released first; otherwise the
 * underlying hardware routing may remain active until the card is reopened.
 *
 * @note  Thread-safe; the destroy path is guarded by the same atomic flag as
 *        AudioManager_GetInstance().
 * @see   AudioManager_GetInstance
 */
void AudioManager_Destroy(void);

/**
 * @brief Create a hardware audio patch between sources and sinks.
 *
 * An audio patch is a direct hardware routing between one or more source
 * endpoints and one or more sink endpoints (devices or internal ports). It
 * bypasses the streaming pipeline and is intended for board-level passthrough
 * scenarios such as I2S-IN -> I2S-OUT loopback.
 *
 * The endpoints in @p sources and @p sinks must agree on a sample rate /
 * channel count / format that the underlying hardware supports. Each
 * AudioPatchConfig::type selects which member of @c node is read.
 *
 * @param[in] manager      AudioManager singleton returned by
 *                         AudioManager_GetInstance(); must not be NULL.
 * @param[in] num_sources  Number of entries in @p sources.
 * @param[in] sources      Array of @p num_sources source endpoint configs.
 * @param[in] num_sinks    Number of entries in @p sinks.
 * @param[in] sinks        Array of @p num_sinks sink endpoint configs.
 * @return Non-negative patch index on success; negative OSAL error code on
 *         failure.
 * @retval >=0                          Patch index, pass to
 *                                      AudioManager_ReleaseAudioPatch().
 * @retval OSAL_ERR_NO_INIT             @p manager is NULL or not the active
 *                                      singleton.
 * @retval OSAL_ERR_INVALID_OPERATION   Endpoint configuration not supported
 *                                      by the hardware.
 * @note  Only supported on amebadplus (I2S-IN -> I2S-OUT) in the passthrough
 *        architecture; on the mixer build this function is a stub that
 *        returns 0 and logs an error.
 * @see   AudioManager_ReleaseAudioPatch, AudioPatchConfig
 */
int32_t AudioManager_CreateAudioPatch(struct AudioManager *manager,
                                        uint32_t num_sources, struct AudioPatchConfig *sources,
                                        uint32_t num_sinks, struct AudioPatchConfig *sinks);

/**
 * @brief Release a hardware audio patch previously created by
 *        AudioManager_CreateAudioPatch().
 *
 * Tears down the hardware routing associated with @p patch_index. The index
 * becomes invalid after this call; do not pass it again.
 *
 * @param[in] manager      AudioManager singleton; must not be NULL.
 * @param[in] patch_index  Index returned by AudioManager_CreateAudioPatch().
 * @return Result of the release operation.
 * @retval AUDIO_OK                     The patch was released successfully.
 * @retval AUDIO_ERR_NO_INIT            @p manager is NULL or not the active
 *                                      singleton.
 * @retval AUDIO_ERR_INVALID_OPERATION  @p patch_index is unknown or already
 *                                      released.
 * @note  On the mixer build this function is a stub that returns 0.
 * @see   AudioManager_CreateAudioPatch
 */
int32_t AudioManager_ReleaseAudioPatch(struct AudioManager *manager, int32_t patch_index);

/** @} End of AudioManager_Functions group */

#ifdef __cplusplus
}
#endif

/** @} */

#endif
