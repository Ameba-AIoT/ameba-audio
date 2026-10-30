#define SM_LOG_TAG "PlaylistParser"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "osal_c/osal_errnos.h"
#include "osal_c/osal_mem.h"
#include "osal_c/osal_mutex.h"
#include "osal_c/osal_thread.h"
#include "osal_c/osal_condition.h"

#include "cutils/container_of.h"

#include "media/media_player.h"
#include "media/media_player_common.h"

#include "cJSON/cJSON.h"

#include "speech_log.h"
#include "speech_tts.h"

#include "playlist_parser.h"

#define WAKE_UP_AUDIO_FILE      "vfs://tts/10001.mp3"
#define VAD_TIMEOUT_AUDIO_FILE  "vfs://tts/10002.mp3"
/* These are TTS-name strings (as produced by speech_tts.c), NOT raw intent ids:
 * intent 37 "play music" -> TTS name "29", intent 38 "stop music" -> TTS name "30".
 * PlaylistParser_AsrMsg matches the parsed TTS names against these. Set to "-1" to disable. */
#define PLAY_MUSIC_INTENT_ID    "29"
#define STOP_MUSIC_INTENT_ID    "30"
#define MUSIC_URL               "vfs://music.mp3"

#define MAX_TTS_CONTENT_LEN (64)
#define MAX_SUB_TTS         (10)
/* Longest URL actually built is "vfs://tts/angle/1360.mp3" (24 chars); 64 leaves
 * room to spare and matches the fixed slot width used by the command ring. */
#define MAX_URL_LEN         (64)

/* Command ring depth: worst case one ASR result queues up to 10 prompts plus a
 * play/stop-music command. 16 covers it with margin. The whole ring lives inside
 * the single heap allocation for the parser (no per-message malloc). */
#define CMD_QUEUE_DEPTH     (16)

/* Worker only does bookkeeping + non-buffering MediaPlayer calls (Reset/Start);
 * ASR JSON is parsed on the caller thread, and Prepare is async, so a modest stack
 * is enough. */
#define WORKER_STACK_SIZE   (2 * 1024)   /* measured peak 1168B during playback, 1.75x margin */

typedef enum {
    CMD_TTS = 0,      /* play a prompt; preempts music, which is stopped (not resumed) */
    CMD_PLAY_MUSIC,   /* start background music from the start */
    CMD_STOP_MUSIC,   /* drop background music entirely */
} CmdType;

typedef struct {
    CmdType type;
    char url[MAX_URL_LEN];
} Cmd;

typedef enum {
    PLAY_NONE = 0,
    PLAY_MUSIC,
    PLAY_TTS,
} PlayKind;

/* Async playback lifecycle of the single MediaPlayer, driven by the worker.
 *   IDLE      -> nothing loaded
 *   PREPARING -> SetDataSource+PrepareAsync issued, waiting for PREPARED callback
 *   PLAYING   -> Start issued, waiting for PLAYBACK_COMPLETE / error */
typedef enum {
    PLAYER_IDLE = 0,
    PLAYER_PREPARING,
    PLAYER_PLAYING,
} PlayerState;

struct PlaylistParser {
    MediaPlayer *player;
    MediaPlayerCallback callback;

    osal_mutex_t lock;
    osal_cond_t  cond;
    osal_thread_t thread;
    bool running;

    /* --- everything below is protected by lock --- */
    Cmd  ring[CMD_QUEUE_DEPTH];
    int  head;
    int  count;

    /* barge-in request (wake-up / VAD timeout). Preempts whatever is playing. */
    bool  preempt;
    char  preempt_url[MAX_URL_LEN];

    /* background music requested but not yet started (played from the start by HandleSavedMusic).
     * Interrupted music is not resumed, so no playback position is tracked. */
    bool    music_saved;
    char    music_url[MAX_URL_LEN];

    PlayKind    current;        /* semantic kind of the loaded clip */
    PlayerState player_state;   /* async state of the MediaPlayer */

    /* set by the MediaPlayer callback (runs on the media stack thread). */
    volatile bool ev_prepared;
    volatile bool ev_done;
    volatile bool ev_err;

