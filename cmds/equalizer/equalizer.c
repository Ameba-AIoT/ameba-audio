/*
 * Copyright (c) 2025 Realtek, LLC.
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

#define LOG_TAG "Equalizer"

#include <math.h>
#include "ameba.h"
#include "os_wrapper.h"
#include "ameba_audio_mixer_usrcfg.h"
#include "audio/audio_control.h"
#include "audio/audio_record.h"
#include "audio/audio_service.h"
#include "audio/audio_track.h"
#include "audio/audio_equalizer.h"
#include "common/audio_errnos.h"

#include "audio_cmd_common.h"

#define MAX_EQUALIZER_INSTANCES 1

typedef struct {
    int channel;
    int rate;
    int format;
    int bytes_one_time;
    int device;
    int duration;
    int pressure_test;

    int eq;
} eq_params_t;

static const eq_params_t EQ_DEFAULT_PARAMS = {
    .channel        = 1,
    .rate           = 48000,
    .format         = 16,
    .bytes_one_time = 960,
    .device         = DEVICE_IN_MIC,
    .duration       = 86400,
    .pressure_test  = 0,
    .eq            = 0,
};

static eq_params_t s_params[MAX_EQUALIZER_INSTANCES];
static int s_cfg_cnt = 0;

static struct AudioEqualizer *g_audio_equalizer = NULL;

static int32_t GetFormatForBits(int format_bits)
{
    int32_t format;
    switch (format_bits) {
    case 16:
        format = AUDIO_FORMAT_PCM_16_BIT;
        break;
    case 24:
        format = AUDIO_FORMAT_PCM_24_BIT;
        break;
    case 32:
        format = AUDIO_FORMAT_PCM_32_BIT;
        break;
    default:
        RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS,"format:%d not supportd \n", format_bits);
        return AUDIO_FORMAT_INVALID;
    }
    return format;
}

static void equalizer_dump(void)
{
    RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS,"dump eq param, total bands:%d", 10);
    for (int i = 0; i < 10; i++) {
        int16_t level = AudioEqualizer_GetBandLevel(g_audio_equalizer, i);
        int16_t filter_type = AudioEqualizer_GetBandFilterType(g_audio_equalizer, i);
        int32_t fc = AudioEqualizer_GetCenterFreq(g_audio_equalizer, i);
        int16_t q = AudioEqualizer_GetQfactor(g_audio_equalizer, i);
        RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS,"band[%d] enable:1 filter:%d fc:%d, q:%d, gain:%d \n",
                    i, filter_type, fc, q, level);
    }
    RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS,"dump end");
}

static void equalizer_create(void)
{
    if (g_audio_equalizer) {
        RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS,"eq already created\n");
        return;
    }

    RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS,"eq version:%d \n", kEqVersion);

    g_audio_equalizer = AudioEqualizer_Create();
    if (!g_audio_equalizer) {
        RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS,"AudioEqualizer_Create failed\n");
        return;
    }

    AudioEqualizer_Init(g_audio_equalizer, 0, 0);

    int16_t bands = AudioEqualizer_GetNumberOfBands(g_audio_equalizer);
    RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS,"default bands: %d\n", bands);

    int16_t *range = AudioEqualizer_GetBandLevelRange(g_audio_equalizer);
    RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS,"band range:(%d, %d) \n", range[0], range[1]);
    free(range);

    bands = 10;
    AudioEqualizer_SetNumberOfBands(g_audio_equalizer, bands);

    // fc: 40hz, 90hz, 180hz...
    int32_t fc[10] = {40, 90, 180, 380, 760, 1000, 3020, 6010, 12010, 18010};
    // db: -2db, +4db, -1db...
    int32_t gain[10] = {-200, 400, -100, 300, -800, 800, -200, 300, -100, 200};
    // q: 0.7, 0.96, 0.6...
    int32_t qfactor_arr[10] = {70, 96, 60, 70, 60, 50, 50, 66, 40, 70};
    int32_t type[10] = {[0 ... 9] = AUDIO_EQUALIZER_TYPE_PEAKING};
    type[0] = AUDIO_EQUALIZER_TYPE_HIGHPASS;
    type[9] = AUDIO_EQUALIZER_TYPE_LOWPASS;

    for (int i = 0; i < bands; i++) {
        AudioEqualizer_SetCenterFreq(g_audio_equalizer, i, fc[i]);
        AudioEqualizer_SetBandLevel(g_audio_equalizer, i, gain[i]);
        AudioEqualizer_SetQfactor(g_audio_equalizer, i, qfactor_arr[i]);
        AudioEqualizer_SetBandFilterType(g_audio_equalizer, i, type[i]);
    }

    AudioEqualizer_SetEnabled(g_audio_equalizer, true);
}

static void equalizer_destroy(void)
{
    if (g_audio_equalizer) {
        AudioEqualizer_SetEnabled(g_audio_equalizer, false);
        AudioEqualizer_Destroy(g_audio_equalizer);
        g_audio_equalizer = NULL;
    }
    RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS,"eq disabled\n");
}

static uint32_t Record_Sample(eq_params_t *param)
{
    int8_t *buffer;
    int8_t *out_buffer;
    uint32_t size;
    ssize_t size_read;
    uint32_t flags = AUDIO_OUTPUT_FLAG_NONE;
    int64_t bytes_read = 0;
    uint32_t frame_bytes = param->channel * param->format / 8;
    int32_t format = GetFormatForBits(param->format);
    int64_t record_size = (int64_t)param->rate * (int64_t)param->duration * (int64_t)param->channel * (int64_t)param->format / (int64_t)8;
    int32_t track_buf_size = 4096;

    if (format == (int32_t)AUDIO_FORMAT_INVALID) {
        RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS,"invalid record format bits:%d \n", param->format);
        return 0;
    }

    struct AudioRecord *audio_record;
    audio_record = AudioRecord_Create();
    if (!audio_record) {
        RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS,"record create failed \n");
        return 0;
    }
    AudioRecordConfig record_config;
    record_config.sample_rate = param->rate;
    record_config.format = format;
    record_config.channel_count = param->channel;
    record_config.device = param->device;
    record_config.buffer_bytes = 0;
    AudioRecord_Init(audio_record, &record_config, flags);
    AudioRecord_Start(audio_record);

    struct AudioTrack *audio_track;
    audio_track = AudioTrack_Create();
    if (!audio_track) {
        RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS,"new AudioTrack failed, destroy record \n");
        AudioRecord_Destroy(audio_record);
        return 0;
    }
    track_buf_size = AudioTrack_GetMinBufferBytes(audio_track, AUDIO_CATEGORY_MEDIA, param->rate, format, param->channel) * 4;
    AudioTrackConfig  track_config;
    track_config.category_type = AUDIO_CATEGORY_MEDIA;
    track_config.sample_rate = param->rate;
    track_config.format = format;
    track_config.channel_count = param->channel;
    track_config.buffer_bytes = track_buf_size;
    AudioTrack_Init(audio_track, &track_config, flags);
    AudioTrack_Start(audio_track);

    size = param->bytes_one_time;
    buffer = (int8_t *) malloc(size);
    if (!buffer) {
        RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS,"failed to malloc buffer \n");
        return 0;
    }

    out_buffer = (int8_t *) malloc(size);
    if (!out_buffer) {
        free(buffer);
        RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS,"failed to malloc out_buffer \n");
        return 0;
    }

    RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS,"Capturing sample: %d ch, %d hz, record bytes one time:%d (frames:%d frames*channels:%d) \n",
                        param->channel, param->rate, param->bytes_one_time,
                        size / frame_bytes, size / frame_bytes * param->channel);

    if (param->eq) {
        //for passthrough, must create after track start.
        //for mixer, no limit.
        equalizer_create();
        equalizer_dump();
    }

    do {
        size_read = AudioRecord_Read(audio_record, buffer, size, true);
        if ((uint32_t)size_read != size) {
            RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS,"opps size wanted:%d, size actually read:%d\n",
                size, size_read);
        }

        memcpy(out_buffer, buffer, size);

        if (bytes_read >= 100 * param->rate * param->channel * param->format / 8 / 1000) {
            AudioTrack_Write(audio_track, out_buffer, size, true);
        } else {
            memset(out_buffer, 0, size);
            AudioTrack_Write(audio_track, out_buffer, size, true);
            AudioTrack_Write(audio_track, out_buffer, size, true);
        }

        bytes_read += size;

        if (!param->pressure_test) {
            if (bytes_read >= record_size) {
                break;
            }
        }

    } while (1);

    uint32_t frames = bytes_read / (param->channel * param->format / 8);

    free(buffer);
    buffer = NULL;
    free(out_buffer);
    out_buffer = NULL;

    AudioRecord_Stop(audio_record);
    AudioRecord_Destroy(audio_record);
    AudioTrack_Stop(audio_track);
    AudioTrack_Destroy(audio_track);

    if (param->eq) {
        equalizer_destroy();
    }

    return frames;
}

static void EqualizerTask(void *param)
{
    uint32_t frames;
    eq_params_t *params = (eq_params_t *)param;

    AudioService_Init();
    frames = Record_Sample(params);
    rtos_time_delay_ms(2 * RTOS_TICK_RATE_HZ);

    RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS,"Recorded %lu frames \n", frames);
    rtos_task_delete(NULL);
}

static cmd_parse_entry_t s_equalizer_entries[] = {
    /* Lowercase short + long */
    {"-c",        NULL,   EQ_DEFAULT_PARAMS.channel},
    {"--channels",NULL,   EQ_DEFAULT_PARAMS.channel},
    {"-r",        NULL,   EQ_DEFAULT_PARAMS.rate},
    {"--rate",    NULL,   EQ_DEFAULT_PARAMS.rate},
    {"-f",        NULL,   EQ_DEFAULT_PARAMS.format},
    {"--format",  NULL,   EQ_DEFAULT_PARAMS.format},
    {"-b",        NULL,   EQ_DEFAULT_PARAMS.bytes_one_time},
    {"--buffer",  NULL,   EQ_DEFAULT_PARAMS.bytes_one_time},
    {"-d",        NULL,   EQ_DEFAULT_PARAMS.duration},
    {"--duration",NULL,   EQ_DEFAULT_PARAMS.duration},
    {"-p",        NULL,   EQ_DEFAULT_PARAMS.pressure_test},
    {"--pres",    NULL,   EQ_DEFAULT_PARAMS.pressure_test},
    /* Long only */
    {"--device",  NULL,   EQ_DEFAULT_PARAMS.device},
};

