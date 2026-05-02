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

#ifndef AMEBA_BASE_OSAL_OSAL_C_INCLUDE_OSAL_C_OSAL_CONDITION_H
#define AMEBA_BASE_OSAL_OSAL_C_INCLUDE_OSAL_C_OSAL_CONDITION_H

#include <limits.h>
#include <stdbool.h>

#if defined(__linux__)
#include <pthread.h>
#endif

#include "osal_c/osal_errnos.h"
#include "osal_c/osal_macros.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__linux__)

typedef struct osal_cond_t {
    pthread_cond_t handle;
} osal_cond_t;

OSAL_STATIC_INLINE
int osal_cond_init(osal_cond_t *cond) {
    pthread_condattr_t attr;
    pthread_condattr_init(&attr);
    pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);

    pthread_cond_init(&cond->handle, &attr);
    pthread_condattr_destroy(&attr);
    return OSAL_OK;
}

OSAL_STATIC_INLINE
int osal_cond_destroy(osal_cond_t *cond) {
    return -pthread_cond_destroy(&cond->handle);
}

OSAL_STATIC_INLINE
int osal_cond_wait(osal_cond_t *cond, osal_mutex_t *mutex) {
    return -pthread_cond_wait(&cond->handle, &mutex->handle);
}

OSAL_STATIC_INLINE
int osal_cond_wait_relative(osal_cond_t *cond, osal_mutex_t *mutex, int64_t usec) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    // On 32-bit devices, tv_sec is 32-bit, but `reltime` is 64-bit.
    int64_t reltime_sec = usec / 1000000;

    ts.tv_nsec += static_cast<long>((usec % 1000000) * 1000);
    if (reltime_sec < INT64_MAX && ts.tv_nsec >= 1000000000) {
        ts.tv_nsec -= 1000000000;
        ++reltime_sec;
    }

    int64_t time_sec = ts.tv_sec;
    if (time_sec > INT64_MAX - reltime_sec) {
        time_sec = INT64_MAX;
    } else {
        time_sec += reltime_sec;
    }

    ts.tv_sec = (time_sec > LONG_MAX) ? LONG_MAX : static_cast<long>(time_sec);

    return -pthread_cond_timedwait(&cond->handle, &mutex->handle, &ts);
}

OSAL_STATIC_INLINE
int osal_cond_signal(osal_cond_t *cond) {
    return -pthread_cond_signal(&cond->handle);
}

OSAL_STATIC_INLINE
int osal_cond_broadcast(osal_cond_t *cond) {
    return -pthread_cond_broadcast(&cond->handle);
}

#else // !defined(__linux__)

#define OSAL_COND_MAX_WAITERS 4

typedef struct osal_cond_t {
    void *xTasksWaiting[OSAL_COND_MAX_WAITERS];
} osal_cond_t;

int osal_cond_init(osal_cond_t *cond);
int osal_cond_destroy(osal_cond_t *cond);
int osal_cond_wait(osal_cond_t *cond, osal_mutex_t *mutex);
int osal_cond_wait_relative(osal_cond_t *cond, osal_mutex_t *mutex, int64_t usec);
int osal_cond_signal(osal_cond_t *cond);
int osal_cond_broadcast(osal_cond_t *cond);

#endif

#ifdef __cplusplus
}
#endif

#endif // AMEBA_BASE_OSAL_OSAL_C_INCLUDE_OSAL_C_OSAL_CONDITION_H
