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

#define LOG_TAG "MyStreamS"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "os_wrapper.h"

#include "log/log.h"
#include "common/audio_errnos.h"

#include "mystream_source.h"

/* prepare delay simulation */
//#define MDS_PREPARE_DELAY_TEST

/* source read unsmooth simulation */
//#define MDS_READ_UNSMOOTH_TEST

/* unknown length source simulation */
#define MDS_UNKNOWN_LENGTH_TEST

#define MDS_SLEEP_TIME_MS 2

#ifdef MDS_PREPARE_DELAY_TEST
int kMDSPrepareDelayTimeMs = 3000;
volatile int g_prepared = 0;
volatile bool g_prepare_thread_alive = 0;
void MyStreamSource_PrepareTestThread(void *Data);
#endif

#ifdef MDS_READ_UNSMOOTH_TEST
#define MDS_READ_RETRY_NUM 200
int kMDSReadCount = 0;
int kMDSReadRetry = 0;
#endif

#ifdef MDS_UNKNOWN_LENGTH_TEST
int kMDSLengthIncreaseTotalTimeMs = 100;
volatile int g_source_total_length = 0;
volatile bool g_length_increase_thread_alive = 0;
void MyStreamSource_UnknownLengthTestThread(void *Data);
#endif

void MyStreamSource_WaitLoopExit(void);


// ----------------------------------------------------------------------
// MyStreamSource
StreamSource *MyStreamSource_Create(char *data, int length)
{
    MEDIA_LOGD("MyStreamSource_Create(%p, %d)", data, length);
    if (!data) {
        MEDIA_LOGE("invalid source or ring_buffer.");
        return NULL;
    }

    MyStreamSource *data_source = calloc(1, sizeof(MyStreamSource));
    if (!data_source) {
        MEDIA_LOGE("fail to alloc MyStreamSource.");
        return NULL;
    }

    data_source->base.CheckPrepared = MyStreamSource_CheckPrepared;
    data_source->base.ReadAt = MyStreamSource_ReadAt;
    data_source->base.GetLength = MyStreamSource_GetLength;

    data_source->data = data;
    data_source->data_length = length;
    data_source->alive = 1;

#ifdef MDS_UNKNOWN_LENGTH_TEST
    data_source->data_length = 0;
    data_source->unknown_data_length = 1;
    g_source_total_length = length;
    g_length_increase_thread_alive = 0;
    MEDIA_LOGD("g_source_total_length: %d, last_data_gained: %d", g_source_total_length, data_source->last_data_gained);
    rtos_task_create(NULL, "UnknownLengthThread", MyStreamSource_UnknownLengthTestThread, (void *)data_source, 2048 * 4, 0);
#endif

    MEDIA_LOGD("length: %d, unknown_data_length: %d", data_source->data_length, data_source->unknown_data_length);

#ifdef MDS_PREPARE_DELAY_TEST
    g_prepared = 0;
    g_prepare_thread_alive = 0;
    rtos_task_create(NULL, "MyStreamSource_PrepareTestThread", MyStreamSource_PrepareTestThread, (void *)data_source, 2048 * 4, 0);
#endif

    return (StreamSource *)data_source;
}

void MyStreamSource_Destroy(MyStreamSource *source)
{
    if (!source) {
        return;
    }

    MEDIA_LOGD("MyStreamSource_Destroy");

    source->alive = 0;

    MyStreamSource_WaitLoopExit();

    free((void *)source);
    source = NULL;
}

int32_t MyStreamSource_CheckPrepared(const StreamSource *source)
{
    if (!source) {
        return AUDIO_ERR_NO_INIT;
    }

#ifdef MDS_PREPARE_DELAY_TEST
    if (!g_prepared) {
        return AUDIO_ERR_NO_INIT;
    }
#endif

    MyStreamSource *data_source = (MyStreamSource *)source;

    return data_source->data ? AUDIO_OK : AUDIO_ERR_NO_INIT;
}

