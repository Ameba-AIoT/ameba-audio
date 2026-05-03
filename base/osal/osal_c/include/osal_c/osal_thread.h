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

#ifndef AMEBA_BASE_OSAL_OSAL_C_INCLUDE_OSAL_C_OSAL_THREAD_H
#define AMEBA_BASE_OSAL_OSAL_C_INCLUDE_OSAL_C_OSAL_THREAD_H

#include <stddef.h>
#include <stdbool.h>

#include "osal_c/osal_errnos.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    OSAL_THREAD_PRI_LOWEST         =  19,
    OSAL_THREAD_PRI_BACKGROUND     =  10,
    OSAL_THREAD_PRI_NORMAL         =   0,
    OSAL_THREAD_PRI_FOREGROUND     =  -2,
    OSAL_THREAD_PRI_DISPLAY        =  -4,
    OSAL_THREAD_PRI_URGENT_DISPLAY =  -8,
    OSAL_THREAD_PRI_VIDEO          = -10,
    OSAL_THREAD_PRI_AUDIO          = -16,
    OSAL_THREAD_PRI_URGENT_AUDIO   = -19,
    OSAL_THREAD_PRI_HIGHEST        = -20,

    OSAL_THREAD_PRI_DEFAULT        = OSAL_THREAD_PRI_NORMAL,
    OSAL_THREAD_PRI_MORE_FAVORABLE = -1,
    OSAL_THREAD_PRI_LESS_FAVORABLE = +1,
} osal_thread_priority;

typedef struct osal_thread_param {
    const char *name;
    osal_thread_priority priority;
    size_t stack_size;
    bool joinable;
} osal_thread_param;

typedef void *osal_thread_t;
typedef void *(*osal_thread_entry_t)(void *);

int osal_thread_create(osal_thread_t *thread,
                       osal_thread_entry_t start_routine, void *arg,
                       osal_thread_param *param);

int osal_thread_join(osal_thread_t thread, void **retval);

osal_thread_t osal_thread_self(void);

int osal_thread_equal(osal_thread_t t1, osal_thread_t t2);

#ifdef __cplusplus
}
#endif

#endif // AMEBA_BASE_OSAL_OSAL_C_INCLUDE_OSAL_C_OSAL_THREAD_H
