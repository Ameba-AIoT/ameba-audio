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
 * @addtogroup Media
 * @{
 *
 * @brief Declares variables for media framework.
 *
 *
 * @since 1.0
 * @version 1.0
 */

/**
 * @file media_player_common.h
 *
 * @brief Provides status, errors, info variables for media framework..
 *
 *
 * @since 1.0
 * @version 1.0
 */

#ifndef AMEBA_FWK_MEDIA_PLAYBACK_MEDIA_MEDIA_PLAYER_COMMON_H
#define AMEBA_FWK_MEDIA_PLAYBACK_MEDIA_MEDIA_PLAYER_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Defines the sink types of a player.
 *
 * @since 1.0
 * @version 1.0
 */
enum AudioSinkTypes {
    /** Audio is played in local. */
    AUDIO_SINK_TYPE_LOCAL = 0,
    /** Audio is played via bluetooth. */
    AUDIO_SINK_TYPE_BT = 1,
    /** Audio is played via uac. */
    AUDIO_SINK_TYPE_UAC = 2,
};

/**
 * @brief Defines all the player status.
 *
 * @since 1.0
 * @version 1.0
 */
enum MediaPlayerStates {
    /** Used to indicate an idle state */
    MEDIA_PLAYER_IDLE = 0,
    /** Used to indicate player is preparing */
    MEDIA_PLAYER_PREPARING = 1,
    /** Used to indicate player prepared */
    MEDIA_PLAYER_PREPARED = 2,
    /** Used to indicate player started */
    MEDIA_PLAYER_STARTED = 3,
    /** Used to indicate player paused */
    MEDIA_PLAYER_PAUSED = 4,
    /** Used to indicate player stopped */
    MEDIA_PLAYER_STOPPED = 5,
    /** Used to indicate player completed */
    MEDIA_PLAYER_PLAYBACK_COMPLETE = 6,
    /** Used to indicate rewind player completed */
    MEDIA_PLAYER_REWIND_COMPLETE = 7,
    /** Used to indicate player error */
    MEDIA_PLAYER_ERROR = 8,
};

/**
 * @brief Defines the player errors.
 *
 * @since 1.0
 * @version 1.0
 */
enum MediaPlayerErrors {
    /** Used to indicate a player error */
    MEDIA_PLAYER_ERROR_UNKNOWN = 0,
};

/**
 * @brief Defines the extra informations of a player.
 *
 * @since 1.0
 * @version 1.0
 */
enum MediaPlayerInfos {
    /** Player is temporarily pausing playback internally in order to buffer more data. */
    MEDIA_PLAYER_INFO_BUFFERING_START = 0,
    /** Player is resuming playback after filling buffers. */
    MEDIA_PLAYER_INFO_BUFFERING_END = 1,
    /** Player buffered data percentage update. */
    MEDIA_PLAYER_INFO_BUFFERING_INFO_UPDATE = 2,
    /** The media is not rewindable (e.g live stream). */
    MEDIA_PLAYER_INFO_NOT_REWINDABLE = 3,
};

#ifdef __cplusplus
}
#endif

#endif // AMEBA_FWK_MEDIA_PLAYBACK_MEDIA_MEDIA_PLAYER_COMMON_H
/** @} */