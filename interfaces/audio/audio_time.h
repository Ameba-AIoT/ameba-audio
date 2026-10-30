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
 * @file audio_time.h
 *
 * @brief Provides definition of the audio timestamp structure.
 *
 *
 * @since 1.0
 * @version 1.0
 */

#ifndef AMEBA_AUDIO_INTERFACES_AUDIO_AUDIO_TIME_H
#define AMEBA_AUDIO_INTERFACES_AUDIO_AUDIO_TIME_H

#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup AudioTime_Types AudioTime Types
 * @{
 */

/**
 * @brief Audio stream timestamp: a frame position paired with the
 *        time at which that position was rendered (playback) or captured
 *        (record).
 *
 * Filled in by AudioTrack_GetTimestamp() / AudioRecord_GetTimestamp().
 * Use it for Audio sync, latency measurement. The pair (position, time) is
 * read atomically by the implementation;
 *
 * @since 1.0
 * @version 1.0
 */
typedef struct AudioTimestamp {
	/** Frame position on the stream timeline. For an output track, this is
	 *  the number of frames the device has rendered through the speaker
	 *  (presentation position). For an input record, this is the number of
	 *  frames captured at the device. Counts in stream frames, not bytes. */
	uint64_t            position;
	/** time at which @c position was reached. */
	struct timespec     time;
} AudioTimestamp;

/** @} End of AudioTime_Types group */

#ifdef __cplusplus
}
#endif

/** @} */

#endif
