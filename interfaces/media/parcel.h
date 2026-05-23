/*
 * Copyright (c) 2025 Realtek, LLC.
 * All rights reserved.
 *
 * Licensed under the Realtek License, Version 1.0 (the License);
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
 * @brief Declares Parcel APIs for data serialization/deserialization.
 *
 * @since 1.0
 * @version 1.0
 */

/**
 * @file parcel.h
 *
 * @brief Provides Parcel APIs for inter-process communication (IPC) data serialization.
 * Parcel is a data container used for marshalling/unmarshalling primitive data types
 * and buffers. It is typically used in IPC scenarios where data needs to be passed
 * between different processes or threads.
 *
 * @since 1.0
 * @version 1.0
 */

#ifndef AMEBA_AUDIO_BASE_CUTILS_INCLUDE_CUTILS_PARCEL_H
#define AMEBA_AUDIO_BASE_CUTILS_INCLUDE_CUTILS_PARCEL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Opaque Parcel structure handle */
typedef struct Parcel Parcel;

/**
 * @brief Parcel data release callback function type.
 *
 * @param parcel The Parcel object pointer.
 * @param data The data buffer to be released.
 * @param dataSize The size of the data buffer.
 * @since 1.0
 * @version 1.0
 */
typedef void (*release_func)(Parcel* parcel, const uint8_t* data, size_t dataSize);

/**
 * @brief Creates a new Parcel object.
 *
 * Allocates and initializes a new Parcel instance for data serialization.
 * The returned Parcel must be freed by calling Parcel_Destroy() when no longer needed.
 *
 * @return Pointer to the newly created Parcel object, or NULL if allocation fails.
 * @note After creation, the Parcel is empty and ready for writing or reading.
 * @since 1.0
 * @version 1.0
 */
Parcel *Parcel_Create(void);

/**
 * @brief Sets IPC data to Parcel.
 *
 * Assigns externally allocated data to the Parcel for IPC scenarios.
 * The Parcel takes ownership of the data buffer. When the Parcel is destroyed,
 * the provided release function will be called to free the data if provided.
 *
 * @param parcel The Parcel object pointer.
 * @param data The data buffer to set.
 * @param data_size The size of the data buffer in bytes.
 * @param rel_func The release callback function to free the data, can be NULL.
 * @note This function is typically used for receiving IPC data.
 * @since 1.0
 * @version 1.0
 */
void Parcel_IpcSetData(Parcel *parcel, uint8_t* data, size_t data_size, release_func rel_func);

/**
 * @brief Destroys a Parcel object.
 *
 * Releases all resources associated with the Parcel, including allocated data buffers.
 * After calling this function, the Parcel pointer becomes invalid.
 *
 * @param parcel The Parcel object pointer.
 * @note If a release function was provided via Parcel_IpcSetData(), it will be called.
 * @since 1.0
 * @version 1.0
 */
void Parcel_Destroy(Parcel *parcel);

/**
 * @brief Gets the IPC data pointer from Parcel.
 *
 * @param parcel The Parcel object pointer.
 * @return Pointer to the internal data buffer, or NULL if no data is set.
 * @since 1.0
 * @version 1.0
 */
uint8_t* Parcel_IpcData(Parcel *parcel);

/**
 * @brief Gets the IPC data size from Parcel.
 *
 * @param parcel The Parcel object pointer.
 * @return The size of the data buffer in bytes.
 * @since 1.0
 * @version 1.0
 */
size_t Parcel_IpcDataSize(Parcel *parcel);

/**
 * @brief Writes a boolean value to Parcel.
 *
 * @param parcel The Parcel object pointer.
 * @param value The boolean value to write.
 * @return true on success, false on failure.
 * @since 1.0
 * @version 1.0
 */
bool Parcel_WriteBool(Parcel *parcel, bool value);

/**
 * @brief Reads a boolean value from Parcel.
 *
 * @param parcel The Parcel object pointer.
 * @return The boolean value read from Parcel.
 * @since 1.0
 * @version 1.0
 */
bool Parcel_ReadBool(Parcel *parcel);

