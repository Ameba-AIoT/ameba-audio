/*
 * Copyright (c) 2021 Realtek, LLC.
 * All rights reserved.
 *
 * Licensed under the Realtek License, Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License from PanKore
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @addtogroup Audio
 * @{
 *
 * @brief Declares APIs for audio framework.
 *
 *
 * @since 1.0
 * @version 1.0
 */

/**
 * @file audio_effect.h
 *
 * @brief Provides APIs of the audio effect.
 *
 *
 * @since 1.0
 * @version 1.0
 */

#ifndef AMEBA_AUDIO_INTERFACES_AUDIO_AUDIO_EFFECT_H
#define AMEBA_AUDIO_INTERFACES_AUDIO_AUDIO_EFFECT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct AudioEffect;
struct EffectAttributesBuilder;

int32_t EffectAttributesBuilder_Create(struct EffectAttributesBuilder **builder);
void    EffectAttributesBuilder_Destroy(struct EffectAttributesBuilder *builder);
int32_t EffectAttributesBuilder_SetInRate(struct EffectAttributesBuilder *builder, uint32_t rate);
int32_t EffectAttributesBuilder_SetOutRate(struct EffectAttributesBuilder *builder, uint32_t rate);
int32_t EffectAttributesBuilder_SetInFormat(struct EffectAttributesBuilder *builder, uint32_t format);
int32_t EffectAttributesBuilder_SetOutFormat(struct EffectAttributesBuilder *builder, uint32_t format);
int32_t EffectAttributesBuilder_SetInChannels(struct EffectAttributesBuilder *builder, uint32_t channels);
int32_t EffectAttributesBuilder_SetOutChannels(struct EffectAttributesBuilder *builder, uint32_t channels);
int32_t EffectAttributesBuilder_SetFrameCount(struct EffectAttributesBuilder *builder, uint32_t frame_count);

int32_t AudioEffect_Create(struct EffectAttributesBuilder *builder, struct AudioEffect **stream);
void    AudioEffect_Destroy(struct AudioEffect *stream);
int32_t AudioEffect_SetConfig(struct AudioEffect *stream);
int32_t AudioEffect_Start(struct AudioEffect *stream);
int32_t AudioEffect_Stop(struct AudioEffect *stream);
void    AudioEffect_SetInBuffer(struct AudioEffect *stream, int16_t *buf);
void    AudioEffect_SetOutBuffer(struct AudioEffect *stream, int16_t *buf);
void    AudioEffect_SetFrameCount(struct AudioEffect *module, int32_t frame_count);
void    AudioEffect_Process(struct AudioEffect *stream);
int32_t AudioEffect_Command(struct AudioEffect *stream, uint32_t cmd_code, uint32_t cmd_size, void *cmd_data, uint32_t *reply_size, void *reply_data);

#ifdef __cplusplus
}
#endif

#endif