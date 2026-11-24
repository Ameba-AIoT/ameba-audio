/*
 * Copyright (c) 2022 Realtek, LLC.
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

#include <stdlib.h>

#include "ameba_audio_stream_utils.h"
#include "ameba_audio_types.h"

#include "ameba_audio_stream_buffer.h"

/*
 * read_ptr/write_ptr are mirror indexes in [0, 2 * capacity), see
 * ameba_audio_stream_buffer.h. Each one is stored only by its own side, after
 * the data it covers has been copied, so the other side never sees an index
 * ahead of the data.
 */
static inline size_t rb_offset(const AudioBuffer *buffer, size_t idx)
{
	return (idx < buffer->capacity) ? idx : (idx - buffer->capacity);
}

static inline size_t rb_advance(const AudioBuffer *buffer, size_t idx, size_t bytes)
{
	idx += bytes;
	if (idx >= 2 * buffer->capacity) {
		idx -= 2 * buffer->capacity;
	}
	return idx;
}

/* bytes written and not yet read: 0 is empty, capacity is full */
static inline size_t rb_used(const AudioBuffer *buffer)
{
	size_t w = buffer->write_ptr;
	size_t r = buffer->read_ptr;

	return (w >= r) ? (w - r) : (w + 2 * buffer->capacity - r);
}

AudioBuffer *ameba_audio_stream_buffer_create(void)
{
	AudioBuffer *buffer;
	buffer = (AudioBuffer *)rtos_mem_calloc(1, sizeof(AudioBuffer));
	if (!buffer) {
		HAL_AUDIO_ERROR("[AmebaAudioRbuf] calloc AudioRBuffer fail");
		return NULL;
	}
	buffer->read_ptr = 0;
	buffer->write_ptr = 0;
	buffer->capacity = 0;
	buffer->raw_data = NULL;
	return buffer;
}

void ameba_audio_stream_buffer_release(AudioBuffer *buffer)
{
	if (buffer->raw_data != NULL) {
		rtos_mem_free(buffer->raw_data);
		buffer->raw_data = NULL;
	}

	if (buffer) {
		rtos_mem_free(buffer);
	}
}


void ameba_audio_stream_buffer_alloc(AudioBuffer *buffer, size_t capacity)
{
	buffer->raw_data  = (char *)rtos_mem_calloc(capacity, sizeof(char));
	if (!(buffer->raw_data)) {
		HAL_AUDIO_ERROR("[AmebaAudioRbuf] calloc raw_data fail");
		return;
	}

	buffer->capacity = capacity;
}

size_t ameba_audio_stream_buffer_get_buffer_capacity(AudioBuffer *buffer)
{
	return buffer->capacity;
}

/*buffer bytes remain in buffer to be consumed*/
size_t ameba_audio_stream_buffer_get_remain_size(AudioBuffer *buffer)
{
	if (buffer->capacity == 0) {
		return 0;
	}

	return rb_used(buffer);
}

/*buffer bytes available in buffer to be written*/
size_t ameba_audio_stream_buffer_get_available_size(AudioBuffer *buffer)
{
	if (buffer->capacity == 0) {
		return 0;
	}

	/* in noirq mode GDMA consumes/produces without moving the index, so the
	 * index distance can exceed capacity there */
	size_t used = rb_used(buffer);
	return (used >= buffer->capacity) ? 0 : (buffer->capacity - used);
}

void ameba_audio_stream_buffer_flush(AudioBuffer *buffer)
{
	buffer->read_ptr = 0;
	buffer->write_ptr = 0;
}

/*write to buffer by hal*/
size_t ameba_audio_stream_buffer_write_in_noirq_mode(AudioBuffer *buffer, const void *data, size_t bytes, uint32_t period_bytes)
{
	if (bytes == 0 || buffer->capacity == 0) {
		return 0;
	}

	size_t capacity = buffer->capacity;
	size_t bytes_to_write = bytes;
	size_t wp = rb_offset(buffer, buffer->write_ptr);
	size_t tx_addr;

	if (bytes_to_write <= capacity - wp) {
		tx_addr = (size_t)buffer->raw_data + wp;
		memcpy(buffer->raw_data + wp, (u8 *)data, bytes_to_write);
		DCache_CleanInvalidate((uint32_t)tx_addr, (period_bytes + CACHE_LINE_SIZE));
	} else {
		size_t size_1 = capacity - wp;
		memcpy(buffer->raw_data + wp, (u8 *)data, size_1);
		size_t size_2 = bytes_to_write - size_1;
		memcpy(buffer->raw_data, (u8 *)data + size_1, size_2);
		DCache_CleanInvalidate((uint32_t)buffer->raw_data, ((uint32_t)capacity + CACHE_LINE_SIZE));
	}

	__DMB();
	buffer->write_ptr = rb_advance(buffer, buffer->write_ptr, bytes_to_write);

	return bytes_to_write;
}

