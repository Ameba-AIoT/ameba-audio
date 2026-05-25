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
 * @file audio_track.h
 *
 * @brief Provides APIs of the audio playback streaming.
 *
 *
 * @since 1.0
 * @version 1.0
 */

#ifndef AMEBA_AUDIO_INTERFACES_AUDIO_AUDIO_TRACK_H
#define AMEBA_AUDIO_INTERFACES_AUDIO_AUDIO_TRACK_H

#include <stdint.h>
#include <sys/types.h>

#include "audio/audio_time.h"
#include "audio/audio_type.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup AudioTrack_Types AudioTrack Types
 *  @{
 */

/**
 * @brief stucture for audio playback.
 *
 * @section threading Threading and concurrency
 *
 * Every public AudioTrack_* API is thread safe. Do not call any
 * AudioTrack_* API from interrupt context. There are currently no user
 * callbacks delivered by AudioTrack — all interaction is synchronous.
 *
 * @section error_codes Error codes
 *
 * Negative return values are the @c AUDIO_ERR_* codes defined
 * in <tt>interfaces/common/audio_errnos.h</tt>.
 */

struct AudioTrack;

/**
 * @brief Defines all the audio track configs.
 *
 * @since 1.0
 * @version 1.0
 */
typedef struct {
	/** category type of track */
	uint32_t category_type;
	/** sample_rate of track, supports 8000,16000,32000,44100,48000,96000,192000 */
	uint32_t sample_rate;
	/** channel num of track, supports 1, 2 in mixer architecture, and 1, 2, 4, 6, 8 in passthrough. */
	uint32_t channel_count;
	/** format of track, supports
	 * [mixer] AUDIO_FORMAT_PCM_32_BIT, AUDIO_FORMAT_PCM_16_BIT,
	 * AUDIO_FORMAT_PCM_24_BIT, AUDIO_FORMAT_PCM_FLOAT.
	 * [passthrough] AUDIO_FORMAT_PCM_32_BIT, AUDIO_FORMAT_PCM_16_BIT,
	 * AUDIO_FORMAT_PCM_24_BIT, AUDIO_FORMAT_PCM_8_24_BIT.
	 */
	uint32_t format;
	/** bufsize of track */
	uint32_t buffer_bytes;
} AudioTrackConfig;

/**
 * @brief Playback rate parameters for AudioTrack_SetPlaybackRate().
 *
 * Only available in the mixer build; the passthrough build returns
 * @c AUDIO_ERR_INVALID_OPERATION.
 *
 * @since 1.0
 * @version 1.0
 */
typedef struct {
	/** Playback speed multiplier in the range [0.5, 6.0]. 1.0 = normal speed. */
	float speed;
	/** Pitch multiplier; only 1.0 is supported, other values are ignored. */
	float pitch;
} AudioPlaybackRate;

/** @} End of AudioTrack_Types group */

/** @defgroup AudioTrack_Functions AudioTrack Functions
 *  @{
 */

/**
 * @brief Allocate and zero-initialize a new AudioTrack handle.
 *
 * Must be paired with AudioTrack_Destroy(). Call AudioTrack_Init() before any
 * streaming API.
 *
 * @return Pointer to a new AudioTrack on success; NULL on allocation failure.
 * @see AudioTrack_Init, AudioTrack_Destroy
 *
 * @since 1.0
 * @version 1.0
 */
struct AudioTrack *AudioTrack_Create(void);

/**
 * @brief Tear down an AudioTrack and free all associated resources.
 *
 * After this call the @c track pointer becomes invalid.
 *
 * @param[in] track AudioTrack handle returned by AudioTrack_Create(); a NULL
 *                  pointer is silently ignored.
 *
 * @since 1.0
 * @version 1.0
 */
void AudioTrack_Destroy(struct AudioTrack *track);

/**
 * @brief Init audio track.
 * See the {@link AudioTrackConfig} for information about the options available to configure
 * your track.
 *
 * @param track is the pointer of struct AudioTrack.
 * @param config a {@link AudioTrackConfig} instance used to configure track information.
 * @param flags is the output flags for current audio track.
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_NO_INIT | the init is not done.
 * AUDIO_ERR_INVALID_OPERATION | the config value is not proper.
 * AUDIO_ERR_NO_MEMORY | the memory alloc for track is fail.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioTrack_Init(struct AudioTrack *track, const AudioTrackConfig *config, uint32_t flags);

/**
 * @brief Set the start-playing water level (in bytes).
 *
 * After Start the track will hold output silence until at least @p bytes of
 * PCM has been written; this lets the application pre-buffer to avoid early
 * underrun. Mixer build only — passthrough returns 0 without effect.
 *
 * @param[in] track AudioTrack handle.
 * @param[in] bytes Threshold in bytes; must be ≤ the track buffer size.
 * @return The threshold actually applied (which may be clamped). Negative on
 *         invalid input.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioTrack_SetStartThresholdBytes(struct AudioTrack *track, int32_t bytes);

/**
 * @brief Query the current start-playing water level.
 *
 * @param[in] track AudioTrack handle.
 * @return Threshold bytes currently in effect; 0 if not configured / on
 *         passthrough build.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioTrack_GetStartThresholdBytes(struct AudioTrack *track);

/**
 * @brief Start streaming on this track. Begins consuming data written via
 *        AudioTrack_Write() and rendering it to the output device.
 *
 * Legal initial states: STOPPED and FLUSHED.
 * Calling Start while already ACTIVE returns @c AUDIO_ERR_INVALID_OPERATION.
 *
 * @param[in] track AudioTrack handle, already Init-ed.
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | should not start now.
 * AUDIO_ERR_DEAD_OBJECT | the ipc is dead.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioTrack_Start(struct AudioTrack *track);

/**
 * @brief Stop streaming on this track.
 *
 * Stop track streaming. In mixer mode, wait until remained buffer rendered.
 * In passthrough, stop immediately.
 *
 * @param[in] track AudioTrack handle.
 *
 * @since 1.0
 * @version 1.0
 */
void AudioTrack_Stop(struct AudioTrack *track);

/**
 * @brief Push PCM samples from the application into the track for playback.
 *
 * Data must match the format / channel_count / sample_rate set in
 * AudioTrack_Init().
 *
 * - @c should_block = true : the call waits until all @p size bytes have been
 *   queued; suitable for typical pull-from-file pipelines. If the track is
 *   stopped/flushed mid-call, the wait is interrupted and the partial byte
 *   count (or a negative error) is returned.
 * - @c should_block = false : the call writes only what fits without
 *   blocking and returns immediately; may return 0 if no buffer space is
 *   currently available.
 *
 * Calling Write before AudioTrack_Start() (passthrough) returns
 * @c AUDIO_ERR_INVALID_OPERATION; in mixer build, data may queue but is not
 * rendered until Start.
 *
 * @param[in] track        AudioTrack handle, Init-ed.
 * @param[in] buffer       Source PCM buffer; must not be NULL.
 * @param[in] size         Number of bytes to write from @p buffer.
 * @param[in] should_block True to block until done (recommended), false for
 *                         non-blocking semantics.
 * @return Bytes actually written on success (0 ≤ ret ≤ @p size); a negative
 *         @c AUDIO_ERR_* on failure.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioTrack_Write(struct AudioTrack *track, const void *buffer, size_t size, bool should_block);

/**
 * @brief Get minimun buffer bytes for track.
 *
 * @param track is the pointer of struct AudioTrack.
 * @param category_type can be a value listed below: \n
 * type | Description
 * ----------------------| -----------------------
 * AUDIO_CATEGORY_MEDIA | the data stream is music.
 * AUDIO_CATEGORY_COMMUNICATION | the data stream is call.
 * AUDIO_CATEGORY_TTS | the data stream is voice recognition.
 * AUDIO_CATEGORY_BEEP | the data stream is beep.
 * @param sample_rate is the samplerate of AudioTrack.
 * @param format can be a value listed below: \n
 * format | Description
 * ----------------------| -----------------------
 * AUDIO_FORMAT_PCM_8_BIT | 8bit data format.
 * AUDIO_FORMAT_PCM_16_BIT | 16bit data format.
 * AUDIO_FORMAT_PCM_32_BIT | 32bit data format.
 * AUDIO_FORMAT_PCM_FLOAT | float data format.
 * AUDIO_FORMAT_PCM_24_BIT | 24bit data format.
 * @param channel_count is the channel count of AudioTrack.
 * @return {@link AudioTrackConfig#buffer_bytes} size of mininum buffer bytes
 * @since 1.0
 * @version 1.0
 */
size_t AudioTrack_GetMinBufferBytes(struct AudioTrack *track, uint32_t category_type, uint32_t sample_rate, uint32_t format, uint32_t channel_count);

/**
 * @brief Set samplerate of track.
 *
 * @param track is the pointer of struct AudioTrack.
 * @param sample_rate the samplerate of track.
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | param not supported.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioTrack_SetSampleRate(struct AudioTrack *track, uint32_t sample_rate);

/**
 * @brief Get samplerate of audio track.
 *
 * @param track is the pointer of struct AudioTrack.
 * @return samplerate
 * @since 1.0
 * @version 1.0
 */
uint32_t AudioTrack_GetSampleRate(struct AudioTrack *track);

/**
 * @brief Set format of track.
 *
 * @param track is the pointer of struct AudioTrack.
 * @param format can be a value listed below: \n
 * format | Description
 * ----------------------| -----------------------
 * AUDIO_FORMAT_PCM_8_BIT | 8bit data format.
 * AUDIO_FORMAT_PCM_16_BIT | 16bit data format.
 * AUDIO_FORMAT_PCM_32_BIT | 32bit data format.
 * AUDIO_FORMAT_PCM_FLOAT | float data format.
 * AUDIO_FORMAT_PCM_24_BIT | 24bit data format.
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | param not supported.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioTrack_SetFormat(struct AudioTrack *track, uint32_t format);

/**
 * @brief Get format of audio track.
 *
 * @param track is the pointer of struct AudioTrack.
 * @return format can be a value listed below: \n
 * format | Description
 * ----------------------| -----------------------
 * AUDIO_FORMAT_INVALID | invalid data format.
 * AUDIO_FORMAT_PCM_8_BIT | 8bit data format.
 * AUDIO_FORMAT_PCM_16_BIT | 16bit data format.
 * AUDIO_FORMAT_PCM_32_BIT | 32bit data format.
 * AUDIO_FORMAT_PCM_FLOAT | float data format.
 * AUDIO_FORMAT_PCM_24_BIT | 24bit data format.
 * @since 1.0
 * @version 1.0
 */
uint32_t AudioTrack_GetFormat(struct AudioTrack *track);

/**
 * @brief Set channel count of track.
 *
 * @param track is the pointer of struct AudioTrack.
 * @param channel the channel count of track.
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | param not supported.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioTrack_SetChannelCount(struct AudioTrack *track, uint32_t channel);

/**
 * @brief Get channel count of audio track.
 *
 * @param track is the pointer of struct AudioTrack.
 * @return channel count of track.
 * @since 1.0
 * @version 1.0
 */
uint32_t AudioTrack_GetChannelCount(struct AudioTrack *track);

/**
 * @brief Pause playback while keeping queued data and the current position.
 *
 * Mixer build only: ACTIVE → PAUSED. Resume with AudioTrack_Start(). Calling
 * Pause from any other state is a no-op. The passthrough build does not
 * implement Pause and the call is silently ignored.
 *
 * @param[in] track AudioTrack handle.
 *
 * @since 1.0
 * @version 1.0
 */
void AudioTrack_Pause(struct AudioTrack *track);

/**
 * @brief Drop all queued samples without stopping the track.
 *
 * Mixer build only: must be called from PAUSED (typical sequence is
 * Pause → Flush → Start to seek). Resets the playback position to 0.
 * Calling Flush from ACTIVE or FLUSHED is a no-op. The passthrough build
 * does not implement Flush and the call is silently ignored.
 *
 * @param[in] track AudioTrack handle.
 *
 * @since 1.0
 * @version 1.0
 */
void AudioTrack_Flush(struct AudioTrack *track);

/**
 * @brief Set volume of track.
 *
 * @param track is the pointer of struct AudioTrack.
 * @param left the left channel volume of track, the value ranges from 0.0 to 1.0.
 * @param right the right channel volume of track, the value ranges from 0.0 to 1.0.
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | param not supported.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioTrack_SetVolume(struct AudioTrack *track, float left, float right);

/**
 * @brief Set speed of track.
 *
 * @param track is the pointer of struct AudioTrack.
 * @param rate See the {@link AudioPlaybackRate} for information.
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_PARAM | the speed value is not supported.
 * AUDIO_ERR_INVALID_OPERATION | param not supported.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioTrack_SetPlaybackRate(struct AudioTrack *track, AudioPlaybackRate rate);

/**
 * @brief Get speed of track.
 *
 * @param track is the pointer of struct AudioTrack.
 * @param rate See the {@link AudioPlaybackRate} for information.
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | param not supported.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioTrack_GetPlaybackRate(struct AudioTrack *track, AudioPlaybackRate *rate);

/**
 * @brief Read the current playback timestamp.
 *
 * Use for audio sync and latency calculation. Only valid while the track is
 * actively rendering — in mixer build, calling in STOPPED / FLUSHED returns
 * @c AUDIO_ERR_WOULD_BLOCK.
 *
 * @param[in]  track  AudioTrack handle.
 * @param[out] tstamp Out parameter; on success holds presented frame count
 *                    and the time at which it was reached.
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_WOULD_BLOCK | track is not active (mixer build).
 * AUDIO_ERR_INVALID_OPERATION | param not supported.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioTrack_GetTimestamp(struct AudioTrack *track, AudioTimestamp *tstamp);

/**
 * @brief Read the system time and the audio presentation time as a pair.
 *
 * Returns a pair (system clock now, audio PTS now).
 * Passthrough build only — mixer build returns @c AUDIO_ERR_INVALID_OPERATION.
 * Must be called after AudioTrack_Start().
 *
 * @param[in]  track    AudioTrack handle.
 * @param[out] now_ns   System time in nanoseconds.
 * @param[out] audio_ns Audio presentation time in nanoseconds at the same
 *                      instant as @p now_ns.
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | not supported in this build / state.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioTrack_GetPresentTime(struct AudioTrack *track, int64_t *now_ns, int64_t *audio_ns);

/**
 * @brief Read the system time at which the most recent Start/Stop took
 *        effect on the rendering hardware.
 *
 * Passthrough build only — mixer build returns @c AUDIO_ERR_INVALID_OPERATION.
 *
 * @param[in]  audio_track AudioTrack handle.
 * @param[out] trigger_ns  Trigger time in nanoseconds.
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | not supported in this build / state.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioTrack_GetTriggerTimestamp(struct AudioTrack *audio_track, int64_t *trigger_ns);

/**
 * @brief Set per-track private parameters via a "key=value;..." string.
 *
 * Passthrough build only — the mixer build returns
 * @c AUDIO_ERR_INVALID_OPERATION. Must be called after AudioTrack_Start()
 * (the underlying HAL stream has to exist).
 *
 * Recognised keys include:
 * - <tt>amp_pin=&lt;N&gt;</tt> — GPIO pin number (numeric form of e.g. _PB_7
 *   from fwlib/include/ameba_pinmux.h; _PB_7 = 0x27 = 39) used to drive the
 *   amplifier mute / power-on signal.
 *
 * @param[in] track AudioTrack handle.
 * @param[in] strs  Semicolon-separated key=value parameters; must not be NULL.
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | param not supported, or wrong build / state.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioTrack_SetParameters(struct AudioTrack *track, const char *strs);

/**
 * @brief Query free space in the underlying DMA / streaming buffer.
 *
 * Use to size the next AudioTrack_Write() in non-blocking applications.
 * Passthrough build only — mixer build is unimplemented and returns 0.
 * Returns 0 if the underlying stream has not yet been opened (i.e. before
 * AudioTrack_Start()).
 *
 * @param[in] track AudioTrack handle.
 * @return Number of bytes that can be written without blocking.
 * @since 1.0
 * @version 1.0
 */
uint32_t AudioTrack_GetBufferStatus(struct AudioTrack *track);

/**
 * @brief Query the total size (capacity) of the track buffer.
 *
 * Passthrough build only — mixer build is unimplemented and returns 0.
 * Returns 0 before AudioTrack_Start() opens the stream.
 *
 * @param[in] track AudioTrack handle.
 * @return Total buffer capacity in bytes.
 * @since 1.0
 * @version 1.0
 */
uint32_t AudioTrack_GetBufferSize(struct AudioTrack *track);

/**
 * @brief Get msec latency of audio track.
 *
 * @param track is the pointer of struct AudioTrack.
 * @param latency is the pointer of the latency user wants.
 * @return  Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_NO_INIT | track error.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioTrack_GetLatency(struct AudioTrack *track, uint32_t *latency);

/**
 * @brief Read the cumulative number of frames played since the last Start.
 *
 * Reset to zero by AudioTrack_Stop() and AudioTrack_Flush(). The counter
 * is 64-bit so practical overflow is not a concern. In mixer build this
 * returns 0 (success) when the track is not active.
 *
 * @param[in]  track    AudioTrack handle.
 * @param[out] position Out parameter; total frames presented to the device.
 * @return  Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_NO_INIT | track not initialised / underlying stream not open.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioTrack_GetPosition(struct AudioTrack *track, uint64_t *position);

/** @} End of AudioTrack_Functions group */

#ifdef __cplusplus
}
#endif


#endif  // AMEBA_AUDIO_INTERFACES_AUDIO_AUDIO_TRACK_H
/** @} */
