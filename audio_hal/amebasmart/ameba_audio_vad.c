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
 * @file ameba_audio_vad.c
 * @brief amebasmart low-level VAD platform implementation. Mirrors the style of
 *        ameba_audio_stream_render.c — only fwlib bring-up, IRQ glue, SRAM ring
 *        readback and the rearm sequence live here. No vtable plumbing, no
 *        AudioHwVad knowledge. The vtable lives in primary_audio_hw_vad.c.
 */

#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "basic_types.h"

#include "ameba_soc.h"
#include "ameba_vad.h"
#include "ameba_vad_pc.h"
#include "os_wrapper.h"

#include "audio_hw_debug.h"
#include "audio_hw_osal_errnos.h"

#include "ameba_audio_vad.h"

extern u32  get_hit_addr(void);
extern void get_vad_data(u32 time_period_ms, u8 *buf);
extern void vad_handler_reset(void);

#define VAD_SRAM_LOW             0x20020000U
#define VAD_SRAM_BUF_BYTES       0x20000U     /* 128 KB across all blocks    */
#define VAD_DEFAULT_MIC_INDEX    1U
#define VAD_BYTES_PER_MS         32U          /* 16 kHz × 16-bit × 1ch       */
#define VAD_MAX_CHANNELS         4U           /* codecs 0..3 -> blocks A..D  */

struct AmebaAudioVad {
    struct AmebaAudioVadConfig  cfg;
    rtos_sema_t                 wake_sema;
    AmebaAudioVadWakeCb         wake_cb;
    void                       *wake_cb_user;
    volatile uint32_t           hit_flag;
    bool                        running;
};

static struct AmebaAudioVad *s_singleton = NULL;

static void ameba_audio_vad_dmic_pinmux(uint32_t dmic_idx)
{
    (void)dmic_idx;
    Pinmux_Config(_PB_22, PINMUX_FUNCTION_DMIC);
    Pinmux_Config(_PB_21, PINMUX_FUNCTION_DMIC);
}

static void ameba_audio_vad_amic_power(uint32_t amic_idx)
{
    u32 pin;
    GPIO_InitTypeDef gpio_init;

    switch (amic_idx) {
    case 1:  pin = _PA_30; break;
    case 2:  pin = _PA_31; break;
    case 3:  pin = _PB_0;  break;
    case 4:  pin = _PB_1;  break;
    default: return;
    }

    gpio_init.GPIO_Pin  = pin;
    gpio_init.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_Init(&gpio_init);
    GPIO_WriteBit(pin, 1);
}

static uint32_t vad_amic_for_channel(const struct AmebaAudioVadConfig *cfg, uint32_t ch)
{
    return cfg->mic_index + ch;
}

static uint32_t vad_dmic_for_channel(const struct AmebaAudioVadConfig *cfg, uint32_t ch)
{
    return (cfg->mic_index + ch) & 1U;
}

static void ameba_audio_vad_setup_buffers(uint32_t channel_count)
{
    switch (channel_count) {
    case 2:
        VAD_Buf_move_two(VAD_CODEC_ZERO, VAD_CODEC_ONE,
                         VAD_BLOCK_AB, VAD_BLOCK_MODE);
        break;
    case 3:
        VAD_Buf_move_three(VAD_CODEC_ZERO, VAD_CODEC_ONE, VAD_CODEC_TWO,
                           VAD_BLOCK_ABC, VAD_BLOCK_MODE);
        break;
    case 4:
        VAD_Buf_move_four(VAD_CODEC_ZERO, VAD_CODEC_ONE, VAD_CODEC_TWO, VAD_CODEC_THREE,
                          VAD_BLOCK_ABCD, VAD_BLOCK_MODE);
        break;
    case 1:
    default:
        VAD_Buf_move_one(VAD_CODEC_ZERO, VAD_BLOCK_A, VAD_BLOCK_MODE);
        break;
    }
}

static void ameba_audio_vad_bringup_codecs(struct AmebaAudioVad *self)
{
    uint32_t n = self->cfg.channel_count;

    if (self->cfg.is_dmic) {
        ameba_audio_vad_dmic_pinmux(self->cfg.mic_index);
        for (uint32_t ch = 0; ch < n; ch++) {
            VAD_DMIC_Codec_Init(ch);
            VAD_DMIC_Select(ch, vad_dmic_for_channel(&self->cfg, ch));
            VAD_DMIC_Codec_Enable(ch);
        }
        VAD_Codec_Select(0);
        VAD_Pitch_Set(VAD_HOLD);
        for (uint32_t ch = 0; ch < n; ch++) {
            VAD_HPF_Init(ch, VAD_HPF_SEVEN);
        }
    } else {
        AUDIO_CODEC_SetLDOMode(POWER_ON);
        for (uint32_t ch = 0; ch < n; ch++) {
            ameba_audio_vad_amic_power(vad_amic_for_channel(&self->cfg, ch));
        }
        VAD_ADC_Clock_Enable(CLK_FOR_PC_VAD_CODEC);
        for (uint32_t ch = 0; ch < n; ch++) {
            VAD_ADC_Codec_Init(ch);
            VAD_ADC_Select(ch, ch);
        }
        VAD_Codec_Select(0);
        VAD_Pitch_Set(VAD_INSTANT);
        for (uint32_t ch = 0; ch < n; ch++) {
            VAD_HPF_Init(ch, VAD_HPF_SEVEN);
        }
        for (uint32_t ch = 0; ch < n; ch++) {
            VAD_ADC_Init(ch);
        }
    }
}

