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
 * @brief Declares APIs for media framework.
 *
 *
 * @since 1.0
 * @version 1.0
 */

/**
 * @file media_player.h
 *
 * @brief Provides the APIs to implement operations related to manage playback.
 * These player interfaces can be used to control playback of audio files and streams(via https or rtsp),
 * register observer functions, and control the feature status.
 * Playback control includs start, stop, pause, resume, rewind and so on.
 *
 *
 * @since 1.0
 * @version 1.0
 */

#ifndef AMEBA_FWK_MEDIA_PLAYBACK_MEDIA_MEDIA_PLAYER_H
#define AMEBA_FWK_MEDIA_PLAYBACK_MEDIA_MEDIA_PLAYER_H

#include "parcel.h"
#include "stream_source.h"
#include "media_player_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MediaPlayer MediaPlayer;
typedef struct MediaPlayerCallback MediaPlayerCallback;
typedef enum AudioSinkTypes AudioSinkTypes;

/**
 * @brief Provides calback interfaces.
 */
struct MediaPlayerCallback {
    /**
     * @brief Called when player status is changed.
     *
     * @param listener The MediaPlayerCallback object pointer.
     * @param player The MediaPlayer object pointer.
     * @param state The player status, one of {@link MediaPlayerStates}.
     * @since 1.0
     * @version 1.0
     */
    void (*OnMediaPlayerStateChanged)(const MediaPlayerCallback *listener, const MediaPlayer *player, int state);

    /**
     * @brief Called when player information is received.
     *
     * @param listener The MediaPlayerCallback object pointer.
     * @param player The MediaPlayer object pointer.
     * @param info Indicates the information type. For details, see {@link MediaPlayerInfos}.
     * @param extra Indicates the information code.
     * @since 1.0
     * @version 1.0
     */
    void (*OnMediaPlayerInfo)(const MediaPlayerCallback *listener, const MediaPlayer *player, int info, int extra);

    /**
     * @brief Called when a player error occurs.
     *
     * @param listener The MediaPlayerCallback object pointer.
     * @param player The MediaPlayer object pointer.
     * @param error Indicates the error type. For details, see {@link MediaPlayerErrors}.
     * @param extra Indicates the error code.
     * @since 1.0
     * @version 1.0
     */
    void (*OnMediaPlayerError)(const MediaPlayerCallback *listener, const MediaPlayer *player, int error, int extra);
};

/**
 * @brief Creates MediaPlayer.
 *
 * @return a new MediaPlayer object pointer.
 * @since 1.0
 * @version 1.0
 */
MediaPlayer *MediaPlayer_Create(void);

/**
 * @brief Creates MediaPlayer with sink type.
 *
 * @param type The sink type, one of {@link AudioSinkTypes}
 * @return a new MediaPlayer object pointer.
 * @since 1.0
 * @version 1.0
 */
MediaPlayer *MediaPlayer_CreateEx(AudioSinkTypes type);

/**
 * @brief Destory MediaPlayer.
 *
 * @param player The MediaPlayer object pointer.
 * @since 1.0
 * @version 1.0
 */
void    MediaPlayer_Destory(MediaPlayer *player);

/**
 * @brief Sets the source(url) to use.
 *
 * @param player The MediaPlayer object pointer.
 * @param url The url of the file you want to play.
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | playback is not in idle state.
 * AUDIO_ERR_UNKNOWN_ERROR | operation is failed.
 * @since 1.0
 * @version 1.0
 */
int32_t MediaPlayer_SetSource(MediaPlayer *player, const char *url);

/**
 * @brief Sets the StreamSource to use.
 *
 * @param player The MediaPlayer object pointer.
 * @param source The StreamSource implemented by customer.
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | playback is not in idle state.
 * AUDIO_ERR_UNKNOWN_ERROR | operation is failed.
 * @since 1.0
 * @version 1.0
 */
int32_t MediaPlayer_SetDataSource(MediaPlayer *player, StreamSource *source);

/**
 * @brief Prepares the player for playback, synchronously.
 *
 * @param player The MediaPlayer object pointer.
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | playback is not in idle state.
 * AUDIO_ERR_UNKNOWN_ERROR | operation is failed.
 * @since 1.0
 * @version 1.0
 */
int32_t MediaPlayer_Prepare(MediaPlayer *player);

/**
 * @brief Prepares the player for playback, asynchronously.
 * A call to {@link MediaPlayer_PrepareAsync()} (asynchronous) will first transfers
 * the playbackt to the MEDIA_PLAYER_PREPARING state after the call returns
 * (which occurs almost right away) while the internal player engine continues
 * working on the rest of preparation work until the preparation work completes.
 * When the preparation completes, the internal player engine then calls a user
 * supplied callback method OnMediaPlayerStateChanged(..., MEDIA_PLAYER_PREPARED)
 * of the MediaPlayerCallbacks interface, if an MediaPlayerCallbacks object is registered
 * beforehand via {@link MediaPlayer_SetCallbacks()}.
 *
 * @param player The MediaPlayer object pointer.
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | playback is not in idle state.
 * AUDIO_ERR_UNKNOWN_ERROR | operation is failed.
 * @since 1.0
 * @version 1.0
 */
int32_t MediaPlayer_PrepareAsync(MediaPlayer *player);

