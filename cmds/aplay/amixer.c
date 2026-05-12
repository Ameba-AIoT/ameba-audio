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

typedef struct {
    float      volume;  // 0.0 ~ 1.0
    int32_t    mute;    // 0/1
} amixer_params_t;

static const amixer_params_t AMIXER_DEFAULT_PARAMS = {
    .volume = 1.0f,
    .mute   = 0,
};

static float     s_vol = 0.6;
static uint32_t  s_mute = 0;

void example_track_control_thread(void *param)
{
    (void) param;
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
    CMD_PARSE_FLOAT(p->volume, "-v", AMIXER_DEFAULT_PARAMS.volume);
    CMD_PARSE_INT(p->mute,     "-m", AMIXER_DEFAULT_PARAMS.mute);
}

static void amixer_help(void)
{
    RTK_LOGI(TAG, "amixer [OPTION...]\n"
        "\t\t test cmd: amixer [-v] volume [-m] mute\n");
}

static uint32_t amixer_handler(cmd_params_t *params)
{
    if (params->argc <= 1) {
        amixer_help();
        return TRUE;
    }

    amixer_params_t p;
    parse_amixer_params(params, &p);

    s_vol  = p.volume;
    s_mute = p.mute;

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