/*write to buffer by hal*/
size_t ameba_audio_stream_buffer_write(AudioBuffer *buffer, const void *data, size_t bytes)
{
	if (bytes == 0 || buffer->capacity == 0) {
		return 0;
	}

	size_t capacity = buffer->capacity;
	size_t avail = capacity - rb_used(buffer);
	if (avail == 0) {
		return 0;
	}

	size_t bytes_to_write = (bytes <= avail) ? bytes : avail;
	size_t wp = rb_offset(buffer, buffer->write_ptr);

	if (bytes_to_write <= capacity - wp) {
		memcpy(buffer->raw_data + wp, (u8 *)data, bytes_to_write);
	} else {
		size_t size_1 = capacity - wp;
		memcpy(buffer->raw_data + wp, (u8 *)data, size_1);
		size_t size_2 = bytes_to_write - size_1;
		memcpy(buffer->raw_data, (u8 *)data + size_1, size_2);
	}

	__DMB();
	buffer->write_ptr = rb_advance(buffer, buffer->write_ptr, bytes_to_write);

	HAL_AUDIO_PVERBOSE("after write,rp:%d,wp:%d,bytes_to_write:%d", buffer->read_ptr, buffer->write_ptr, bytes_to_write);

	return bytes_to_write;
}

/*read buffer by gdma, update readptr here*/
void ameba_audio_stream_buffer_update_tx_readptr(AudioBuffer *buffer, size_t bytes)
{
	if (bytes == 0 || buffer->capacity == 0) {
		return;
	}

	size_t used = rb_used(buffer);
	size_t bytes_to_read = (bytes <= used) ? bytes : used;

	__DMB();
	buffer->read_ptr = rb_advance(buffer, buffer->read_ptr, bytes_to_read);

	HAL_AUDIO_PVERBOSE("gdma up rd done,rp:%d,wp:%d", buffer->read_ptr, buffer->write_ptr);
}

/*byte offset in raw_data where the next read starts*/
size_t ameba_audio_stream_buffer_get_tx_readptr(AudioBuffer *buffer)
{
	return rb_offset(buffer, buffer->read_ptr);
}

/*write to buffer by gdma, update writeptr here*/
void ameba_audio_stream_buffer_update_rx_writeptr(AudioBuffer *buffer, size_t bytes)
{
	if (bytes == 0 || buffer->capacity == 0) {
		return;
	}

	if (bytes > buffer->capacity - rb_used(buffer)) {
		return;
	}

	if (rb_offset(buffer, buffer->write_ptr) + bytes > buffer->capacity) {
		HAL_AUDIO_ERROR("write ptr out of range!");
	}

	__DMB();
	buffer->write_ptr = rb_advance(buffer, buffer->write_ptr, bytes);
	HAL_AUDIO_CVERBOSE("gdma up wr done,rp:%d,wp:%d", buffer->read_ptr, buffer->write_ptr);
}

/*read buffer by hal*/
size_t ameba_audio_stream_buffer_read(AudioBuffer *buffer, void *data, size_t bytes, uint32_t mode)
{
	if (bytes == 0 || buffer->capacity == 0) {
		return 0;
	}

	size_t capacity = buffer->capacity;
	size_t bytes_to_read;
	if (mode == AMEBA_AUDIO_DMA_NOIRQ_MODE) {
		bytes_to_read = bytes;
	} else {
		size_t used = rb_used(buffer);
		bytes_to_read = (bytes <= used) ? bytes : used;
	}
	/* data behind the write index snapshot must not be read before it */
	__DMB();

	size_t rp = rb_offset(buffer, buffer->read_ptr);

	if (bytes_to_read <= capacity - rp) {
		/*must add cache clean invalidate here. otherwize buf read may remain dirty.*/
		DCache_CleanInvalidate((uint32_t)buffer->raw_data + (uint32_t)rp, bytes_to_read);
		memcpy(data, buffer->raw_data + rp, bytes_to_read);
	} else {
		size_t size_1 = capacity - rp;
		/*must add cache clean invalidate here. otherwize buf read may remain dirty.*/
		DCache_CleanInvalidate((uint32_t)buffer->raw_data + (uint32_t)rp, size_1);
		memcpy((u8 *)data, buffer->raw_data + rp, size_1);
		size_t size_2 = bytes_to_read - size_1;
		DCache_CleanInvalidate((uint32_t)buffer->raw_data, size_2);
		memcpy((u8 *)data + size_1, buffer->raw_data, size_2);
	}

	__DMB();
	buffer->read_ptr = rb_advance(buffer, buffer->read_ptr, bytes_to_read);
	HAL_AUDIO_CVERBOSE("after read,rp:%u,wp:%u", buffer->read_ptr, buffer->write_ptr);
	return bytes_to_read;
}

/*byte offset in raw_data where the next write starts*/
size_t ameba_audio_stream_buffer_get_rx_writeptr(AudioBuffer *buffer)
{
	return rb_offset(buffer, buffer->write_ptr);
}