    /* Playback generation, bumped on every Reset/prepare. A callback stamps the
     * gen that was current when it fired; the worker only honours events whose
     * gen still matches the in-flight session, so a stale PREPARED/DONE from a
     * clip that was already preempted cannot be misapplied to the new one. */
    uint32_t gen;
    volatile uint32_t ev_gen;

};

/* ---------- command ring (under lock) ---------- */

static bool RingPush(PlaylistParser *p, CmdType type, const char *url)
{
    if (p->count >= CMD_QUEUE_DEPTH) {
        SM_LOGE("cmd ring full, drop type %d", (int)type);
        return false;
    }
    int idx = (p->head + p->count) % CMD_QUEUE_DEPTH;
    p->ring[idx].type = type;
    if (url) {
        strncpy(p->ring[idx].url, url, MAX_URL_LEN - 1);
        p->ring[idx].url[MAX_URL_LEN - 1] = '\0';
    } else {
        p->ring[idx].url[0] = '\0';
    }
    p->count++;
    return true;
}

static bool RingPop(PlaylistParser *p, Cmd *out)
{
    if (p->count == 0) {
        return false;
    }
    *out = p->ring[p->head];
    p->head = (p->head + 1) % CMD_QUEUE_DEPTH;
    p->count--;
    return true;
}

static bool RingHeadIsTts(PlaylistParser *p)
{
    return (p->count > 0) && (p->ring[p->head].type == CMD_TTS);
}

static void RingClear(PlaylistParser *p)
{
    p->head = 0;
    p->count = 0;
}

/* Post a command and wake the worker. */
static void PostCmd(PlaylistParser *p, CmdType type, const char *url)
{
    osal_mutex_lock(&p->lock);
    RingPush(p, type, url);
    osal_cond_signal(&p->cond);
    osal_mutex_unlock(&p->lock);
}

/* ---------- MediaPlayer callback (media stack thread) ----------
 * MUST stay non-blocking: only flag the event and wake the worker. The worker
 * owns every MediaPlayer call, so blocking here would deadlock the media stack. */
static void OnStateChanged(const MediaPlayerCallback *listener, const MediaPlayer *player, int state)
{
    (void)player;
    PlaylistParser *p = container_of(listener, PlaylistParser, callback);
    osal_mutex_lock(&p->lock);
    if (state == MEDIA_PLAYER_PREPARED) {
        p->ev_prepared = true;
        p->ev_gen = p->gen;
        osal_cond_signal(&p->cond);
    } else if (state == MEDIA_PLAYER_PLAYBACK_COMPLETE) {
        p->ev_done = true;
        p->ev_gen = p->gen;
        osal_cond_signal(&p->cond);
    }
    osal_mutex_unlock(&p->lock);
}

static void OnInfo(const MediaPlayerCallback *listener, const MediaPlayer *player, int info, int extra)
{
    (void)listener; (void)player; (void)info; (void)extra;
}

static void OnError(const MediaPlayerCallback *listener, const MediaPlayer *player, int error, int extra)
{
    (void)player; (void)error; (void)extra;
    PlaylistParser *p = container_of(listener, PlaylistParser, callback);
    osal_mutex_lock(&p->lock);
    p->ev_err = true;
    p->ev_gen = p->gen;
    osal_cond_signal(&p->cond);
    osal_mutex_unlock(&p->lock);
}

/* ---------- worker helpers (lock released) ---------- */

/* Kick off async preparation of url. Returns true if SetDataSource+PrepareAsync
 * were accepted; the PREPARED event arrives later via the callback. */
static bool PlayerPrepare(PlaylistParser *p, const char *url)
{
    MediaPlayer_Reset(p->player);
    if (MediaPlayer_SetDataSource(p->player, url) != 0) {
        SM_LOGE("SetDataSource fail %s", url);
        return false;
    }
    if (MediaPlayer_PrepareAsync(p->player) != 0) {
        SM_LOGE("PrepareAsync fail %s", url);
        MediaPlayer_Reset(p->player);
        return false;
    }
    return true;
}

/* Called after the PREPARED event: start playback from the beginning. */
static bool PlayerStart(PlaylistParser *p)
{
    if (MediaPlayer_Start(p->player) != 0) {
        SM_LOGE("Start fail");
        MediaPlayer_Reset(p->player);
        return false;
    }
    return true;
}

