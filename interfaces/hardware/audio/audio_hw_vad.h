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
 * @addtogroup HAL
 * @{
 *
 * @brief HAL ops for the on-chip VAD (voice activity detection) hardware.
 *
 * @since 1.0
 * @version 1.0
 */

/**
 * @file audio_hw_vad.h
 *
 * @brief HAL surface that the AudioVad framework calls into. The HW VAD is a
 *        singleton — there is only one detector + SRAM ring on the chip — so
 *        the HAL exposes a single `GetAudioHwVad()` accessor instead of the
 *        AudioHwCard / CreateStreamIn factory chain that AudioRecord uses.
 *
 * @since 1.0
 * @version 1.0
 */

#ifndef AMEBA_AUDIO_INTERFACES_HARDWARE_AUDIO_AUDIO_HW_VAD_H
#define AMEBA_AUDIO_INTERFACES_HARDWARE_AUDIO_AUDIO_HW_VAD_H

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#include "hardware/audio/audio_hw_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief HW VAD configuration passed from the framework down into the HAL.
 *        Mirrors AudioVadConfig but uses HAL types where available.
 */
struct AudioHwVadConfig {
    enum AudioHwDevice device;
    uint32_t mic_index;
    /** Number of SRAM-buffered VAD channels (1..4); 0 defaults to 1. The wake
     *  source is always a single codec, but the ring can buffer up to 4. */
    uint32_t channel_count;
    uint32_t det_mv_threshold;
    uint32_t det_od_threshold;
};

/**
 * @brief HAL wake callback fired from the worker context (NOT from ISR). The
 *        ISR itself only does sema_give + flag set; the worker wakes up and
 *        invokes this callback so the upper layer can run user-defined work.
 */
typedef void (*AudioHwVadWakeCallback)(void *user);

/**
 * @brief HW VAD ops vtable. Implementations live in audio_hal/<platform>/.
 *
 *        Each function pointer mirrors one AudioVad framework call; the
 *        framework simply forwards. The HAL owns the IRQ, the wake-up sema and
 *        the SRAM ring details — the framework above never touches fwlib.
 */
struct AudioHwVad {
    /**
     * @brief Initialise hardware: codec LDO, AMIC bias, VAD ADC routing,
     *        IRQ registration, AP wake source. Does not enable detection.
     */
    int32_t (*Init)(struct AudioHwVad *vad, const struct AudioHwVadConfig *cfg);

    /**
     * @brief Enable the VAD detector. After this call the AP can sleep and the
     *        next utterance will fire VADBT_OR_VADPC IRQ.
     */
    int32_t (*Start)(struct AudioHwVad *vad);

    /**
     * @brief Disable the VAD detector. Symmetric to Start. Does not power off
     *        the codec or AMIC bias — that happens at Deinit.
     */
    int32_t (*Stop)(struct AudioHwVad *vad);

    /**
     * @brief Block until the next VAD wake-up.
     *
     * @param timeout_ms 0xFFFFFFFF for forever.
     * @return 0 on wake-up, < 0 on timeout.
     */
    int32_t (*WaitWakeup)(struct AudioHwVad *vad, uint32_t timeout_ms);

    /**
     * @brief Pull PCM from the SRAM ring via the platform driver's get_vad_data
     *        equivalent. First call after each wake locks ~400 ms pre-roll
     *        from `hit_addr`; subsequent calls walk forward (and may busy-wait
     *        for live data), so callers should request small per-frame buffers.
     */
    int32_t (*ReadLookback)(struct AudioHwVad *vad, void *buffer, size_t buffer_bytes);

    /**
     * @brief Switch ADC clock from the VAD codec path to the audio codec path
     *        so an AudioRecord stream can be opened.
     */
    int32_t (*PrepareForRecord)(struct AudioHwVad *vad);

    /**
     * @brief Restore codec LDO / AMIC bias / clocks / VAD ADC routing / VAD IRQ
     *        / VADMEM clock, clear pitch-detect latch and release the wakelock.
     *        Idempotent and safe whether or not AudioRecord actually ran.
     */
    int32_t (*Rearm)(struct AudioHwVad *vad);

    /**
     * @brief Free resources. Disables IRQ, powers down codec / AMIC.
     */
    void (*Deinit)(struct AudioHwVad *vad);

    /**
     * @brief Register a worker-context wake callback. May be NULL.
     */
    int32_t (*RegisterCallback)(struct AudioHwVad *vad, AudioHwVadWakeCallback cb, void *user);

    /**
     * @brief Set runtime tuning parameters. See audio_vad.h for the recognised
     *        key=val syntax.
     */
    int32_t (*SetParameters)(struct AudioHwVad *vad, const char *strs);

    /**
     * @brief Address inside the SRAM ring where the latest wake-up triggered.
     */
    uint32_t (*GetHitAddress)(struct AudioHwVad *vad);
};

/**
 * @brief Singleton accessor. Returns the platform's AudioHwVad instance,
 *        creating it on first call. Returns NULL if HW VAD is unsupported on
 *        the current platform.
 */
struct AudioHwVad *GetAudioHwVad(void);

/**
 * @brief Drop a reference obtained via GetAudioHwVad. The actual instance is
 *        not freed (it's a singleton); this exists so future implementations
 *        can refcount if needed.
 */
void DestroyAudioHwVad(struct AudioHwVad *vad);

#ifdef __cplusplus
}
#endif

#endif  // AMEBA_AUDIO_INTERFACES_HARDWARE_AUDIO_AUDIO_HW_VAD_H
/** @} */
