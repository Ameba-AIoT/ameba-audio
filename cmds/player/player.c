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

#define TAG "Player"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "os_wrapper.h"
#include "platform_stdlib.h"
#include "basic_types.h"

#include "audio/audio_service.h"
#include "media/media_player.h"

#include "mystream_source.h"
#include "player.h"

/* source data */
#include "48k_2c_30s_mp3.h"

#include "audio_cmd_common.h"

//#define USE_CACHE
#define USE_PREPARE_ASYNC

#define MAX_URL_SIZE 1024
static char g_url[MAX_URL_SIZE];
bool g_streaming = false;
float g_volume = 1.0;

typedef struct {
    char url[MAX_URL_SIZE];
    bool streaming;
    float volume;
} player_params_t;

static const player_params_t PLAYER_DEFAULT_PARAMS = {
    .url = "",
    .streaming = false,
    .volume = 1.0,
};

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
    RTK_LOGI(TAG, "OnStateChanged(%p %p), (%d)\n", listener, player, state);

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
        RTK_LOGI(TAG, "start reset\n");
        g_playing_status = STOPPED;
        break;
    }

    case MEDIA_PLAYER_PAUSED: { //pause received when do pause or start rewinding
        RTK_LOGI(TAG, "paused\n");
        g_playing_status = PAUSED;
        break;
    }

    case MEDIA_PLAYER_REWIND_COMPLETE: { //rewind done received, then start
        RTK_LOGI(TAG, "rewind complete\n");
        g_playing_status = REWIND_COMPLETE;
        break;
    }
    }
}

void OnInfo(const struct MediaPlayerCallback *listener, const struct MediaPlayer *player, int info, int extra)
{
    RTK_LOGI(TAG, "OnInfo (%p %p), (%d, %d)\n", listener, player, info, extra);

    switch (info) {
    case MEDIA_PLAYER_INFO_BUFFERING_START: {
        RTK_LOGI(TAG, "MEDIA_PLAYER_INFO_BUFFERING_START\n");
        break;
    }

    case MEDIA_PLAYER_INFO_BUFFERING_END: {
        RTK_LOGI(TAG, "MEDIA_PLAYER_INFO_BUFFERING_END\n");
        break;
    }

    case MEDIA_PLAYER_INFO_BUFFERING_INFO_UPDATE: {
        RTK_LOGI(TAG, "MEDIA_PLAYER_INFO_BUFFERING_INFO_UPDATE %d\n", extra);
        break;
    }

    case MEDIA_PLAYER_INFO_NOT_REWINDABLE: {
        RTK_LOGI(TAG, "MEDIA_PLAYER_INFO_NOT_REWINDABLE\n");
        break;
    }
    }
}

void OnError(const struct MediaPlayerCallback *listener, const struct MediaPlayer *player, int error, int extra)
{
    RTK_LOGI(TAG, "OnError (%p %p), (%d, %d)\n", player, listener, error, extra);
}

