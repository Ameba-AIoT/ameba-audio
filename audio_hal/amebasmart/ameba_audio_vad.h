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
 * @file ameba_audio_vad.h
 * @brief amebasmart low-level VAD platform API (mirrors the
 *        ameba_audio_stream_capture.h style). Only `primary_audio_hw_vad.c`
 *        is meant to call into this — callers above the HAL must go through
 *        the `AudioHwVad` vtable in `interfaces/hardware/audio/audio_hw_vad.h`.
 *
 *        This file owns the fwlib touch points: codec / AMIC bring-up, VAD
 *        ADC routing, IRQ handler + wake semaphore, SRAM ring readback and
 *        the 8-step rearm. No vtable plumbing here.
 */

#ifndef AMEBA_AUDIO_AUDIO_HAL_AMEBASMART_AMEBA_AUDIO_VAD_H
#define AMEBA_AUDIO_AUDIO_HAL_AMEBASMART_AMEBA_AUDIO_VAD_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct AmebaAudioVad;

struct AmebaAudioVadConfig {
    uint32_t mic_index;
    /**
     * Number of SRAM-buffered VAD channels (1..4). The HW VAD has a single wake
     * source (codec 0 only), but it can simultaneously buffer up to four codecs
     * into blocks A/B/C/D. channel_count > 1 brings up extra codecs/mics and
     * makes read_lookback return all channels interleaved. 0 defaults to 1.
     */
    uint32_t channel_count;
    bool     is_dmic;     /* true = DMIC path; false = AMIC path (default) */
    uint32_t det_mv_threshold;
    uint32_t det_od_threshold;
};

/**
 * @brief Wake callback fired from worker context (NOT ISR) right after
 *        `ameba_audio_vad_wait_wakeup` returns successfully.
 */
typedef void (*AmebaAudioVadWakeCb)(void *user);

/**
 * @brief Allocate, init clocks/codec/AMIC/VAD ADC, register IRQ. Detection
 *        is NOT enabled yet — call `ameba_audio_vad_arm` for that.
 */
struct AmebaAudioVad *ameba_audio_vad_open(const struct AmebaAudioVadConfig *cfg);

/**
 * @brief Disarm if running, free resources.
 */
void ameba_audio_vad_close(struct AmebaAudioVad *self);

/**
 * @brief Enable VAD detector + IRQ + AP wake source, switch VADMEM clock to
 *        `CKSL_VADM_VAD` for sleep. Idempotent.
 */
int32_t ameba_audio_vad_arm(struct AmebaAudioVad *self);

/**
 * @brief Disable VAD detector + IRQ. Idempotent.
 */
int32_t ameba_audio_vad_disarm(struct AmebaAudioVad *self);

/**
 * @brief Block on the wake semaphore. Returns 0 on wake-up, < 0 on timeout.
 *        On success, the wake callback (if registered) is invoked from worker
 *        context before this function returns.
 */
int32_t ameba_audio_vad_wait_wakeup(struct AmebaAudioVad *self, uint32_t timeout_ms);

/**
 * @brief Read PCM out of the VAD SRAM ring via fwlib's `get_vad_data`.
 *
 *        The first call after each wake locks the start at
 *        `hit_addr - PRE_READ_NUM_BLOCK` (~400 ms pre-roll, set in fwlib).
 *        Subsequent calls walk forward and busy-wait for live data, so this
 *        is intended to be called repeatedly with small per-frame buffers
 *        (e.g. one 16 ms / 512 B AFE frame) until KWS fires or a frame budget
 *        is reached. `buffer_bytes` is rounded down to whole milliseconds
 *        (32 B/ms per channel @ 16k/16-bit). With channel_count > 1 the data
 *        comes back interleaved (ch0,ch1,..,ch0,ch1,..) so each millisecond
 *        costs 32 * channel_count bytes. Returns bytes copied, or 0 if the
 *        request was smaller than 1 ms.
 */
int32_t ameba_audio_vad_read_lookback(struct AmebaAudioVad *self, void *buffer, size_t buffer_bytes);

/**
 * @brief Switch ADC clock from VAD codec path to audio codec path so that
 *        AudioRecord can subsequently bring up SPORT/GDMA on the same mic.
 */
int32_t ameba_audio_vad_switch_to_record(struct AmebaAudioVad *self);

/**
 * @brief Full 8-step recovery: re-enable AC/AUDIO clocks, codec LDO, AMIC bias,
 *        VAD ADC routing, pitch/HPF, clear pitch-detect latch, VAD_Start +
 *        IRQ enable, VADMEM clock back to VAD source, release the wakelock
 *        taken in the IRQ. Idempotent and safe whether or not AudioRecord ran.
 */
int32_t ameba_audio_vad_rearm_full(struct AmebaAudioVad *self);

/**
 * @brief Update detection thresholds. NOP for fields left at 0 in `cfg`.
 *        Safe to call any time after `_open`; effective immediately.
 */
int32_t ameba_audio_vad_set_thresholds(struct AmebaAudioVad *self, const struct AmebaAudioVadConfig *cfg);

/**
 * @brief Latest VAD trigger address inside the SRAM ring (raw fwlib value).
 */
uint32_t ameba_audio_vad_get_hit_address(struct AmebaAudioVad *self);

/**
 * @brief Register a worker-context wake callback. May be NULL to clear.
 */
void ameba_audio_vad_set_wake_cb(struct AmebaAudioVad *self, AmebaAudioVadWakeCb cb, void *user);

#ifdef __cplusplus
}
#endif

#endif  // AMEBA_AUDIO_AUDIO_HAL_AMEBASMART_AMEBA_AUDIO_VAD_H