static void equalizer_help(void)
{
    RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS, "equalizer [OPTION...] \n"
        "\t-h, --help              show this help message\n");

    int entry_count = sizeof(s_equalizer_entries) / sizeof(s_equalizer_entries[0]);
    for (int i = 0; i < entry_count; i++) {
        RTK_LOGI(LOG_TAG, "\t%-25s (default: %d)\n",
            s_equalizer_entries[i].arg, s_equalizer_entries[i].default_value);
    }

    RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS,
        "\nExamples:\n"
        "\tequalizer -c 1 -r 48000 -g -800\n"
        "\tequalizer -c 2 -r 44100 -f 16 -g -400\n"
        "\tequalizer -d 20 --device 134217729\n");
}

static void parse_equalizer_params(cmd_params_t *params, eq_params_t *p)
{
    *p = EQ_DEFAULT_PARAMS;

    cmd_parse_entry_t entries[] = {
        {"-c",        &p->channel,        EQ_DEFAULT_PARAMS.channel},
        {"--channels",&p->channel,        EQ_DEFAULT_PARAMS.channel},
        {"-r",        &p->rate,           EQ_DEFAULT_PARAMS.rate},
        {"--rate",    &p->rate,           EQ_DEFAULT_PARAMS.rate},
        {"-f",        &p->format,         EQ_DEFAULT_PARAMS.format},
        {"--format",  &p->format,         EQ_DEFAULT_PARAMS.format},
        {"-b",        &p->bytes_one_time, EQ_DEFAULT_PARAMS.bytes_one_time},
        {"--buffer",  &p->bytes_one_time, EQ_DEFAULT_PARAMS.bytes_one_time},
        {"-d",        &p->duration,       EQ_DEFAULT_PARAMS.duration},
        {"--duration",&p->duration,       EQ_DEFAULT_PARAMS.duration},
        {"-p",        &p->pressure_test,  EQ_DEFAULT_PARAMS.pressure_test},
        {"-e",        &p->eq,             EQ_DEFAULT_PARAMS.eq},
        {"--eq",      &p->eq,             EQ_DEFAULT_PARAMS.eq},
        {"--pres",    &p->pressure_test,  EQ_DEFAULT_PARAMS.pressure_test},
        {"--device",  &p->device,         EQ_DEFAULT_PARAMS.device},
    };

    int entry_count = sizeof(entries) / sizeof(entries[0]);
    cmd_parse_all_int(params, entries, entry_count);
}

static uint32_t equalizer_handler(cmd_params_t *params)
{
    if (params->argc <= 1) {
        equalizer_help();
        return TRUE;
    }

    if (s_cfg_cnt >= MAX_EQUALIZER_INSTANCES) {
        RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS, "support less than %d instances \n", MAX_EQUALIZER_INSTANCES);
        return FALSE;
    }

    parse_equalizer_params(params, &s_params[s_cfg_cnt]);

    RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS, "create equalizer example \n");

    rtos_task_t equalizer_task;
    if (rtos_task_create(&equalizer_task, "EqualizerTask",
                        EqualizerTask, &s_params[s_cfg_cnt],
                        4096, 5) != RTK_SUCCESS) {
        RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS,"error: rtos_task_create(EqualizerTask) failed \n");
        return FALSE;
    }

    s_cfg_cnt++;

    return TRUE;
}

DEFINE_CMD_WRAPPER(equalizer, equalizer_handler, 5);

CMD_TABLE_DATA_SECTION
const COMMAND_TABLE equalizer_cmd_table[] = {
    {
        "equalizer", equalizer_cmd_thread
    },
};
