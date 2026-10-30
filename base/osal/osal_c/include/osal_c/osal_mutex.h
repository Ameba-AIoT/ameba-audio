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

#ifndef AMEBA_BASE_OSAL_OSAL_C_INCLUDE_OSAL_C_OSAL_MUTEX_H
#define AMEBA_BASE_OSAL_OSAL_C_INCLUDE_OSAL_C_OSAL_MUTEX_H

#if defined(__linux__)
#include <pthread.h>
#else
#include "FreeRTOS.h"
#include "semphr.h"
#endif

#include "osal_c/osal_errnos.h"
#include "osal_c/osal_macros.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct osal_mutex_t {
#if defined(__linux__)
    pthread_mutex_t handle;
#else
    SemaphoreHandle_t handle;
#endif
} osal_mutex_t;

#define OSAL_DECLARE_MUTEX(mutex) osal_mutex_t mutex

#if defined(__linux__)

OSAL_STATIC_INLINE
int osal_mutex_init(osal_mutex_t *mutex) {
    return -pthread_mutex_init(&mutex->handle, NULL);
}

OSAL_STATIC_INLINE
int osal_mutex_destroy(osal_mutex_t *mutex) {
    return -pthread_mutex_destroy(&mutex->handle);
}

OSAL_STATIC_INLINE
int osal_mutex_lock(osal_mutex_t *mutex) {
    return -pthread_mutex_lock(&mutex->handle);
}

OSAL_STATIC_INLINE
int osal_mutex_try_lock(osal_mutex_t *mutex) {
    return -pthread_mutex_trylock(&mutex->handle);
}

OSAL_STATIC_INLINE
int osal_mutex_unlock(osal_mutex_t *mutex) {
    return -pthread_mutex_unlock(&mutex->handle);
}

#else // !defined(__linux__)

OSAL_STATIC_INLINE
int osal_mutex_init(osal_mutex_t *mutex) {
    if (!mutex) return OSAL_ERR_INVALID_PARAM;
    mutex->handle = xSemaphoreCreateMutex();
    return (mutex->handle) ? OSAL_OK : OSAL_ERR_OPERATION_FAIL;
}

OSAL_STATIC_INLINE
int osal_mutex_destroy(osal_mutex_t *mutex)
{
    if (!mutex || !mutex->handle) return OSAL_ERR_OPERATION_FAIL;
    vSemaphoreDelete(mutex->handle);
    return OSAL_OK;
}

OSAL_STATIC_INLINE
int osal_mutex_lock(osal_mutex_t *mutex)
{
    if (!mutex || !mutex->handle) return OSAL_ERR_OPERATION_FAIL;
    xSemaphoreTake(mutex->handle, portMAX_DELAY);
    return OSAL_OK;
}

OSAL_STATIC_INLINE
int osal_mutex_try_lock(osal_mutex_t *mutex)
{
    if (!mutex || !mutex->handle) return OSAL_ERR_OPERATION_FAIL;
    BaseType_t ret = xSemaphoreTake(mutex->handle, 0);
    return (ret == pdTRUE) ? OSAL_OK : OSAL_ERR_TIMED_OUT;
}

OSAL_STATIC_INLINE
int osal_mutex_unlock(osal_mutex_t *mutex)
{
    if (!mutex || !mutex->handle) return OSAL_ERR_OPERATION_FAIL;
    xSemaphoreGive(mutex->handle);
    return OSAL_OK;
}

#endif

#ifdef __cplusplus
}
#endif

#endif // AMEBA_BASE_OSAL_OSAL_C_INCLUDE_OSAL_C_OSAL_MUTEX_H