void StartPlay(struct MediaPlayer *player, const char *url)
{
    if (player == NULL) {
        RTK_LOGI(TAG, "start play fail, player is NULL!\n");
        return;
    }

    RTK_LOGI(TAG, "start to play: %s\n", url);
    int32_t ret = 0;
    StreamSource *stream_source = NULL;

    g_playing_status = IDLE;

    RTK_LOGI(TAG, "SetSource\n");

    if (g_streaming) {
        stream_source = MyStreamSource_Create((char *)ready_to_convert0, sizeof(ready_to_convert0));
        ret = MediaPlayer_SetDataSource(player, stream_source);
    } else {
        ret = MediaPlayer_SetSource(player, url);
    }

    if (ret) {
        RTK_LOGE(TAG, "SetDataSource fail:error=%d\n", (int)ret);
        goto exit;
    }

#ifdef USE_PREPARE_ASYNC
    ret = MediaPlayer_PrepareAsync(player);
    if (ret) {
        RTK_LOGE(TAG, "prepare async fail:error=%ld\n", ret);
        goto exit;
    }

    g_playing_status = PREPARING;

    while (g_playing_status != PREPARED) {
        rtos_time_delay_ms(20);
        if (g_playing_status == IDLE) {
            RTK_LOGE(TAG, "player not prepared, now goto exit!\n");
            goto exit;
        }
    }
#else
    RTK_LOGI(TAG, "Prepare\n");
    ret = MediaPlayer_Prepare(player);
    if (ret) {
        RTK_LOGE(TAG, "prepare fail:error=%d\n", (int)ret);
        goto exit;
    }
#endif

    RTK_LOGI(TAG, "Start\n");
    ret = MediaPlayer_Start(player);
    if (ret) {
        RTK_LOGE(TAG, "start fail:error=%d\n", (int)ret);
        goto exit;
    }

    g_playing_status = PLAYING;

    int64_t duration = 0;
    MediaPlayer_GetDuration(player, &duration);
    RTK_LOGI(TAG, "duration is %lldms\n", duration);

    while (g_playing_status == PLAYING || g_playing_status == PAUSED) {
        rtos_time_delay_ms(1000);
    }

    if (g_playing_status == PLAYING_COMPLETED || g_playing_status == IDLE) {
        RTK_LOGI(TAG, "play complete, now stop.\n");
        MediaPlayer_Stop(player);
    }

    while (g_playing_status == PLAYING_COMPLETED) {
        rtos_time_delay_ms(1000);
    }

    if (g_playing_status == STOPPED) {
        RTK_LOGI(TAG, "play stopped, now reset.\n");
        MediaPlayer_Reset(player);
    }

exit:
    if (stream_source) {
        MyStreamSource_Destroy((MyStreamSource *)stream_source);
    }

    RTK_LOGI(TAG, "play %s done!!!!\n", url);
}

void player_thread(void *param)
{
    (void)param;
    MEM_DEBUG_DECLARE(mem_dbg);
    MEM_DEBUG_INIT(mem_dbg);

    RTK_LOGI(TAG, "player test start......\n");

    AudioService_Init();
    RTK_LOGI(TAG, "AudioService_Init done\n");

    struct MediaPlayerCallback *callback = (struct MediaPlayerCallback *)malloc(sizeof(struct MediaPlayerCallback));
    if (!callback) {
        RTK_LOGE(TAG, "Calloc MediaPlayerCallback fail.\n");
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

    RTK_LOGI(TAG, "player test done......\n");

    MEM_DEBUG_DUMP(mem_dbg);
    rtos_task_delete(NULL);
}

static void player_help(void)
{
    RTK_LOGI(TAG, "player [OPTION...]\n"
            "\t\t[-f file]        An audio file buffer or path\n"
            "\t\t[-s 0/1]         Use stream source flag, stream source must be used together with audio file buffer\n"
            "\t\tExamples:\n"
            "\t\t1. play a http file:\n"
            "\t\t   player -f http://aod.cos.tx.xmcdn.com/group72/M02/0A/07/wKgO0F4tEivQbT6uAEBqyNIMu88237.mp3\n"
            "\t\t2. play stream source\n"
            "\t\t   player -f buffer -s 1\n");
}

static void parse_player_params(cmd_params_t *params, player_params_t *p)
{
    *p = PLAYER_DEFAULT_PARAMS;

    CMD_PARSE_STRING(p->url, MAX_URL_SIZE, "-f", "");
    CMD_PARSE_BOOL(p->streaming, "-s");
    CMD_PARSE_FLOAT(p->volume, "-v", 1.0);
}

static uint32_t player_handler(cmd_params_t *params)
{
    if (params->argc <= 1) {
        player_help();
        return TRUE;
    }

    player_params_t p;
    parse_player_params(params, &p);

	if (strlen(p.url) == 0) {
        RTK_LOGE(TAG, "No file specified, use -f option\n");
        return FALSE;
    }

	/* 使用p->url, p->streaming, p->volume */
    strcpy(g_url, p.url);
    g_streaming = p.streaming;
    g_volume = p.volume;

    RTK_LOGI(TAG, "Playing: %s, streaming=%d, volume=%f\n", p.url, p.streaming, p.volume);

    if (rtos_task_create(NULL, "player_thread", player_thread, NULL,
                        5632, 1) != RTK_SUCCESS) {
        return FALSE;
    }

    return TRUE;
}
DEFINE_CMD_WRAPPER(player, player_handler, 5);

CMD_TABLE_DATA_SECTION
const COMMAND_TABLE player_cmd_table[] = {
    {
        "player", player_cmd_thread
    },
};