/* Stop and unload whatever is loaded; the caller sets player_state to IDLE. Lock released. */
static void PlayerReset(PlaylistParser *p)
{
    MediaPlayer_Reset(p->player);
}

/* ---------- worker state machine ----------
 * Every helper below is called with p->lock held. Calls into the MediaPlayer
 * temporarily drop the lock so API callers and callbacks are never blocked. */

/* Forget the background music; interrupted music is never resumed. */
static void DropMusic(PlaylistParser *p)
{
    p->music_saved = false;
    p->music_url[0] = '\0';
}

/* Stop whatever is loaded and go back to IDLE. */
static void StopClip(PlaylistParser *p)
{
    if (p->player_state != PLAYER_IDLE) {
        p->gen++;   /* invalidate any callback still in flight for this clip */
        osal_mutex_unlock(&p->lock);
        PlayerReset(p);
        osal_mutex_lock(&p->lock);
    }
    p->player_state = PLAYER_IDLE;
    p->current = PLAY_NONE;
}

/* Start async preparation of a new clip. Returns false (and stays IDLE) on failure. */
static bool StartClip(PlaylistParser *p, PlayKind kind, const char *src)
{
    char url[MAX_URL_LEN];
    strncpy(url, src, MAX_URL_LEN);
    p->current = kind;
    p->ev_prepared = p->ev_done = p->ev_err = false;
    p->gen++;   /* new session: only its own PREPARED will match */
    osal_mutex_unlock(&p->lock);
    bool ok = PlayerPrepare(p, url);
    osal_mutex_lock(&p->lock);
    if (!ok) {
        p->player_state = PLAYER_IDLE;
        p->current = PLAY_NONE;
        return false;
    }
    p->player_state = PLAYER_PREPARING;
    return true;
}

/* Each handler returns true if it acted, so the worker re-evaluates from the top. */

/* Barge-in: wake-up / VAD timeout preempts whatever plays now. */
static bool HandlePreempt(PlaylistParser *p)
{
    if (!p->preempt) {
        return false;
    }
    char prompt[MAX_URL_LEN];
    strncpy(prompt, p->preempt_url, MAX_URL_LEN);
    p->preempt = false;
    RingClear(p);

    if (p->current == PLAY_MUSIC) {
        DropMusic(p);
    }
    StopClip(p);
    p->ev_prepared = p->ev_done = p->ev_err = false;
    RingPush(p, CMD_TTS, prompt);
    return true;
}

/* Error on the current clip: reset and move on. */
static bool HandleError(PlaylistParser *p)
{
    if (p->player_state == PLAYER_IDLE || !p->ev_err || p->ev_gen != p->gen) {
        return false;
    }
    PlayKind done = p->current;
    p->ev_err = p->ev_prepared = p->ev_done = false;
    StopClip(p);
    if (done == PLAY_MUSIC) {
        DropMusic(p);
    }
    return true;
}

/* Async prepare finished: start playback from the beginning. */
static bool HandlePrepared(PlaylistParser *p)
{
    if (p->player_state != PLAYER_PREPARING || !p->ev_prepared || p->ev_gen != p->gen) {
        return false;
    }
    p->ev_prepared = false;
    osal_mutex_unlock(&p->lock);
    bool ok = PlayerStart(p);
    osal_mutex_lock(&p->lock);
    if (ok) {
        p->player_state = PLAYER_PLAYING;
    } else {
        p->player_state = PLAYER_IDLE;
        if (p->current == PLAY_MUSIC) {
            DropMusic(p);
        }
        p->current = PLAY_NONE;
    }
    return true;
}

/* Current clip reached its natural end. */
static bool HandleCompletion(PlaylistParser *p)
{
    if (p->player_state != PLAYER_PLAYING || !p->ev_done || p->ev_gen != p->gen) {
        return false;
    }
    PlayKind done = p->current;
    p->ev_done = false;
    StopClip(p);
    if (done == PLAY_MUSIC) {
        DropMusic(p);
    }
    return true;
}

/* A TTS prompt is waiting while music plays: stop the music for good. */
static bool HandleTtsOverMusic(PlaylistParser *p)
{
    if (p->current != PLAY_MUSIC || p->player_state != PLAYER_PLAYING || !RingHeadIsTts(p)) {
        return false;
    }
    DropMusic(p);
    StopClip(p);
    return true;
}

