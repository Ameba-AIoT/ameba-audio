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
 * @brief Declares APIs for the audio VAD (Voice Activity Detection) framework.
 *
 * @since 1.0
 * @version 1.0
 */

/**
 * @file audio_vad.h
 *
 * @brief Hardware VAD wakeup framework. The HW VAD is a singleton resource
 * (one detector, one SRAM ring, one ISR) so the framework exposes a singleton
 * API — no Create/Destroy, no opaque handle. Lifecycle is just Init/Deinit:
 *
 *   - The chip keeps a SRAM ring buffer running while the AP is asleep, so on
 *     wake-up there is up to 1 s of pre-roll PCM that triggered the detection.
 *   - AudioRecord/SPORT/GDMA are powered down during sleep, so KWS must run on
 *     the lookback buffer first; AudioRecord can only be started afterwards.
 *
 * Lifecycle:
 *
 *   AudioVad_Init(&cfg)
 *   AudioVad_RegisterCallback(cb, user)
 *   AudioVad_Start()                         // arm HW VAD as wake source
 *   for (;;) {
 *       AudioVad_WaitWakeup(timeout)         // blocks until VAD IRQ fires
 *       AudioVad_ReadLookback(buf, n)        // 1 s of pre-roll PCM (Stage 1)
 *       if (kws_hit) {
 *           AudioVad_PrepareForRecord()      // switch ADC clock to codec
 *           // AudioRecord_Start ... read ... feed ASR ... AudioRecord_Stop
 *       }
 *       AudioVad_Rearm()     // reset latches, restore codec/AMIC, sleep again
 *   }
 *   AudioVad_Stop()
 *   AudioVad_Deinit()
 *
 * @since 1.0
 * @version 1.0
 */

#ifndef AMEBA_AUDIO_INTERFACES_AUDIO_AUDIO_VAD_H
#define AMEBA_AUDIO_INTERFACES_AUDIO_AUDIO_VAD_H

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#include "audio/audio_type.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AudioVad configuration. Defaults (when a field is 0) target a single
 *        AMIC1 channel running at 16 kHz / 16-bit / mono with a 1 s lookback
 *        window — the HW VAD's only supported configuration on amebasmart.
 *
 * @since 1.0
 * @version 1.0
 */
typedef struct {
    /** Audio device, supports DEVICE_IN_MIC for AMIC, DEVICE_IN_DMIC for DMIC. */
    uint32_t device;
    /**
     * Mic index within the selected device type. For channel_count > 1 this is
     * the index of channel 0; subsequent channels take the next mic in order.
     * AMIC (DEVICE_IN_MIC): 1..4, selects the bias GPIO (PA30/PA31/PB0/PB1).
     * DMIC (DEVICE_IN_DMIC_*): 0-based pair index.
     * 0 = framework default (1 for AMIC).
     */
    uint32_t mic_index;
    /**
     * Number of channels buffered into the VAD SRAM ring, range 1..4 (0 = 1).
     * The HW VAD always wakes on a SINGLE source (codec 0), but it can buffer
     * up to four codecs into SRAM blocks A/B/C/D simultaneously. With
     * channel_count > 1, AudioVad_ReadLookback returns the channels interleaved
     * (ch0,ch1,..,ch0,ch1,..) and each millisecond occupies 32 * channel_count
     * bytes. Channel i is sourced from mic (mic_index + i).
     */
    uint32_t channel_count;
    /** Set the threshold of majority vote to determine whether the speech features appear range(0-31). */
    uint32_t det_mv_threshold;
    /** Set the default threshold of onset detection(0-31). */
    uint32_t det_od_threshold;
} AudioVadConfig;

/**
 * @brief Wake event delivered to AudioVadCallback.
 */
typedef enum {
    /** HW VAD detected voice activity and woke the AP. Fires once per wake. */
    AUDIO_VAD_EVENT_WAKEUP = 0,
} AudioVadEvent;

/**
 * @brief VAD wake callback. Called from the framework's worker context (NOT from
 *        ISR), so it is safe to take semaphores, allocate, log etc. The callback
 *        only signals; it must not call AudioVad_ReadLookback /
 *        AudioVad_PrepareForRecord / AudioVad_Rearm itself — those belong on the
 *        application thread that called AudioVad_WaitWakeup.
 */
typedef void (*AudioVadCallback)(AudioVadEvent event, void *user);

/**
 * @brief Init the singleton AudioVad with the given config. Brings the codec /
 *        AMIC bias / VAD ADC routing online, but does NOT enable the VAD
 *        detector yet — call AudioVad_Start() for that. The HW VAD is a
 *        singleton resource, so a second Init() call before Deinit() returns
 *        AUDIO_ERR_INVALID_OPERATION.
 *
 * @param config is the pointer of AudioVadConfig used to configure HW VAD.
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_PARAM | config is NULL.
 * AUDIO_ERR_INVALID_OPERATION | the HW VAD is already initialised.
 * AUDIO_ERR_NOT_SUPPORT | the HW VAD is unavailable on this platform.
 * AUDIO_ERR_NO_MEMORY | memory allocation failed.
 */