ssize_t MyStreamSource_ReadAt(const StreamSource *source, off_t offset, void *data, size_t size)
{
    if (!source || !data || !size) {
        MEDIA_LOGE("ReadAt invalid param, source: %p, data: %p, size: %d", source, data, size);
        return (ssize_t)AUDIO_ERR_INVALID_OPERATION;
    }

    //MEDIA_LOGD("MyStreamSource_ReadAt offset: %d, size: %d", offset, size);

    MyStreamSource *data_source = (MyStreamSource *)source;

    if (offset >= data_source->data_length) {
        if (data_source->unknown_data_length && !data_source->last_data_gained) {
            //MEDIA_LOGE("ReadAt offset(%d) beyond unknown length data, now data_length(%d)", offset, data_source->data_length);
            return (ssize_t)STREAM_SOURCE_READ_AGAIN;
        }
        MEDIA_LOGD("ReadAt offset(%ld) beyond data length(%d), unknown_length(%d), source(%p), data(%p)",
               offset,
               data_source->data_length, data_source->unknown_data_length,
               data_source, data);
        return (ssize_t)STREAM_SOURCE_EOF;
    }

    if ((data_source->data_length - (int)offset) < (int)size) {
        //MEDIA_LOGD("free size %d is smaller than read size %d, so change read size", data_source->data_length - offset, size);
        size = data_source->data_length - offset;
    }

#ifdef MDS_READ_UNSMOOTH_TEST
    if (kMDSReadCount % 10 == 0) {
        kMDSReadRetry++;
        if (kMDSReadRetry == MDS_READ_RETRY_NUM) {
            kMDSReadRetry = 0;
            kMDSReadCount++;
        }
        MEDIA_LOGD("read again %d-%d", kMDSReadCount, kMDSReadRetry);
        return (ssize_t)STREAM_SOURCE_READ_AGAIN;
    }
#endif

    //MEDIA_LOGD("memcpy %p, %p, %d", data, data_source->data + offset, (size_t)size);
    memcpy(data, data_source->data + offset, (size_t)size);

#ifdef MDS_READ_UNSMOOTH_TEST
    kMDSReadCount++;
#endif

    return size;
}

int32_t MyStreamSource_GetLength(const StreamSource *source, off_t *size)
{
    if (!source) {
        return STREAM_SOURCE_FAIL;
    }

    MyStreamSource *data_source = (MyStreamSource *)source;
    *size = data_source->data_length;

    if (data_source->unknown_data_length && !data_source->last_data_gained) {
        return STREAM_SOURCE_UNKNOWN_LENGTH;
    }

    return 0;
}


// ----------------------------------------------------------------------
// Private Interfaces
void MyStreamSource_WaitLoopExit(void)
{
    int count = 10 * 1000 / MDS_SLEEP_TIME_MS; //wait 3s

#ifdef MDS_UNKNOWN_LENGTH_TEST
#endif
    while (count) {
        int exit = 1;
#ifdef MDS_PREPARE_DELAY_TEST
        if (g_prepare_thread_alive) {
            exit = 0;
        }
#endif
#ifdef MDS_PREPARE_DELAY_TEST
        if (g_length_increase_thread_alive) {
            exit = 0;
        }
#endif
        if (exit) {
            break;
        }

        rtos_time_delay_ms(MDS_SLEEP_TIME_MS);
        //MEDIA_LOGD("wait task exit time=%d.", count);
        count--;
    }
}

#ifdef MDS_PREPARE_DELAY_TEST
void MyStreamSource_PrepareTestThread(void *Data)
{
    MyStreamSource *data_source = (MyStreamSource *)Data;
    MEDIA_LOGD("[MyStreamSource_PrepareTestThread] start");
    g_prepare_thread_alive = 1;

    unsigned int count = kMDSPrepareDelayTimeMs / MDS_SLEEP_TIME_MS;
    do {
        rtos_time_delay_ms(MDS_SLEEP_TIME_MS);
        count--;
    } while (count && data_source->alive);

    g_prepared = 1;
    g_prepare_thread_alive = 0;
    MEDIA_LOGD("[MyStreamSource_PrepareTestThread] exit");
    rtos_task_delete(NULL);
}
#endif

#ifdef MDS_UNKNOWN_LENGTH_TEST
void MyStreamSource_UnknownLengthTestThread(void *Data)
{
    MyStreamSource *data_source = (MyStreamSource *)Data;
    MEDIA_LOGD("[UnknownLengthThread] start, g_source_total_length: %d", g_source_total_length);

    g_length_increase_thread_alive = 1;
    int count = kMDSLengthIncreaseTotalTimeMs / MDS_SLEEP_TIME_MS;
    int block_length = g_source_total_length / count;
    int delta = g_source_total_length % count;

    MEDIA_LOGD("count(%d), block_length(%d), delta(%d), data_source->data_length(%d)", count, block_length, delta, data_source->data_length);
    do {
        data_source->data_length += block_length;
        count--;
        rtos_time_delay_ms(MDS_SLEEP_TIME_MS);
    } while (count > 0 && data_source->alive);

    data_source->data_length += delta;
    data_source->last_data_gained = 1;
    g_length_increase_thread_alive = 0;
    MEDIA_LOGD("[UnknownLengthThread] exit, total data_length: %d", data_source->data_length);
    rtos_task_delete(NULL);
}
#endif