/* Player idle: take the next queued command. */
static bool HandleNextCmd(PlaylistParser *p)
{
    Cmd c;
    if (p->player_state != PLAYER_IDLE || !RingPop(p, &c)) {
        return false;
    }
    if (c.type == CMD_STOP_MUSIC) {
        DropMusic(p);
    } else if (c.type == CMD_PLAY_MUSIC) {
        strncpy(p->music_url, c.url, MAX_URL_LEN - 1);
        p->music_url[MAX_URL_LEN - 1] = '\0';
        p->music_saved = true;   /* started by HandleSavedMusic once the queue is empty */
    } else { /* CMD_TTS */
        StartClip(p, PLAY_TTS, c.url);
    }
    return true;
}

/* Player idle and nothing queued: start the saved background music. */
static bool HandleSavedMusic(PlaylistParser *p)
{
    if (p->player_state != PLAYER_IDLE || !p->music_saved || !p->music_url[0]) {
        return false;
    }
    if (!StartClip(p, PLAY_MUSIC, p->music_url)) {
        DropMusic(p);
    }
    return true;
}

/* ---------- worker thread ----------
 * Single owner of the MediaPlayer. Runs the handlers in priority order and sleeps
 * in cond_wait when none of them has anything to do (idle, or waiting for an async
 * callback). A barge-in signals the cond and is served first by HandlePreempt; the
 * only blocking call the worker makes is Reset(), which a barge-in needs anyway. */
static void *WorkerLoop(void *arg)
{
    PlaylistParser *p = (PlaylistParser *)arg;

    osal_mutex_lock(&p->lock);
    while (p->running) {
        if (HandlePreempt(p) || HandleError(p) || HandlePrepared(p) || HandleCompletion(p) ||
            HandleTtsOverMusic(p) || HandleNextCmd(p) || HandleSavedMusic(p)) {
            continue;
        }
        osal_cond_wait(&p->cond, &p->lock);
    }
    osal_mutex_unlock(&p->lock);
    return NULL;
}

/* ---------- ASR intent parsing (caller thread) ---------- */

/* Map an ASR result JSON to TTS names, appending the prompts of every recognized command in order.
 * Returns the number of names written to tts_list (at most MAX_SUB_TTS). */
static int ParseAsrIntent(const char *json, const char **tts_list)
{
    SM_LOGD("==>rec: %s", json);

    cJSON *res_obj = cJSON_Parse(json);
    if (!res_obj) {
        SM_LOGE("asr json parse fail");
        return 0;
    }

    cJSON *type_obj = cJSON_GetObjectItem(res_obj, "type");
    cJSON *cmds_obj = cJSON_GetObjectItem(res_obj, "commands");
    if (!cJSON_IsArray(cmds_obj)) {
        SM_LOGE("asr json has no commands");
        cJSON_Delete(res_obj);
        return 0;
    }

    const char *id_key_name = (cJSON_IsNumber(type_obj) && type_obj->valueint == 1) ? "intent_id" : "id";
    char tts_content[MAX_TTS_CONTENT_LEN];
    int list_nums = 0;
    int cmd_nums = cJSON_GetArraySize(cmds_obj);

    for (int i = 0; i < cmd_nums; i++) {
        cJSON *cmd_obj = cJSON_GetArrayItem(cmds_obj, i);
        cJSON *intent_id_obj = cJSON_GetObjectItem(cmd_obj, id_key_name);
        if (!cJSON_IsNumber(intent_id_obj)) {
            continue;
        }

        const IntentTtsInfo *intent = SpeechTts_GetIntentInfo(intent_id_obj->valueint);
        if (!intent) {
            continue;
        }

        for (int k = 0; k < intent->sub_tts_nums; k++) {
            if (list_nums >= MAX_SUB_TTS) {
                SM_LOGW("too many tts, drop the rest");
                cJSON_Delete(res_obj);
                return list_nums;
            }

            const TtsInfo *tts = &intent->tts[k];
            const char *tts_name = NULL;
            if (!tts->have_slot) {
                tts_name = tts->tts_name;
            } else {
                cJSON *slot_obj = cJSON_GetObjectItem(cJSON_GetObjectItem(cmd_obj, "slots"), tts->slot_name);
                if (cJSON_IsString(slot_obj)) {
                    snprintf(tts_content, sizeof(tts_content), "%s%s", slot_obj->valuestring, tts->suffix ? tts->suffix : "");
                    tts_name = SpeechTts_GetNameByContent(tts_content);
                }
            }

            if (tts_name) {
                tts_list[list_nums++] = tts_name;
            }
        }
    }

    cJSON_Delete(res_obj);
    return list_nums;
}

