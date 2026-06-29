/*
 * Copyright (c) 2026 Realtek Corp.
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

#ifndef AMEBA_BASE_LOG_INCLUDE_LOG_LOG_H
#define AMEBA_BASE_LOG_INCLUDE_LOG_LOG_H

#include <stdint.h>

#ifdef __linux__
#include "log_posix.h"
#else
#include "log.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define MEDIA_LOG_VERBOSE   ((rtk_log_level_t)10)
#define MEDIA_LOG_DEBUG     RTK_LOG_DEBUG
#define MEDIA_LOG_INFO      RTK_LOG_INFO
#define MEDIA_LOG_WARN      RTK_LOG_WARN
#define MEDIA_LOG_ERROR     RTK_LOG_ERROR
#define MEDIA_LOG_FATAL     RTK_LOG_ALWAYS

#ifndef LOG_TAG
#define LOG_TAG NOTAG
#endif

#ifndef __predict_false
#define __predict_false(exp) __builtin_expect((exp) != 0, 0)
#endif

// ---------------------------------------------------------------------

/*
 * Log a fatal error. If given condition fails, this stops program
 * execution like a normal assertion, but also generating the given message.
 */

#define MEDIA_LOG_ALWAYS_FATAL_IF(cond, fmt, ...) do {                      \
        if (__predict_false(cond)) { \
            rtk_log_write(MEDIA_LOG_FATAL, LOG_TAG, 'A',                    \
                "Assertion failed: " #cond " - " fmt "\n", ##__VA_ARGS__);  \
            while(1);                                                       \
        }                                                                   \
    } while(0)

#define MEDIA_LOG_ALWAYS_FATAL(fmt, ...) do {                                   \
        rtk_log_write(MEDIA_LOG_FATAL, LOG_TAG, 'A', fmt "\n", ##__VA_ARGS__);  \
        while(1);                                                               \
    } while(0)

#define MEDIA_LOG_ASSERT(cond, ...)  \
    MEDIA_LOG_ALWAYS_FATAL_IF(!(cond), "Assert failed", ##__VA_ARGS__)

// ---------------------------------------------------------------------

/*
 * C/C++ logging functions.
 */

#define MEDIA_LOG_ITEM(level, letter, format, ...) do {                         \
        if (COMPIL_LOG_LEVEL >= level) \
            rtk_log_write(level, LOG_TAG, letter, format "\n", ##__VA_ARGS__);  \
    } while(0)

#define MEDIA_LOG_ITEM_IF(cond, level, letter, format, ...) do {                \
        if (__predict_false(cond) && COMPIL_LOG_LEVEL >= level)                 \
            rtk_log_write(level, LOG_TAG, letter, format "\n", ##__VA_ARGS__);  \
    } while(0)

#ifndef MEDIA_LOGV
#define MEDIA_LOGV(...) MEDIA_LOG_ITEM(MEDIA_LOG_VERBOSE, 'V', __VA_ARGS__)
#endif

#ifndef MEDIA_LOGV_IF
#define MEDIA_LOGV_IF(cond, ...) MEDIA_LOG_ITEM_IF(cond, MEDIA_LOG_VERBOSE, 'V', __VA_ARGS__)
#endif

#ifndef MEDIA_LOGD
#define MEDIA_LOGD(...) MEDIA_LOG_ITEM(RTK_LOG_DEBUG, 'D', __VA_ARGS__)
#endif

#ifndef MEDIA_LOGD_IF
#define MEDIA_LOGD_IF(cond, ...) MEDIA_LOG_ITEM_IF(cond, RTK_LOG_DEBUG, 'D', __VA_ARGS__)
#endif

#ifndef MEDIA_LOGI
#define MEDIA_LOGI(...) MEDIA_LOG_ITEM(RTK_LOG_INFO, 'I', __VA_ARGS__)
#endif

#ifndef MEDIA_LOGI_IF
#define MEDIA_LOGI_IF(cond, ...) MEDIA_LOG_ITEM_IF(cond, RTK_LOG_INFO, 'I', __VA_ARGS__)
#endif

#ifndef MEDIA_LOGW
#define MEDIA_LOGW(...) MEDIA_LOG_ITEM(RTK_LOG_WARN, 'W', __VA_ARGS__)
#endif

#ifndef MEDIA_LOGW_IF
#define MEDIA_LOGW_IF(cond, ...) MEDIA_LOG_ITEM_IF(cond, RTK_LOG_WARN, 'W', __VA_ARGS__)
#endif

#ifndef MEDIA_LOGE
#define MEDIA_LOGE(...) MEDIA_LOG_ITEM(RTK_LOG_ERROR, 'E', __VA_ARGS__)
#endif

#ifndef MEDIA_LOGE_IF
#define MEDIA_LOGE_IF(cond, ...) MEDIA_LOG_ITEM_IF(cond, RTK_LOG_ERROR, 'E', __VA_ARGS__)
#endif

#ifdef __cplusplus
}
#endif

#endif // AMEBA_BASE_LOG_INCLUDE_LOG_LOG_H