/**
 * @brief Writes an int8 value to Parcel.
 *
 * @param parcel The Parcel object pointer.
 * @param value The int8 value to write.
 * @return true on success, false on failure.
 * @since 1.0
 * @version 1.0
 */
bool Parcel_WriteInt8(Parcel *parcel, int8_t value);

/**
 * @brief Reads an int8 value from Parcel.
 *
 * @param parcel The Parcel object pointer.
 * @return The int8 value read from Parcel.
 * @since 1.0
 * @version 1.0
 */
int8_t Parcel_ReadInt8(Parcel *parcel);

/**
 * @brief Writes an int16 value to Parcel.
 *
 * @param parcel The Parcel object pointer.
 * @param value The int16 value to write.
 * @return true on success, false on failure.
 * @since 1.0
 * @version 1.0
 */
bool Parcel_WriteInt16(Parcel *parcel, int16_t value);

/**
 * @brief Reads an int16 value from Parcel.
 *
 * @param parcel The Parcel object pointer.
 * @return The int16 value read from Parcel.
 * @since 1.0
 * @version 1.0
 */
int16_t Parcel_ReadInt16(Parcel *parcel);

/**
 * @brief Writes an int32 value to Parcel.
 *
 * @param parcel The Parcel object pointer.
 * @param value The int32 value to write.
 * @return true on success, false on failure.
 * @since 1.0
 * @version 1.0
 */
bool Parcel_WriteInt32(Parcel *parcel, int32_t value);

/**
 * @brief Reads an int32 value from Parcel.
 *
 * @param parcel The Parcel object pointer.
 * @return The int32 value read from Parcel.
 * @since 1.0
 * @version 1.0
 */
int32_t Parcel_ReadInt32(Parcel *parcel);

/**
 * @brief Writes an int64 value to Parcel.
 *
 * @param parcel The Parcel object pointer.
 * @param value The int64 value to write.
 * @return true on success, false on failure.
 * @since 1.0
 * @version 1.0
 */
bool Parcel_WriteInt64(Parcel *parcel, int64_t value);

/**
 * @brief Reads an int64 value from Parcel.
 *
 * @param parcel The Parcel object pointer.
 * @return The int64 value read from Parcel.
 * @since 1.0
 * @version 1.0
 */
int64_t Parcel_ReadInt64(Parcel *parcel);

/**
 * @brief writes a uint8 value to Parcel.
 *
 * @param parcel The Parcel object pointer.
 * @param value The uint8 value to write.
 * @return true on success, false on failure.
 * @since 1.0
 * @version 1.0
 */
bool Parcel_WriteUint8(Parcel *parcel, uint8_t value);

/**
 * @brief Reads a uint8 value from Parcel.
 *
 * @param parcel The Parcel object pointer.
 * @return The uint8 value read from Parcel.
 * @since 1.0
 * @version 1.0
 */
uint8_t Parcel_ReadUint8(Parcel *parcel);

/**
 * @brief Writes a uint16 value to Parcel.
 *
 * @param parcel The Parcel object pointer.
 * @param value The uint16 value to write.
 * @return true on success, false on failure.
 * @since 1.0
 * @version 1.0
 */
bool Parcel_WriteUint16(Parcel *parcel, uint16_t value);

/**
 * @brief Reads a uint16 value from Parcel.
 *
 * @param parcel The Parcel object pointer.
 * @return The uint16 value read from Parcel.
 * @since 1.0
 * @version 1.0
 */
uint16_t Parcel_ReadUint16(Parcel *parcel);

/**
 * @brief Writes a uint32 value to Parcel.
 *
 * @param parcel The Parcel object pointer.
 * @param value The uint32 value to write.
 * @return true on success, false on failure.
 * @since 1.0
 * @version 1.0
 */
bool Parcel_WriteUint32(Parcel *parcel, uint32_t value);

/**
 * @brief Reads a uint32 value from Parcel.
 *
 * @param parcel The Parcel object pointer.
 * @return The uint32 value read from Parcel.
 * @since 1.0
 * @version 1.0
 */
