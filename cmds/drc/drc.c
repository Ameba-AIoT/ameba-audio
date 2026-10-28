/*
 * Copyright (c) 2026 Realtek, LLC.
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

/*
 * DRC (Dynamic Range Compression) test command.
 *
 * This command ONLY drives the framework AudioDrc_* interface. It does NOT
 * create an AudioRecord/AudioTrack of its own -- it attaches DRC to whatever
 * output effect chain is available. The natural partner is `arecord`, whose
 * mic->speaker loopback provides an active stream for DRC to work on
 * (enabling this cmd's menu also auto-selects arecord in Kconfig).
 *
 * Invocation model:
 *   drc [--mode 0|1]   enable DRC on the running stream; the cmd task exits
 *                      right after enabling, but the DRC stays live (the
 *                      handle is held in a file-scope global) until either
 *                      the underlying AudioTrack tears down or the user
 *                      runs `drc --off`.
 *   drc --off          disable + destroy the currently-active DRC.
 *
 * Startup order:
 *   - Mixer path: the mixer output thread is spawned eagerly during
 *     AudioService_Init (AudioPolicyImpl::Init -> CreateOutput), so
 *     GeneralAudio::CreateEffect succeeds immediately -- `drc` may start
 *     BEFORE or AFTER `arecord`. Same behavior as the equalizer_tune cmd.
 *   - Passthrough path: s_drc_module is created lazily inside
 *     AudioTrack_Start (see audio_stream/passthrough/audio_track.c), so
 *     `arecord` (or another AudioTrack-producing command) must be started
 *     BEFORE `drc`. Matches how the equalizer / spectrum cmds behave.
 *
 * Typical session on the serial console:
 *     arecord -c 2 -r 48000 -f 16      # start the loopback first
 *     drc --mode 0                     # enable single-band DRC
 *     drc --off                        # later: disable DRC (arecord keeps going)
 */

#define LOG_TAG "Drc"

#include <inttypes.h>

#include "ameba.h"
#include "os_wrapper.h"

#include "audio/audio_service.h"
#include "audio/audio_drc.h"
#include "common/audio_errnos.h"

#include "audio_cmd_common.h"

typedef struct {
    int mode;         /* AUDIO_DRC_MODE_SINGLE_BAND (0) or _MULTI_BANDS (1) */
    int off;          /* non-zero: teardown the currently-active DRC and exit */
} drc_params_t;

static const drc_params_t DRC_DEFAULT_PARAMS = {
    .mode = AUDIO_DRC_MODE_SINGLE_BAND,
    .off  = 0,
};

static drc_params_t s_params;

/* The DRC lives longer than a single cmd invocation: `drc` enables it, then
 * the task exits with the handle parked here so a later `drc --off` can find
 * and tear it down. NULL means "no DRC currently active". */
static struct AudioDrc *g_active_drc = NULL;

/* Single-band default: broadband compressor, 4 knees, 0dB makeup. */
static const struct AudioDrcConfig DRC_CFG_SINGLE = {
    .num_of_knee_point = 2,
    .attack_time       = 0.005f,
    .release_time      = 0.08f,
    .makeup_gain       = 3.0f,
    .knee_point = {
        { -24.0f, 2.0f },
        { -6.0f, 8.0f },
    },
};

/* Three-band default: low/mid/high compressors split at 300 Hz / 5 kHz.
 * Matches the audio_test/drc/audio_drc_config.c reference. */
static const struct AudioMultiBandsDrcConfig DRC_CFG_MULTI = {
    .split_freq    = { .fc_lo = 300, .fc_hi = 5000 },
    .drc_band_low  = {
        4, 0.01f, 0.05f, 0.0f,
        { { -40.0f, 2.0f }, { -30.0f, 4.0f }, { -12.0f, 6.0f }, { -6.0f, 8.0f } },
    },
    .drc_band_mid  = {
        4, 0.01f, 0.05f, 0.0f,
        { { -55.0f, 1.5f }, { -40.0f, 2.0f }, { -25.0f, 4.0f }, { -15.0f, 8.0f } },
    },
    .drc_band_high = {
        4, 0.01f, 0.05f, 0.0f,
        { { -50.0f, 2.0f }, { -30.0f, 4.0f }, { -12.0f, 5.0f }, {  -6.0f, 6.0f } },
    },
};