/* Barge-in: drop everything queued and play url as soon as possible. */
static void Preempt(PlaylistParser *p, const char *url)
{
    osal_mutex_lock(&p->lock);
    RingClear(p);
    p->preempt = true;
    strncpy(p->preempt_url, url, MAX_URL_LEN - 1);
    p->preempt_url[MAX_URL_LEN - 1] = '\0';
    osal_cond_signal(&p->cond);
    osal_mutex_unlock(&p->lock);
}

/* ---------- public API ---------- */

void PlaylistParser_WakeUp(PlaylistParser *parser)
{
    SM_LOGI("wake up");
    Preempt(parser, WAKE_UP_AUDIO_FILE);
}

void PlaylistParser_VadTimeout(PlaylistParser *parser)
{
    SM_LOGI("vad timeout");
    Preempt(parser, VAD_TIMEOUT_AUDIO_FILE);
}

void PlaylistParser_AsrMsg(PlaylistParser *parser, const char *json)
{
    const char *tts_list[MAX_SUB_TTS];
    char tts_url[MAX_URL_LEN];

    int tts_nums = ParseAsrIntent(json, tts_list);
    for (int i = 0; i < tts_nums; i++) {
        snprintf(tts_url, sizeof(tts_url), "vfs://tts/%s.mp3", tts_list[i]);
        PostCmd(parser, CMD_TTS, tts_url);

        if (strcmp(tts_list[i], PLAY_MUSIC_INTENT_ID) == 0) {
            PostCmd(parser, CMD_PLAY_MUSIC, MUSIC_URL);
        }
        if (strcmp(tts_list[i], STOP_MUSIC_INTENT_ID) == 0) {
            PostCmd(parser, CMD_STOP_MUSIC, NULL);
        }
    }
}

PlaylistParser *PlaylistParser_Create(void)
{
    PlaylistParser *p = (PlaylistParser *)osal_calloc(sizeof(PlaylistParser));
    if (!p) {
        SM_LOGE("malloc playlist parser fail");
        return NULL;
    }

    p->player = MediaPlayer_CreateEx(MEDIA_PLAYER_TYPE_PICO);
    if (!p->player) {
        SM_LOGE("create media player fail");
        osal_free(p);
        return NULL;
    }

    p->callback.OnStateChanged = OnStateChanged;
    p->callback.OnInfo = OnInfo;
    p->callback.OnError = OnError;
    MediaPlayer_SetCallback(p->player, &p->callback);

    osal_mutex_init(&p->lock);
    osal_cond_init(&p->cond);

    p->running = true;

    osal_thread_param param = {
        .name = "plp",
        .priority = OSAL_THREAD_PRI_NORMAL,
        .stack_size = WORKER_STACK_SIZE,
        .joinable = true,
    };
    if (osal_thread_create(&p->thread, WorkerLoop, p, &param) != OSAL_OK) {
        SM_LOGE("create worker thread fail");
        osal_cond_destroy(&p->cond);
        osal_mutex_destroy(&p->lock);
        MediaPlayer_Destory(p->player);
        osal_free(p);
        return NULL;
    }
    return p;
}

void PlaylistParser_Destroy(PlaylistParser *parser)
{
    if (!parser) {
        return;
    }

    osal_mutex_lock(&parser->lock);
    parser->running = false;
    osal_cond_signal(&parser->cond);
    osal_mutex_unlock(&parser->lock);
    osal_thread_join(parser->thread, NULL);

    if (parser->player) {
        MediaPlayer_Reset(parser->player);
        MediaPlayer_Destory(parser->player);
        parser->player = NULL;
    }

    osal_cond_destroy(&parser->cond);
    osal_mutex_destroy(&parser->lock);
    osal_free(parser);
}
