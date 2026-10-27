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

/**
 * @file primary_audio_hw_vad.c
 * @brief amebasmart implementation of the AudioHwVad vtable from
 *        interfaces/hardware/audio/audio_hw_vad.h. The vtable mirrors the
 *        AudioVad framework calls one-to-one and forwards each into the
 *        low-level platform API in ameba_audio_vad.{c,h}. This split keeps
 *        ameba_audio_vad.c free of any HAL/vtable boilerplate, the same way
 *        primary_audio_hw_stream_in.c sits on top of ameba_audio_stream_capture.c.
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "audio_hw_debug.h"
#include "audio_hw_osal_errnos.h"
#include "hardware/audio/audio_hw_vad.h"

#include "ameba_audio_vad.h"

/*----------------------------------------------------------------------------
 * Singleton state
 *----------------------------------------------------------------------------*/

struct PrimaryAudioHwVad {
    struct AudioHwVad        ops;
    struct AmebaAudioVad    *lower;
    bool                     initialised;
    bool                     running;
};

static struct PrimaryAudioHwVad *s_primary = NULL;

/*----------------------------------------------------------------------------
 * Helpers
 *----------------------------------------------------------------------------*/

static void primary_audio_hw_vad_translate_cfg(const struct AudioHwVadConfig *src,
        struct AmebaAudioVadConfig *dst)
{
    memset(dst, 0, sizeof(*dst));
    dst->mic_index             = src->mic_index;
    dst->channel_count         = src->channel_count;
    dst->is_dmic               = (src->device == AUDIO_HW_DEVICE_IN_DMIC_REF_AMIC);
    dst->det_mv_threshold      = src->det_mv_threshold;
    dst->det_od_threshold      = src->det_od_threshold;
}

static int parse_uint_kv(const char *strs, const char *key, uint32_t *out)
{
    const char *p = strstr(strs, key);
    if (!p) {
        return 0;
    }
    p += strlen(key);
    if (*p != '=') {
        return 0;
    }
    p++;
    *out = (uint32_t)strtoul(p, NULL, 0);
    return 1;
}

/*----------------------------------------------------------------------------
 * Vtable ops — each one is a thin forwarder onto ameba_audio_vad_*.
 *----------------------------------------------------------------------------*/

static int32_t primary_audio_hw_vad_init(struct AudioHwVad *base, const struct AudioHwVadConfig *cfg)
{
    struct PrimaryAudioHwVad *self = (struct PrimaryAudioHwVad *)base;
    if (!self || !cfg) {
        return HAL_OSAL_ERR_NO_INIT;
    }
    if (self->initialised) {
        HAL_AUDIO_ERROR("PrimaryAudioHwVad: already initialised");
        return HAL_OSAL_ERR_INVALID_OPERATION;
    }

    struct AmebaAudioVadConfig low_cfg;
    primary_audio_hw_vad_translate_cfg(cfg, &low_cfg);

    self->lower = ameba_audio_vad_open(&low_cfg);
    if (!self->lower) {
        return HAL_OSAL_ERR_NO_MEMORY;
    }

    self->initialised = true;
    return HAL_OSAL_OK;
}

static int32_t primary_audio_hw_vad_start(struct AudioHwVad *base)
{
    struct PrimaryAudioHwVad *self = (struct PrimaryAudioHwVad *)base;
    if (!self || !self->initialised) {
        return HAL_OSAL_ERR_NO_INIT;
    }
    int32_t ret = ameba_audio_vad_arm(self->lower);
    if (ret == HAL_OSAL_OK) {
        self->running = true;
    }
    return ret;
}

static int32_t primary_audio_hw_vad_stop(struct AudioHwVad *base)
{
    struct PrimaryAudioHwVad *self = (struct PrimaryAudioHwVad *)base;
    if (!self || !self->initialised) {
        return HAL_OSAL_ERR_NO_INIT;
    }
    int32_t ret = ameba_audio_vad_disarm(self->lower);
    if (ret == HAL_OSAL_OK) {
        self->running = false;
    }
    return ret;
}

static void primary_audio_hw_vad_deinit(struct AudioHwVad *base)
{
    struct PrimaryAudioHwVad *self = (struct PrimaryAudioHwVad *)base;
    if (!self || !self->initialised) {
        return;
    }

    ameba_audio_vad_close(self->lower);
    self->lower       = NULL;
    self->initialised = false;
    self->running     = false;
}

