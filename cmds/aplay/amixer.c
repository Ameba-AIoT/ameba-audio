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

#define TAG "Amixer"

#include <stdlib.h>
#include <math.h>

#include "ameba_audio_mixer_usrcfg.h"

#include "ameba.h"
#include "os_wrapper.h"

#include "audio/audio_control.h"
#include "audio/audio_service.h"
#include "common/audio_errnos.h"

#include "audio_cmd_common.h"

/* Sentinel outside the valid [-65.625, 0] dB range; means "don't touch max volume". */
#define AMIXER_MAX_VOL_SKIP    1.0f

typedef struct {
    float      volume;         // 0.0 ~ 1.0
    int32_t    mute;           // 0/1
    float      max_volume_db;  // -65.625 ~ 0 dB, or AMIXER_MAX_VOL_SKIP to skip
} amixer_params_t;

static const amixer_params_t AMIXER_DEFAULT_PARAMS = {
    .volume        = 1.0f,
    .mute          = 0,
    .max_volume_db = AMIXER_MAX_VOL_SKIP,
};

static float     s_vol = 0.6;
static uint32_t  s_mute = 0;
static float     s_max_vol_db = AMIXER_MAX_VOL_SKIP;

void example_track_control_thread(void *param)
{
    (void) param;

    if (s_max_vol_db <= 0.0f && s_max_vol_db >= -65.625f) {
        int32_t ret = AudioControl_SetMaxHardwareVolume(s_max_vol_db);
        RTK_LOGS(TAG, RTK_LOG_ALWAYS, "set max hw volume %f dB, ret:%ld \n", s_max_vol_db, ret);
    }

    AudioControl_SetHardwareVolume(s_vol, s_vol);
    AudioControl_SetAmplifierMute(s_mute);

    float left, right;
    bool muted;

    AudioControl_GetHardwareVolume(&left, &right);
    muted = AudioControl_GetAmplifierMute();
    RTK_LOGS(TAG, RTK_LOG_ALWAYS, "amp vol:%f %f, muted:%d \n", left, right, muted);

    rtos_task_delete(NULL);
}

static void parse_amixer_params(cmd_params_t *params, amixer_params_t *p)
{
    *p = AMIXER_DEFAULT_PARAMS;
    CMD_PARSE_FLOAT(p->volume,        "-v", AMIXER_DEFAULT_PARAMS.volume);
    CMD_PARSE_INT(p->mute,            "-m", AMIXER_DEFAULT_PARAMS.mute);
    CMD_PARSE_FLOAT(p->max_volume_db, "-M", AMIXER_DEFAULT_PARAMS.max_volume_db);
}

static void amixer_help(void)
{
    RTK_LOGI(TAG, "amixer [OPTION...]\n"
        "\t\t test cmd: amixer [-v] volume [-m] mute [-M] max_volume_db\n"
        "\t\t   -v <0.0~1.0>          relative volume, default 1.0\n"
        "\t\t   -m <0|1>              amplifier mute, default 0\n"
        "\t\t   -M <-65.625~0.0 dB>   full-scale DAC volume in dB (0.375 dB step), optional\n");
}

static uint32_t amixer_handler(cmd_params_t *params)
{
    if (params->argc <= 1) {
        amixer_help();
        return TRUE;
    }

    amixer_params_t p;
    parse_amixer_params(params, &p);

    s_vol        = p.volume;
    s_mute       = p.mute;
    s_max_vol_db = p.max_volume_db;

    if (rtos_task_create(NULL, "example_track_control_thread",
                        example_track_control_thread,
                        NULL, 1024, 1) != RTK_SUCCESS) {
        RTK_LOGS(TAG, RTK_LOG_ALWAYS, "error: rtos_task_create(example_track_control_thread) failed");
        return FALSE;
    }

    return TRUE;
}

DEFINE_CMD_WRAPPER(amixer, amixer_handler, 5);

CMD_TABLE_DATA_SECTION
const COMMAND_TABLE amixer_cmd_table[] = {
    {
        "amixer", amixer_cmd_thread
    },
};