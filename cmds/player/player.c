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

#define LOG_TAG "Player"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "os_wrapper.h"
#include "platform_stdlib.h"
#include "basic_types.h"

#include "log/log.h"
#include "audio/audio_service.h"
#include "media/media_player.h"

#include "mystream_source.h"
#include "player.h"

/* source data */
#include "48k_2c_30s_mp3.h"

//#define USE_CACHE
#define USE_PREPARE_ASYNC

#define MAX_URL_SIZE 1024
static char g_url[MAX_URL_SIZE];
bool g_streaming = false;
float g_volume = 1.0;

enum PlayingStatus {
    IDLE,
    PREPARING,
    PREPARED,
    PLAYING,
    PAUSED,
    PLAYING_COMPLETED,
    REWIND_COMPLETE,
    STOPPED,
    RESET,
};
int g_playing_status = IDLE;

struct MediaPlayer *g_player;

void OnStateChanged(const struct MediaPlayerCallback *listener, const struct MediaPlayer *player, int state)
{
    MEDIA_LOGD("OnStateChanged(%p %p), (%d)", listener, player, state);

    switch (state) {
    case MEDIA_PLAYER_PREPARED: { //entered for async prepare
        g_playing_status = PREPARED;
        break;
    }

    case MEDIA_PLAYER_PLAYBACK_COMPLETE: { //eos received, then stop
        g_playing_status = PLAYING_COMPLETED;
        break;
    }

    case MEDIA_PLAYER_STOPPED: { //stop received, then reset
        MEDIA_LOGD("start reset");
        g_playing_status = STOPPED;
        break;
    }

    case MEDIA_PLAYER_PAUSED: { //pause received when do pause or start rewinding
        MEDIA_LOGD("paused");
        g_playing_status = PAUSED;
        break;
    }

    case MEDIA_PLAYER_REWIND_COMPLETE: { //rewind done received, then start
        MEDIA_LOGD("rewind complete");
        g_playing_status = REWIND_COMPLETE;
        break;
    }
    }
}

void OnInfo(const struct MediaPlayerCallback *listener, const struct MediaPlayer *player, int info, int extra)
{
    MEDIA_LOGD("OnInfo (%p %p), (%d, %d)", listener, player, info, extra);

    switch (info) {
    case MEDIA_PLAYER_INFO_BUFFERING_START: {
        MEDIA_LOGD("MEDIA_PLAYER_INFO_BUFFERING_START");
        break;
    }

    case MEDIA_PLAYER_INFO_BUFFERING_END: {
        MEDIA_LOGD("MEDIA_PLAYER_INFO_BUFFERING_END");
        break;
    }

    case MEDIA_PLAYER_INFO_BUFFERING_INFO_UPDATE: {
        MEDIA_LOGD("MEDIA_PLAYER_INFO_BUFFERING_INFO_UPDATE %d", extra);
        break;
    }

    case MEDIA_PLAYER_INFO_NOT_REWINDABLE: {
        MEDIA_LOGD("MEDIA_PLAYER_INFO_NOT_REWINDABLE");
        break;
    }
    }
}

void OnError(const struct MediaPlayerCallback *listener, const struct MediaPlayer *player, int error, int extra)
{
    MEDIA_LOGD("OnError (%p %p), (%d, %d)", player, listener, error, extra);
}

void StartPlay(struct MediaPlayer *player, const char *url)
{
    if (player == NULL) {
        MEDIA_LOGD("start play fail, player is NULL!");
        return;
    }

    MEDIA_LOGD("start to play: %s", url);
    int32_t ret = 0;
    StreamSource *stream_source = NULL;

    g_playing_status = IDLE;

    MEDIA_LOGD("SetSource");

    if (g_streaming) {
        stream_source = MyStreamSource_Create((char *)ready_to_convert0, sizeof(ready_to_convert0));
        ret = MediaPlayer_SetDataSource(player, stream_source);
    } else {
        ret = MediaPlayer_SetSource(player, url);
    }

    if (ret) {
        MEDIA_LOGE("SetDataSource fail:error=%d", (int)ret);
        goto exit;
    }

#ifdef USE_PREPARE_ASYNC
    ret = MediaPlayer_PrepareAsync(player);
    if (ret) {
        MEDIA_LOGE("prepare async fail:error=%ld", ret);
        goto exit;
    }

    g_playing_status = PREPARING;

    while (g_playing_status != PREPARED) {
        rtos_time_delay_ms(20);
        if (g_playing_status == IDLE) {
            MEDIA_LOGE("player not prepared, now goto exit!");
            goto exit;
        }
    }
#else
    MEDIA_LOGD("Prepare");
    ret = MediaPlayer_Prepare(player);
    if (ret) {
        MEDIA_LOGE("prepare fail:error=%d", (int)ret);
        goto exit;
    }
#endif

    MEDIA_LOGD("Start");
    ret = MediaPlayer_Start(player);
    if (ret) {
        MEDIA_LOGE("start fail:error=%d", (int)ret);
        goto exit;
    }

    g_playing_status = PLAYING;

    int64_t duration = 0;
    MediaPlayer_GetDuration(player, &duration);
    MEDIA_LOGD("duration is %lldms", duration);

    while (g_playing_status == PLAYING || g_playing_status == PAUSED) {
        rtos_time_delay_ms(1000);
    }

    if (g_playing_status == PLAYING_COMPLETED || g_playing_status == IDLE) {
        MEDIA_LOGD("play complete, now stop.");
        MediaPlayer_Stop(player);
    }

    while (g_playing_status == PLAYING_COMPLETED) {
        rtos_time_delay_ms(1000);
    }

    if (g_playing_status == STOPPED) {
        MEDIA_LOGD("play stopped, now reset.");
        MediaPlayer_Reset(player);
    }

exit:
    if (stream_source) {
        MyStreamSource_Destroy((MyStreamSource *)stream_source);
    }

    MEDIA_LOGD("play %s done!!!!", url);
}