static int32_t primary_audio_hw_vad_wait(struct AudioHwVad *base, uint32_t timeout_ms)
{
    struct PrimaryAudioHwVad *self = (struct PrimaryAudioHwVad *)base;
    if (!self || !self->initialised) {
        return HAL_OSAL_ERR_NO_INIT;
    }
    return ameba_audio_vad_wait_wakeup(self->lower, timeout_ms);
}

static int32_t primary_audio_hw_vad_read_lookback(struct AudioHwVad *base,
        void *buffer, size_t buffer_bytes)
{
    struct PrimaryAudioHwVad *self = (struct PrimaryAudioHwVad *)base;
    if (!self || !self->initialised) {
        return HAL_OSAL_ERR_NO_INIT;
    }
    return ameba_audio_vad_read_lookback(self->lower, buffer, buffer_bytes);
}

static int32_t primary_audio_hw_vad_prepare_record(struct AudioHwVad *base)
{
    struct PrimaryAudioHwVad *self = (struct PrimaryAudioHwVad *)base;
    if (!self || !self->initialised) {
        return HAL_OSAL_ERR_NO_INIT;
    }
    return ameba_audio_vad_switch_to_record(self->lower);
}

static int32_t primary_audio_hw_vad_rearm(struct AudioHwVad *base)
{
    struct PrimaryAudioHwVad *self = (struct PrimaryAudioHwVad *)base;
    if (!self || !self->initialised) {
        return HAL_OSAL_ERR_NO_INIT;
    }
    int32_t ret = ameba_audio_vad_rearm_full(self->lower);
    if (ret == HAL_OSAL_OK) {
        self->running = true;
    }
    return ret;
}

static int32_t primary_audio_hw_vad_register_callback(struct AudioHwVad *base,
        AudioHwVadWakeCallback cb, void *user)
{
    struct PrimaryAudioHwVad *self = (struct PrimaryAudioHwVad *)base;
    if (!self) {
        return HAL_OSAL_ERR_NO_INIT;
    }
    if (self->lower) {
        ameba_audio_vad_set_wake_cb(self->lower, (AmebaAudioVadWakeCb)cb, user);
    }
    return HAL_OSAL_OK;
}

static int32_t primary_audio_hw_vad_set_parameters(struct AudioHwVad *base, const char *strs)
{
    struct PrimaryAudioHwVad *self = (struct PrimaryAudioHwVad *)base;
    if (!self || !self->initialised || !strs) {
        return HAL_OSAL_ERR_NO_INIT;
    }

    struct AmebaAudioVadConfig low_cfg;
    memset(&low_cfg, 0, sizeof(low_cfg));
    parse_uint_kv(strs, "det_mv_threshold",      &low_cfg.det_mv_threshold);
    parse_uint_kv(strs, "det_od_threshold",      &low_cfg.det_od_threshold);

    return ameba_audio_vad_set_thresholds(self->lower, &low_cfg);
}

static uint32_t primary_audio_hw_vad_get_hit_addr(struct AudioHwVad *base)
{
    struct PrimaryAudioHwVad *self = (struct PrimaryAudioHwVad *)base;
    if (!self || !self->initialised) {
        return 0;
    }
    return ameba_audio_vad_get_hit_address(self->lower);
}

/*----------------------------------------------------------------------------
 * Singleton accessor — declared in audio_hw_vad.h
 *----------------------------------------------------------------------------*/

struct AudioHwVad *GetAudioHwVad(void)
{
    if (s_primary) {
        return &s_primary->ops;
    }

    struct PrimaryAudioHwVad *self =
        (struct PrimaryAudioHwVad *)calloc(1, sizeof(struct PrimaryAudioHwVad));
    if (!self) {
        return NULL;
    }

    self->ops.Init             = primary_audio_hw_vad_init;
    self->ops.Start            = primary_audio_hw_vad_start;
    self->ops.Stop             = primary_audio_hw_vad_stop;
    self->ops.Deinit           = primary_audio_hw_vad_deinit;
    self->ops.WaitWakeup       = primary_audio_hw_vad_wait;
    self->ops.ReadLookback     = primary_audio_hw_vad_read_lookback;
    self->ops.PrepareForRecord = primary_audio_hw_vad_prepare_record;
    self->ops.Rearm            = primary_audio_hw_vad_rearm;
    self->ops.RegisterCallback = primary_audio_hw_vad_register_callback;
    self->ops.SetParameters    = primary_audio_hw_vad_set_parameters;
    self->ops.GetHitAddress    = primary_audio_hw_vad_get_hit_addr;

    s_primary = self;
    return &self->ops;
}

void DestroyAudioHwVad(struct AudioHwVad *vad)
{
    (void)vad;
    /* Singleton; nothing to refcount yet. */
}