/**
* @brief Starts or resumes playback. If playback had previously been paused,
* playback will continue from where it was paused. If playback had
* been stopped, or never started before, playback will start at the
* beginning.
*
* @param player The MediaPlayer object pointer.
* @return Returns a value listed below: \n
* int32_t | Description
* ----------------------| -----------------------
* AUDIO_OK | the operation is successful.
* AUDIO_ERR_INVALID_OPERATION | playback is not in idle state.
* AUDIO_ERR_UNKNOWN_ERROR | operation is failed.
* @since 1.0
* @version 1.0
*/
int32_t MediaPlayer_Start(MediaPlayer *player);

/**
* @brief Set the player to be looping or non-looping
*
* @param player The MediaPlayer object pointer.
* @param loop 1 means looping, 0 means non-looping.
* @return Returns a value listed below: \n
* int32_t | Description
* ----------------------| -----------------------
* AUDIO_OK | the operation is successful.
* AUDIO_ERR_INVALID_OPERATION | playback is not in idle state.
* AUDIO_ERR_UNKNOWN_ERROR | operation is failed.
* @since 1.0
* @version 1.0
*/
int32_t MediaPlayer_SetLooping(MediaPlayer *player, int8_t loop);

/**
 * @brief Stops playback after playback has been started or paused.
 *
 * @param player The MediaPlayer object pointer.
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | playback is not in idle state.
 * AUDIO_ERR_UNKNOWN_ERROR | operation is failed.
 * @since 1.0
 * @version 1.0
 */
int32_t MediaPlayer_Stop(MediaPlayer *player);

/**
 * @brief Pauses playback. Call MediaPlayer_Start() to resume.
 *
 * @param player The MediaPlayer object pointer.
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | playback is not in idle state.
 * AUDIO_ERR_UNKNOWN_ERROR | operation is failed.
 * @since 1.0
 * @version 1.0
 */
int32_t MediaPlayer_Pause(MediaPlayer *player);

/**
 * @brief Moves the media to specified time position.
 *
 * @param player The MediaPlayer object pointer.
 * @param msec The offset in milliseconds from the start to rewind to.
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | playback is not in idle state.
 * AUDIO_ERR_UNKNOWN_ERROR | operation is failed.
 * @since 1.0
 * @version 1.0
 */
int32_t MediaPlayer_Rewind(MediaPlayer *player, int64_t msec);

/**
 * @brief Resets the Player to its uninitialized state. After calling
 * this method, you will have to initialize it again by setting the
 * source and calling MediaPlayer_Prepare() or MediaPlayer_PrepareAsync().
 *
 * @param player The MediaPlayer object pointer.
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | playback is not in idle state.
 * AUDIO_ERR_UNKNOWN_ERROR | operation is failed.
 * @since 1.0
 * @version 1.0
 */
int32_t MediaPlayer_Reset(MediaPlayer *player);

/**
 * @brief Gets the current playback position.
 *
 * @param player The MediaPlayer object pointer.
 * @param msec The current time updated during this function.
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | playback is not in idle state.
 * AUDIO_ERR_UNKNOWN_ERROR | operation is failed.
 * @since 1.0
 * @version 1.0
 */
int32_t MediaPlayer_GetCurrentTime(MediaPlayer *player, int64_t *msec);

/**
 * @brief Gets the duration of the file.
 *
 * @param player The MediaPlayer object pointer.
 * @param msec The duration in milliseconds, if no duration is available
 *         (for example, if streaming live content), -1 is updated.
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | playback is not in idle state.
 * AUDIO_ERR_UNKNOWN_ERROR | operation is failed.
 * @since 1.0
 * @version 1.0
 */
int32_t MediaPlayer_GetDuration(MediaPlayer *player, int64_t *msec);

/**
 * @brief Checks whether the player is playing.
 *
 * @param player The MediaPlayer object pointer.
 * @return 1 if currently playing, 0 otherwise.
 * @since 1.0
 * @version 1.0
 */
int MediaPlayer_IsPlaying(MediaPlayer *player);

/**
 * @brief Sets player callbacks.
 *
 * @param player The MediaPlayer object pointer.
 * @param callbacks The {@link MediaPlayerCallbacks} instance used to receive player messages.
 * @since 1.0
 * @version 1.0
 */
void MediaPlayer_SetCallback(MediaPlayer *player, MediaPlayerCallback *callbacks);

/**
 * @brief Sets player volume when play started.
 *
 * @param player The MediaPlayer object pointer.
 * @param left The volume of left channel.
 * @param right The volume of right channel.
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | playback is not in idle state.
 * AUDIO_ERR_UNKNOWN_ERROR | operation is failed.
 * @since 1.0
 * @version 1.0
 */
int32_t MediaPlayer_SetVolume(MediaPlayer *player, float left, float right);

/**
 * @brief Sets player speed when play started.
 *
 * @param player The MediaPlayer object pointer.
 * @param speed The speed of player.
 * @param speed The pitch of player.
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | playback is not in idle state.
 * AUDIO_ERR_UNKNOWN_ERROR | operation is failed.
 * @since 1.0
 * @version 1.0
 */
int32_t MediaPlayer_SetSpeed(MediaPlayer *player, float speed, float pitch);

/**
 * @brief Invokes player request.
 *
 * @param request The request parcel object pointer.
 * @param reply The reply parcel object pointer..
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | playback is not in idle state.
 * AUDIO_ERR_UNKNOWN_ERROR | operation is failed.
 * @since 1.0
 * @version 1.0
 */
int32_t MediaPlayer_Invoke(MediaPlayer *player, Parcel *request, Parcel *reply);

#ifdef __cplusplus
}
#endif

#endif // AMEBA_FWK_MEDIA_PLAYBACK_MEDIA_MEDIA_PLAYER_H
/** @} */