void player_thread(void *param)
{
    (void) param;

    MEDIA_LOGD("player test start......");

    AudioService_Init();
    MEDIA_LOGD("AudioService_Init done");

    struct MediaPlayerCallback *callback = (struct MediaPlayerCallback *)malloc(sizeof(struct MediaPlayerCallback));
    if (!callback) {
        MEDIA_LOGE("Calloc MediaPlayerCallback fail.");
        return;
    }

    callback->OnMediaPlayerStateChanged = OnStateChanged;
    callback->OnMediaPlayerInfo = OnInfo;
    callback->OnMediaPlayerError = OnError;

    g_player = MediaPlayer_Create();

    MediaPlayer_SetCallback(g_player, callback);

    Parcel *request = Parcel_Create();
    Parcel_WriteInt32(request, 1112);
    Parcel_WriteFloat(request, g_volume);
    MediaPlayer_Invoke(g_player, request, NULL);
    Parcel_Destroy(request);

#ifdef USE_CACHE
    int32_t cache_enable = 1;
    char *prefix = "fat://";
    char *cache_dir = "cache";
    int32_t max_cache_count = 100;
    Parcel *cache_request = Parcel_Create();
    Parcel_WriteInt32(cache_request, 4);
    Parcel_WriteInt32(cache_request, cache_enable);
    Parcel_WriteCString(cache_request, prefix);
    Parcel_WriteCString(cache_request, cache_dir);
    Parcel_WriteInt32(cache_request, max_cache_count);
    MediaPlayer_Invoke(g_player, cache_request, NULL);
    Parcel_Destroy(cache_request);
#endif

    StartPlay(g_player, g_url);

    free(callback);
    MediaPlayer_Destory(g_player);
    g_player = NULL;

    rtos_time_delay_ms(1 * 1000);

    MEDIA_LOGD("player test done......");

    rtos_task_delete(NULL);
}

static void player_help(void)
{
    MEDIA_LOGD("player [OPTION...]\n"
            "\t\t[-f file]        An audio file buffer or path\n"
            "\t\t[-s 0/1]         Use stream source flag, stream source must be used together with audio file buffer\n"
            "\t\tExamples:\n"
            "\t\t1. play a http file:\n"
            "\t\t   player -f http://aod.cos.tx.xmcdn.com/group72/M02/0A/07/wKgO0F4tEivQbT6uAEBqyNIMu88237.mp3\n"
            "\t\t2. play stream source\n"
            "\t\t   player -f buffer -s 1\n");
}

uint32_t player_cmd_handle(int argc, char *argv[])
{
    if (argc <= 0) {
        player_help();
        return FALSE;
    }

    memset(g_url, 0, MAX_URL_SIZE);
    g_streaming = false;
    g_volume = 1.0;

    /* parse command line arguments */
    while (*argv) {
        if (strcmp(*argv, "-f") == 0) {
            argv++;
            if (*argv) {
                memset(g_url, 0, MAX_URL_SIZE);
                    snprintf(g_url, MAX_URL_SIZE, "%s", *argv);
            }
        } else if (strcmp((const char *)*argv, "-s") == 0) {
            argv++;
            g_streaming = atoi((const char *)*argv);
        } else if (strcmp((const char *)*argv, "-v") == 0) {
            argv++;
            g_volume = atof((const char *)*argv);
        }

        if (*argv) {
            argv++;
        }
    }
    MEDIA_LOGD("Usage: url is %s, use stream source:%d, volume:%f", g_url, g_streaming, g_volume);

    if (rtos_task_create(NULL, ((const char *)"player_thread"), player_thread, NULL, 8 * 1024, 1) != RTK_SUCCESS) {
        MEDIA_LOGD("%s rtos_task_create(player_thread) failed", __FUNCTION__);
    }

    return TRUE;
}