uint32_t Parcel_ReadUint32(Parcel *parcel);

/**
 * @brief Writes a uint64 value to Parcel.
 *
 * @param parcel The Parcel object pointer.
 * @param value The uint64 value to write.
 * @return true on success, false on failure.
 * @since 1.0
 * @version 1.0
 */
bool Parcel_WriteUint64(Parcel *parcel, uint64_t value);

/**
 * @brief Reads a uint64 value from Parcel.
 *
 * @param parcel The Parcel object pointer.
 * @return The uint64 value read from Parcel.
 * @since 1.0
 * @version 1.0
 */
uint64_t Parcel_ReadUint64(Parcel *parcel);

/**
 * @brief Writes a float value to Parcel.
 *
 * @param parcel The Parcel object pointer.
 * @param value The float value to write.
 * @return true on success, false on failure.
 * @since 1.0
 * @version 1.0
 */
bool Parcel_WriteFloat(Parcel *parcel, float value);

/**
 * @brief Reads a float value from Parcel.
 *
 * @param parcel The Parcel object pointer.
 * @return The float value read from Parcel.
 * @since 1.0
 * @version 1.0
 */
float Parcel_ReadFloat(Parcel *parcel);

/**
 * @brief Writes a double value to Parcel.
 *
 * @param parcel The Parcel object pointer.
 * @param value The double value to write.
 * @return true on success, false on failure.
 * @since 1.0
 * @version 1.0
 */
bool Parcel_WriteDouble(Parcel *parcel, double value);

/**
 * @brief Reads a double value from Parcel.
 *
 * @param parcel The Parcel object pointer.
 * @return The double value read from Parcel.
 * @since 1.0
 * @version 1.0
 */
double Parcel_ReadDouble(Parcel *parcel);

/**
 * @brief Writes a pointer value to Parcel.
 *
 * @param parcel The Parcel object pointer.
 * @param value The pointer value to write.
 * @return true on success, false on failure.
 * @note The pointer is stored as a numeric value, not the actual data it points to.
 * @since 1.0
 * @version 1.0
 */
bool Parcel_WritePointer(Parcel *parcel, void *value);

/**
 * @brief Reads a pointer value from Parcel.
 *
 * @param parcel The Parcel object pointer.
 * @return The pointer value read from Parcel.
 * @since 1.0
 * @version 1.0
 */
void *Parcel_ReadPointer(Parcel *parcel);

/**
 * @brief Writes a data buffer to Parcel.
 *
 * @param parcel The Parcel object pointer.
 * @param data The data buffer to write.
 * @param size The size of the data buffer in bytes.
 * @return true on success, false on failure.
 * @since 1.0
 * @version 1.0
 */
bool Parcel_WriteBuffer(Parcel *parcel, void *data, size_t size);

/**
 * @brief Reads a data buffer from Parcel.
 *
 * @param parcel The Parcel object pointer.
 * @param length The expected length of the buffer to read.
 * @return Pointer to the read buffer, or NULL on failure.
 * @note The returned buffer is owned by Parcel and should not be freed by caller.
 * @since 1.0
 * @version 1.0
 */
void *Parcel_ReadBuffer(Parcel *parcel, size_t length);

/**
 * @brief Writes a C string to Parcel.
 *
 * @param parcel The Parcel object pointer.
 * @param value The C string to write (null-terminated).
 * @return true on success, false on failure.
 * @note The string length (including null terminator) is written before the string data.
 * @since 1.0
 * @version 1.0
 */
bool Parcel_WriteCString(Parcel *parcel, char *value);

/**
 * @brief Reads a C string from Parcel.
 *
 * @param pointer to the Parcel object.
 * @return Pointer to the read C string, or NULL on failure.
 * @note The returned string is owned by Parcel and should not be freed by caller.
 * @since 1.0
 * @version 1.0
 */
char *Parcel_ReadCString(Parcel *parcel);

#ifdef __cplusplus
}
#endif

#endif // AMEBA_AUDIO_BASE_CUTILS_INCLUDE_CUTILS_PARCEL_H
/** @} */