int32_t AudioVad_Init(const AudioVadConfig *config);

/**
 * @brief Tear down the singleton AudioVad. Implicitly stops the VAD HW if it
 *        was started. Safe to call when not initialised (no-op).
 */
void AudioVad_Deinit(void);

/**
 * @brief Arm HW VAD. Routes VADBT_OR_VADPC as an AP wake source and switches the
 *        VADMEM clock to the VAD source so the SRAM ring keeps running across
 *        sleep.
 *
 * @return AUDIO_OK on success.
 */
int32_t AudioVad_Start(void);

/**
 * @brief Disarm HW VAD. Disables the VAD interrupt and stops the detector. Does
 *        NOT power down the codec / AMIC; that happens at AudioVad_Deinit.
 *
 * @return AUDIO_OK on success.
 */
int32_t AudioVad_Stop(void);

/**
 * @brief Register a wake-up callback. The callback fires from the framework's
 *        worker context after the VAD ISR has been promoted via semaphore.
 *
 * @param callback wake-up callback, may be NULL to clear a previous registration.
 * @param user opaque user pointer passed to the callback.
 * @return AUDIO_OK on success.
 */
int32_t AudioVad_RegisterCallback(AudioVadCallback callback, void *user);

/**
 * @brief Block until the next VAD wake-up. Returns immediately if a wake-up has
 *        already been pending since the last AudioVad_Rearm().
 *
 * @param timeout_ms 0xFFFFFFFF for forever, otherwise milliseconds.
 * @return AUDIO_OK on wake-up, AUDIO_ERR_INVALID_OPERATION on timeout.
 */
int32_t AudioVad_WaitWakeup(uint32_t timeout_ms);

/**
 * @brief Pull PCM out of the VAD SRAM ring for Stage 1 (KWS). Delegates to the
 *        fwlib `get_vad_data` driver, which owns the per-channel read pointers
 *        and ring-wrap logic. Semantics:
 *
 *          - The first call after each wake-up locks the start address at
 *            `hit_addr - PRE_READ_NUM_BLOCK` (~400 ms pre-roll, defined by
 *            fwlib). The driver picks this up via `first_time_flag`, which the
 *            VAD IRQ has already set to 1.
 *          - Subsequent calls walk the read pointer forward, busy-waiting on
 *            the live ADC stream when more data is requested than has landed
 *            yet — so a small per-frame buffer (e.g. one 16 ms / 512 B AFE
 *            frame) is the recommended call shape, not a single big slurp.
 *
 *        Internally switches the VADMEM clock to the platform source so the AP
 *        can read SRAM. The clock is restored by AudioVad_Rearm().
 *
 * @param buffer destination buffer for PCM data (16-bit LE). With
 *        channel_count > 1 the channels are interleaved (ch0,ch1,..,ch0,ch1,..).
 * @param buffer_bytes destination size in bytes (rounded down to whole ms;
 *        32 * channel_count B/ms @ 16k/16-bit).
 * @return number of bytes copied on success, < 0 on error, 0 if `buffer_bytes`
 *         was smaller than 1 ms worth of samples.
 */
int32_t AudioVad_ReadLookback(void *buffer, size_t buffer_bytes);

/**
 * @brief Switch the ADC clock from the VAD codec path to the regular audio
 *        codec path so AudioRecord can be brought up. Call between Stage 1
 *        (KWS hit) and AudioRecord_Start().
 *
 * @return AUDIO_OK on success.
 */
int32_t AudioVad_PrepareForRecord(void);

/**
 * @brief Re-arm HW VAD after one wake-up cycle. Restores codec LDO / AMIC bias /
 *        AC + AUDIO peripheral clocks (AudioRecord_Destroy may have refcounted
 *        these back to off), redoes the VAD ADC routing, clears the pitch-detect
 *        latch, re-enables VAD + IRQ, switches VADMEM clock back to the VAD
 *        source, and releases the wakelock so the AP can re-enter sleep.
 *
 *        Safe to call whether or not AudioRecord ran — the false-wake path uses
 *        the same recovery sequence.
 *
 * @return AUDIO_OK on success.
 */
int32_t AudioVad_Rearm(void);

/**
 * @brief Set tuning parameters at runtime. Format follows the AudioRecord
 *        SetParameters convention: "key=val;key=val;...". Recognised keys:
 *
 *          det_mv_threshold=N
 *          det_od_threshold=N
 *
 * @param strs parameter string.
 * @return AUDIO_OK on success.
 */
int32_t AudioVad_SetParameters(const char *strs);

/**
 * @brief Returns the address inside the SRAM ring at which the most recent
 *        wake-up was triggered. Useful for logging / diagnostics; the lookback
 *        path uses the live write pointer, NOT this hit address.
 *
 * @return hit address (0 if no wake-up has happened yet).
 */
uint32_t AudioVad_GetHitAddress(void);

#ifdef __cplusplus
}
#endif

/** @} */

#endif  // AMEBA_AUDIO_INTERFACES_AUDIO_AUDIO_VAD_H
