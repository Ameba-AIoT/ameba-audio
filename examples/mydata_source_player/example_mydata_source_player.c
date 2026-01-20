#define LOG_TAG "MyDataSourceTest"

#include "platform_stdlib.h"
#include "basic_types.h"
#include "audio/audio_service.h"

#include "media/media_player.h"
#include "mydata_source.h"
#include "example_mydata_source_player.h"

#include "os_wrapper.h"

/* source data */
#include "48k_2c_30s_mp3.h"

#define USE_PREPARE_ASYNC

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

struct MediaPlayer *g_mds_player;
int g_mds_playing_status = IDLE;

void OnMDSPlayerStateChanged(const struct MediaPlayerCallback *listener, const struct MediaPlayer *player, int state)
{
    printf("OnMDSPlayerStateChanged(%p %p), (%d)\n", listener, player, state);

    switch (state) {
    case MEDIA_PLAYER_PREPARED: { //entered for async prepare
        g_mds_playing_status = PREPARED;
        break;
    }

    case MEDIA_PLAYER_PLAYBACK_COMPLETE: { //eos received, then stop
        g_mds_playing_status = PLAYING_COMPLETED;
        break;
    }

    case MEDIA_PLAYER_STOPPED: { //stop received, then reset
        printf("start reset\n");
        g_mds_playing_status = STOPPED;
        break;
    }

    case MEDIA_PLAYER_PAUSED: { //pause received when do pause or start rewinding
        printf("paused\n");
        g_mds_playing_status = PAUSED;
        break;
    }

    case MEDIA_PLAYER_REWIND_COMPLETE: { //rewind done received, then start
        printf("rewind complete\n");
        g_mds_playing_status = REWIND_COMPLETE;
        break;
    }
    }
}

void OnMDSPlayerInfo(const struct MediaPlayerCallback *listener, const struct MediaPlayer *player, int info, int extra)
{
    printf("OnMDSPlayerInfo (%p %p), (%d, %d)\n", listener, player, info, extra);

    switch (info) {
    case MEDIA_PLAYER_INFO_BUFFERING_START: {
        printf("MEDIA_PLAYER_INFO_BUFFERING_START\n");
        break;
    }

    case MEDIA_PLAYER_INFO_BUFFERING_END: {
        printf("MEDIA_PLAYER_INFO_BUFFERING_END\n");
        break;
    }

    case MEDIA_PLAYER_INFO_BUFFERING_INFO_UPDATE: {
        printf("MEDIA_PLAYER_INFO_BUFFERING_INFO_UPDATE %d\n", extra);
        break;
    }

    case MEDIA_PLAYER_INFO_NOT_REWINDABLE: {
        printf("MEDIA_PLAYER_INFO_NOT_REWINDABLE\n");
        break;
    }
    }
}

void OnMDSPlayerError(const struct MediaPlayerCallback *listener, const struct MediaPlayer *player, int error, int extra)
{
    printf("OnMDSPlayerError (%p %p), (%d, %d)\n", player, listener, error, extra);
    g_mds_playing_status = IDLE;
}

int mydata_source_test(char *source, int length)
{
    AudioService_Init();
    printf("AudioService_Init done\n");

    StreamSource *data_source = MyDataSource_Create(source, length);

    struct MediaPlayerCallback *callback = (struct MediaPlayerCallback *)calloc(1, sizeof(struct MediaPlayerCallback));
    if (!callback) {
        printf("Calloc MediaPlayerCallback fail.\n");
        return -1;
    }

    callback->OnMediaPlayerStateChanged = OnMDSPlayerStateChanged;
    callback->OnMediaPlayerInfo = OnMDSPlayerInfo;
    callback->OnMediaPlayerError = OnMDSPlayerError;

    g_mds_player = MediaPlayer_Create();

    MediaPlayer_SetCallback(g_mds_player, callback);

    int32_t ret = 0;
    g_mds_playing_status = IDLE;

    printf("MediaPlayer_SetDataSource: %p\n", data_source);

    ret = MediaPlayer_SetDataSource(g_mds_player, data_source);
    if (ret) {
        printf("MediaPlayer_SetDataSource fail:error=%ld\n", ret);
        goto exit;
    }

#ifdef USE_PREPARE_ASYNC
    ret = MediaPlayer_PrepareAsync(g_mds_player);
    if (ret) {
        printf("prepare async fail:error=%ld\n", ret);
        goto exit;
    }

    g_mds_playing_status = PREPARING;

    while (g_mds_playing_status != PREPARED) {
        rtos_time_delay_ms(20);
        if (g_mds_playing_status == IDLE) {
            printf("player1 not prepared, now goto exit!\n");
            goto exit;
        }
    }
#else
    ret = MediaPlayer_Prepare(g_mds_player);
    if (ret) {
        printf("prepare fail:error=%d\n", ret);
        goto exit;
    }
#endif

    printf("prepared\n");

    ret = MediaPlayer_Start(g_mds_player);
    if (ret) {
        printf("start fail:error=%ld\n", ret);
        goto exit;
    }

    g_mds_playing_status = PLAYING;

    while (g_mds_playing_status == PLAYING || g_mds_playing_status == PAUSED) {
        rtos_time_delay_ms(1000);
    }

    if (g_mds_playing_status == PLAYING_COMPLETED || g_mds_playing_status == IDLE) {
        printf("play complete, now stop.\n");
        MediaPlayer_Stop(g_mds_player);
        printf("play stop done.\n");
    }

    while (g_mds_playing_status == PLAYING_COMPLETED) {
        rtos_time_delay_ms(1000);
    }

    if (g_mds_playing_status == STOPPED) {
        printf("play stopped, now reset.\n");
        MediaPlayer_Reset(g_mds_player);
        printf("play reset done.");
    }

    printf("play %p done!!!!\n", data_source);

exit:
    printf("goto exit\n");

    if (data_source) {
        MyDataSource_Destroy((MyDataSource *)data_source);
        data_source = NULL;
    }

    if (g_mds_player) {
        MediaPlayer_Destory(g_mds_player);
        g_mds_player = NULL;
    }

    free(callback);

    rtos_time_delay_ms(1000);

    printf("exit\n");
    return 0;
}

#ifdef CMD_TEST
#define MAX_URL_SIZE 1024
void example_mydata_source_player_test_args_handle(u8  *argv[])
{
    (void) argv;
    printf("mydata source player test start......\n");

    mydata_source_test((char *)ready_to_convert0, sizeof(ready_to_convert0));
    rtos_time_delay_ms(1 * 1000);

    printf("mydata source player test done......\n");
    printf("\n\n");

    return;
}

u32 example_mydata_source_player_test(u16 argc, u8 *argv[])
{
    (void) argc;
    example_mydata_source_player_test_args_handle(argv);
    return TRUE;
}
#endif

void example_mydata_source_player_thread(void *param)
{
    (void) param;

    printf("mydata source player test start......\n");

    mydata_source_test((void *)ready_to_convert0, sizeof(ready_to_convert0));
    rtos_time_delay_ms(1 * 1000);

    printf("mydata source player test done......\n");
    printf("\n\n");

    rtos_task_delete(NULL);
}

void example_mydata_source_player(void)
{
    if (rtos_task_create(NULL, ((const char *)"example_mydata_source_player_thread"), example_mydata_source_player_thread, NULL, 10 * 1024,
                         1) != RTK_SUCCESS) {
        printf("\n\r%s rtos_task_create(example_mydata_source_player_thread) failed", __FUNCTION__);
    }
}