static void DrcTask(void *param)
{
    drc_params_t *p = (drc_params_t *)param;
    struct AudioDrc *drc = NULL;

    AudioService_Init();

    if (p->off) {
        if (!g_active_drc) {
            RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS, "no active drc to disable\n");
            goto done;
        }
        AudioDrc_SetEnabled(g_active_drc, false);
        AudioDrc_Destroy(g_active_drc);
        g_active_drc = NULL;
        RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS, "drc disabled and destroyed\n");
        goto done;
    }

    if (g_active_drc) {
        RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS,
                 "drc already active; run 'drc --off' first to reconfigure\n");
        goto done;
    }

    drc = AudioDrc_Create();
    if (!drc) {
        RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS, "AudioDrc_Create failed\n");
        goto done;
    }

    if (AudioDrc_Init(drc, 0, 0) != AUDIO_OK) {
        RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS, "AudioDrc_Init failed\n");
        AudioDrc_Destroy(drc);
        goto done;
    }

    /* Mode MUST be set before SetEnabled(true); the framework rejects mode
     * changes on an enabled DRC. */
    if (AudioDrc_SetMode(drc, p->mode) != AUDIO_OK) {
        RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS,
                 "AudioDrc_SetMode(%d) failed, is any AudioTrack running?\n", p->mode);
        AudioDrc_Destroy(drc);
        goto done;
    }

    if (p->mode == AUDIO_DRC_MODE_MULTI_BANDS) {
        if (AudioDrc_SetMultiBandsConfig(drc, &DRC_CFG_MULTI) != AUDIO_OK) {
            RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS, "AudioDrc_SetMultiBandsConfig failed\n");
            AudioDrc_Destroy(drc);
            goto done;
        }
        RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS,
                 "drc: multi-band, fc_lo=%dHz fc_hi=%dHz\n",
                 (int)DRC_CFG_MULTI.split_freq.fc_lo,
                 (int)DRC_CFG_MULTI.split_freq.fc_hi);
    } else {
        if (AudioDrc_SetConfig(drc, &DRC_CFG_SINGLE) != AUDIO_OK) {
            RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS, "AudioDrc_SetConfig failed\n");
            AudioDrc_Destroy(drc);
            goto done;
        }
        RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS,
                 "drc: single-band, %d knees, makeup=%dcB\n",
                 (int)DRC_CFG_SINGLE.num_of_knee_point,
                 (int)(DRC_CFG_SINGLE.makeup_gain * 100.0f));
    }

    if (AudioDrc_SetEnabled(drc, true) != AUDIO_OK) {
        RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS, "AudioDrc_SetEnabled failed\n");
        AudioDrc_Destroy(drc);
        goto done;
    }

    g_active_drc = drc;
    RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS, "drc enabled; run 'drc --off' to disable\n");

done:
    rtos_task_delete(NULL);
}

static cmd_parse_entry_t s_drc_entries[] = {
    {"--mode", NULL, DRC_DEFAULT_PARAMS.mode},
    {"--off",  NULL, DRC_DEFAULT_PARAMS.off},
};

static void drc_help(void)
{
    RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS, "drc [OPTION...] \n"
        "\tStart an AudioTrack-producing command (e.g. `arecord`) FIRST,\n"
        "\tthen run this to enable DRC on the running stream. Later run\n"
        "\t`drc --off` to disable it.\n"
        "\t-h, --help              show this help message\n");

    int entry_count = sizeof(s_drc_entries) / sizeof(s_drc_entries[0]);
    for (int i = 0; i < entry_count; i++) {
        RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS, "\t%-25s (default: %d)\n",
            s_drc_entries[i].arg, s_drc_entries[i].default_value);
    }

    RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS,
        "\t--mode 0                single-band DRC (default)\n"
        "\t--mode 1                three-band DRC\n"
        "\t--off 1                 disable + destroy the current DRC\n"
        "\nExamples:\n"
        "\tdrc                     # single-band DRC\n"
        "\tdrc --mode 1            # three-band DRC\n"
        "\tdrc --off 1             # tear down the currently-active DRC\n");
}

static void parse_drc_params(cmd_params_t *params, drc_params_t *p)
{
    *p = DRC_DEFAULT_PARAMS;

    cmd_parse_entry_t entries[] = {
        {"--mode", &p->mode, DRC_DEFAULT_PARAMS.mode},
        {"--off",  &p->off,  DRC_DEFAULT_PARAMS.off},
    };

    int entry_count = sizeof(entries) / sizeof(entries[0]);
    cmd_parse_all_int(params, entries, entry_count);

    if (p->mode != AUDIO_DRC_MODE_SINGLE_BAND && p->mode != AUDIO_DRC_MODE_MULTI_BANDS) {
        RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS,
                 "invalid --mode:%d, falling back to single-band\n", p->mode);
        p->mode = AUDIO_DRC_MODE_SINGLE_BAND;
    }
}

static uint32_t drc_handler(cmd_params_t *params)
{
    if (params->argc > 1 &&
        (strcmp(params->argv[1], "-h") == 0 || strcmp(params->argv[1], "--help") == 0)) {
        drc_help();
        return TRUE;
    }

    parse_drc_params(params, &s_params);

    RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS, "create drc example \n");

    rtos_task_t drc_task;
    if (rtos_task_create(&drc_task, "DrcTask",
                        DrcTask, &s_params,
                        4096, 5) != RTK_SUCCESS) {
        RTK_LOGS(LOG_TAG, RTK_LOG_ALWAYS,"error: rtos_task_create(DrcTask) failed \n");
        return FALSE;
    }

    return TRUE;
}

DEFINE_CMD_WRAPPER(drc, drc_handler, 5);

CMD_TABLE_DATA_SECTION
const COMMAND_TABLE drc_cmd_table[] = {
    {
        "drc", drc_cmd_thread
    },
};