static void ameba_audio_vad_irq(void)
{
    struct AmebaAudioVad *self = s_singleton;
    if (!self) {
        return;
    }

    self->hit_flag  = 1;

    InterruptDis(VADBT_OR_VADPC_IRQ);
    pmu_acquire_wakelock(PMU_VAD_DEVICE);

    if (self->wake_sema) {
        rtos_sema_give(self->wake_sema);
    }
}

static void ameba_audio_vad_apply_thresholds(const struct AmebaAudioVadConfig *cfg)
{
    if (cfg->det_mv_threshold) {
        VAD_Det_MV_Thre(cfg->det_mv_threshold);
    }
    if (cfg->det_od_threshold) {
        VAD_Det_OD_Default_Thre(cfg->det_od_threshold);
    }
}

struct AmebaAudioVad *ameba_audio_vad_open(const struct AmebaAudioVadConfig *cfg)
{
    if (s_singleton) {
        HAL_AUDIO_ERROR("AmebaAudioVad: already open (singleton)");
        return NULL;
    }
    if (!cfg) {
        return NULL;
    }

    struct AmebaAudioVad *self =
        (struct AmebaAudioVad *)calloc(1, sizeof(struct AmebaAudioVad));
    if (!self) {
        return NULL;
    }

    self->cfg = *cfg;

    if (!self->cfg.is_dmic && self->cfg.mic_index == 0) {
        self->cfg.mic_index = VAD_DEFAULT_MIC_INDEX;
    }

    if (self->cfg.channel_count == 0) {
        self->cfg.channel_count = 1;
    } else if (self->cfg.channel_count > VAD_MAX_CHANNELS) {
        self->cfg.channel_count = VAD_MAX_CHANNELS;
    }

    rtos_sema_create_binary(&self->wake_sema);
    if (!self->wake_sema) {
        free(self);
        return NULL;
    }

    RCC_PeriphClockCmd(APBPeriph_AC,    APBPeriph_AC_CLOCK,   ENABLE);
    RCC_PeriphClockCmd(APBPeriph_AUDIO, APBPeriph_CLOCK_NULL, ENABLE);
    RCC_PeriphClockSource_AUDIOCODEC(CKSL_AC_XTAL);

    for (u32 i = 0; i < VAD_SRAM_BUF_BYTES; i++) {
        HAL_WRITE8(VAD_SRAM_LOW, i, 0);
    }
    DCache_CleanInvalidate(VAD_SRAM_LOW, VAD_SRAM_BUF_BYTES);

    VAD_Power_Init(VAD_OSC);

    ameba_audio_vad_setup_buffers(self->cfg.channel_count);
    ameba_audio_vad_bringup_codecs(self);

    ameba_audio_vad_apply_thresholds(&self->cfg);

    s_singleton = self;
    InterruptRegister((IRQ_FUN)ameba_audio_vad_irq, VADBT_OR_VADPC_IRQ, NULL, 4);

    vad_handler_reset();

    self->hit_flag = 0;
    return self;
}

void ameba_audio_vad_close(struct AmebaAudioVad *self)
{
    if (!self) {
        return;
    }

    if (self->running) {
        ameba_audio_vad_disarm(self);
    }

    InterruptUnRegister(VADBT_OR_VADPC_IRQ);

    if (self->wake_sema) {
        rtos_sema_delete(self->wake_sema);
        self->wake_sema = NULL;
    }

    if (s_singleton == self) {
        s_singleton = NULL;
    }
    free(self);
}

int32_t ameba_audio_vad_arm(struct AmebaAudioVad *self)
{
    if (!self) {
        return HAL_OSAL_ERR_NO_INIT;
    }
    if (self->running) {
        return HAL_OSAL_OK;
    }

    InterruptEn(VADBT_OR_VADPC_IRQ, 4);
    VAD_Start();
    SOCPS_SetAPWakeEvent(WAKE_SRC_VADBT_OR_VADPC, ENABLE);
    RCC_PeriphClockSource_VADMEM(CKSL_VADM_VAD);

    self->running = true;
    return HAL_OSAL_OK;
}

int32_t ameba_audio_vad_disarm(struct AmebaAudioVad *self)
{
    if (!self) {
        return HAL_OSAL_ERR_NO_INIT;
    }
    if (!self->running) {
        return HAL_OSAL_OK;
    }

    InterruptDis(VADBT_OR_VADPC_IRQ);
    SOCPS_SetAPWakeEvent(WAKE_SRC_VADBT_OR_VADPC, DISABLE);
    self->running = false;
    return HAL_OSAL_OK;
}

