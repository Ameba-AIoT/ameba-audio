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

/**
 * @addtogroup Media
 * @{
 *
 * @brief Declares APIs for StreamSource.
 *
 *
 * @since 1.0
 * @version 1.0
 */

/**
 * @file stream_source.h
 *
 *
 * @since 1.0
 * @version 1.0
 */

#ifndef AMEBA_FWK_MEDIA_PLAYBACK_MEDIA_STREAM_SOURCE_H
#define AMEBA_FWK_MEDIA_PLAYBACK_MEDIA_STREAM_SOURCE_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct StreamSource StreamSource;
struct StreamSource {
    /**
     * @brief Check whether the data source init success.
     *
     * @param source The StreamSource object pointer.
     * @return Returns a value listed below: \n
     * int32_t | Description
     * ----------------------| -----------------------
     * AUDIO_OK | prepare success.
     * AUDIO_ERR_NO_INIT | source init fail.
     * @since 1.0
     * @version 1.0
     */
    int32_t (*CheckPrepared)(const StreamSource *source);

    /**
     * @brief Returns the number of bytes read, or error value on failure. It's not an error if
     * this returns zero; it just means the given offset is equal to, or
     * beyond, the end of the source.
     *
     * @param source The StreamSource object pointer.
     * @param offset The offset from the beginning of the source.
     * @param data The pointer that stores the data.
     * @param size The size need to read.
     * @return Returns the number of bytes read, or error value on failure: \n
     * AUDIO_ERR_INVALID_OPERATION | the operation is invalid.
     * STREAM_SOURCE_EOF | read meet the end of stream.
     * STREAM_SOURCE_READ_AGAIN | need to read again.
     * @since 1.0
     * @version 1.0
     */
    ssize_t (*ReadAt)(const StreamSource *source, off_t offset, void *data, size_t size);

    /**
     * @brief Get the whole length of the source.
     *
     * @param source The StreamSource object pointer.
     * @param size The size of the source.
     * @return Returns a value listed below: \n
     * int32_t | Description
     * ----------------------| -----------------------
     * AUDIO_OK | the operation is successful.
     * STREAM_SOURCE_FAIL | operation is failed.
     * STREAM_SOURCE_UNKNOWN_LENGTH | this is an unknown length source.
     * @since 1.0
     * @version 1.0
     */
    int32_t (*GetLength)(const StreamSource *source, off_t *size);
};

enum {
    STREAM_SOURCE_OK = 0,

    STREAM_SOURCE_ERROR_BASE = -1000,
    STREAM_SOURCE_READ_AGAIN = STREAM_SOURCE_ERROR_BASE - 1,
    STREAM_SOURCE_FAIL = STREAM_SOURCE_ERROR_BASE - 2,
    STREAM_SOURCE_EOF = STREAM_SOURCE_ERROR_BASE - 3,
    STREAM_SOURCE_UNKNOWN_LENGTH = STREAM_SOURCE_ERROR_BASE - 4,
};

#ifdef __cplusplus
}
#endif

#endif // AMEBA_FWK_MEDIA_PLAYBACK_MEDIA_STREAM_SOURCE_H
/** @} */
