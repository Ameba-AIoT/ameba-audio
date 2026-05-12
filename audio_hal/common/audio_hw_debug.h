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

#ifndef AMEBA_AUDIO_AUDIO_HAL_COMMON_AUDIO_HW_DEBUG_H
#define AMEBA_AUDIO_AUDIO_HAL_COMMON_AUDIO_HW_DEBUG_H

#include "ameba.h"
#include "basic_types.h"
#include "os_wrapper.h"
#include "xlib/string_ext.h"

#define AUDIO_HAL_TAG "AudioHal"

/* Debug options */
#define HAL_AUDIO_ENABLE_LOG                  1
#define HAL_AUDIO_VERBOSE_DEBUG               0
#define HAL_AUDIO_PLAYBACK_VERY_VERBOSE_DEBUG 0
#define HAL_AUDIO_CAPTURE_VERY_VERBOSE_DEBUG  0
#define HAL_AUDIO_PLAYBACK_DUMP_DEBUG         0
#define HAL_AUDIO_CAPTURE_DUMP_DEBUG          0

#define HAL_AUDIO_ERROR(fmt, args...)         RTK_LOGE(AUDIO_HAL_TAG, "[%s]: " fmt "\n", __func__, ## args)

#if HAL_AUDIO_ENABLE_LOG
#define HAL_AUDIO_DEBUG(fmt, args...)         RTK_LOGD(AUDIO_HAL_TAG, "[%s]: " fmt "\n", __func__, ## args)
#define HAL_AUDIO_INFO(fmt, args...)          RTK_LOGI(AUDIO_HAL_TAG, "[%s]: " fmt "\n", __func__, ## args)
#define HAL_AUDIO_WARN(fmt, args...)          RTK_LOGW(AUDIO_HAL_TAG, "[%s]: " fmt "\n", __func__, ## args)
#define HAL_AUDIO_IRQ_INFO(fmt, args...)      RTK_LOGS(NOTAG, RTK_LOG_ALWAYS, "AudioHal [%s]: " fmt "\n", __func__, ## args)
#else
#define HAL_AUDIO_DEBUG(fmt, args...)         do { } while(0)
#define HAL_AUDIO_INFO(fmt, args...)          do { } while(0)
#define HAL_AUDIO_WARN(fmt, args...)          do { } while(0)
#define HAL_AUDIO_IRQ_INFO(fmt, args...)      do { } while(0)
#endif

#if HAL_AUDIO_ENABLE_LOG && HAL_AUDIO_VERBOSE_DEBUG
#define HAL_AUDIO_VERBOSE(fmt, args...)       RTK_LOGI(AUDIO_HAL_TAG, "[%s]: " fmt "\n", __func__, ## args)
#else
#define HAL_AUDIO_VERBOSE(fmt, args...)       do { } while(0)
#endif

#if HAL_AUDIO_ENABLE_LOG && HAL_AUDIO_VERBOSE_DEBUG && HAL_AUDIO_PLAYBACK_VERY_VERBOSE_DEBUG
#define HAL_AUDIO_PVERBOSE(fmt, args...)      RTK_LOGI(AUDIO_HAL_TAG, "[%s]: " fmt "\n", __func__, ## args)
#else
#define HAL_AUDIO_PVERBOSE(fmt, args...)      do { } while(0)
#endif

#if HAL_AUDIO_ENABLE_LOG && HAL_AUDIO_VERBOSE_DEBUG && HAL_AUDIO_CAPTURE_VERY_VERBOSE_DEBUG
#define HAL_AUDIO_CVERBOSE(fmt, args...)      RTK_LOGI(AUDIO_HAL_TAG, "[%s]: " fmt "\n", __func__, ## args)
#else
#define HAL_AUDIO_CVERBOSE(fmt, args...)      do { } while(0)
#endif

#endif