int32_t ameba_audio_vad_wait_wakeup(struct AmebaAudioVad *self, uint32_t timeout_ms)
{
    if (!self || !self->wake_sema) {
        return HAL_OSAL_ERR_NO_INIT;
    }

    if (rtos_sema_take(self->wake_sema, timeout_ms) != RTK_SUCCESS) {
        return HAL_OSAL_ERR_TIMED_OUT;
    }

    if (self->wake_cb) {
        self->wake_cb(self->wake_cb_user);
    }
    return HAL_OSAL_OK;
}

void ameba_audio_vad_set_wake_cb(struct AmebaAudioVad *self, AmebaAudioVadWakeCb cb, void *user)
{
    if (!self) {
        return;
    }
    self->wake_cb      = cb;
    self->wake_cb_user = user;
}

int32_t ameba_audio_vad_read_lookback(struct AmebaAudioVad *self, void *buffer, size_t buffer_bytes)
{
    if (!self || !buffer) {
        return HAL_OSAL_ERR_NO_INIT;
    }

    RCC_PeriphClockSource_VADMEM(CKSL_VADM_HS_PLFM);

    uint32_t n = self->cfg.channel_count ? self->cfg.channel_count : 1;
    uint32_t time_period_ms = (uint32_t)(buffer_bytes / (VAD_BYTES_PER_MS * n));
    if (time_period_ms == 0) {
        return 0;
    }

    DCache_Invalidate(VAD_SRAM_LOW, VAD_SRAM_BUF_BYTES);

    get_vad_data(time_period_ms, (u8 *)buffer);
    return (int32_t)(time_period_ms * VAD_BYTES_PER_MS * n);
}

int32_t ameba_audio_vad_switch_to_record(struct AmebaAudioVad *self)
{
    if (!self) {
        return HAL_OSAL_ERR_NO_INIT;
    }
    if (self->cfg.is_dmic) {
        VAD_TypeDef *vad_dev = (TrustZone_IsSecure())
                               ? (VAD_TypeDef *)VAD_REG_BASE_S
                               : (VAD_TypeDef *)VAD_REG_BASE;
        vad_dev->VAD_CLK_CTRL &= ~(VAD_BIT_PC_TCON_VAD_EN     |
                                   VAD_BIT_PC_TCON_DMIC_EN     |
                                   VAD_BIT_PC_TCON_DMIC_SRC_0_EN |
                                   VAD_BIT_PC_TCON_ADC_0_EN    |
                                   VAD_BIT_PC_TCON_DMIC_SRC_1_EN |
                                   VAD_BIT_PC_TCON_ADC_1_EN);
    } else {
        VAD_ADC_Clock_Enable(CLK_FOR_AUDIO_CODEC);
    }
    return HAL_OSAL_OK;
}

int32_t ameba_audio_vad_rearm_full(struct AmebaAudioVad *self)
{
    if (!self) {
        return HAL_OSAL_ERR_NO_INIT;
    }

    RCC_PeriphClockCmd(APBPeriph_AC,    APBPeriph_AC_CLOCK,   ENABLE);
    RCC_PeriphClockCmd(APBPeriph_AUDIO, APBPeriph_CLOCK_NULL, ENABLE);
    RCC_PeriphClockSource_AUDIOCODEC(CKSL_AC_XTAL);

    ameba_audio_vad_bringup_codecs(self);
    ameba_audio_vad_apply_thresholds(&self->cfg);

    {
        VAD_TypeDef *vad_dev = (TrustZone_IsSecure())
                               ? (VAD_TypeDef *)VAD_REG_BASE_S
                               : (VAD_TypeDef *)VAD_REG_BASE;
        vad_dev->VAD_PITCH_DET_CTRL9 |=  VAD_BIT_PITCH_DET_CLR;
        vad_dev->VAD_PITCH_DET_CTRL9 &= ~VAD_BIT_PITCH_DET_CLR;
    }
    self->hit_flag  = 0;
    vad_handler_reset();

    VAD_Start();
    InterruptEn(VADBT_OR_VADPC_IRQ, 4);

    RCC_PeriphClockSource_VADMEM(CKSL_VADM_VAD);

    pmu_release_wakelock(PMU_VAD_DEVICE);

    self->running = true;
    return HAL_OSAL_OK;
}

int32_t ameba_audio_vad_set_thresholds(struct AmebaAudioVad *self, const struct AmebaAudioVadConfig *cfg)
{
    if (!self || !cfg) {
        return HAL_OSAL_ERR_NO_INIT;
    }
    if (cfg->det_mv_threshold)      self->cfg.det_mv_threshold      = cfg->det_mv_threshold;
    if (cfg->det_od_threshold)      self->cfg.det_od_threshold      = cfg->det_od_threshold;

    ameba_audio_vad_apply_thresholds(&self->cfg);
    return HAL_OSAL_OK;
}

uint32_t ameba_audio_vad_get_hit_address(struct AmebaAudioVad *self)
{
    (void)self;
    return get_hit_addr();
}
