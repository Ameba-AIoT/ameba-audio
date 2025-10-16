/*
* Copyright (c) 2025 Realtek Corp.
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

#ifndef AMEBA_AUDIO_BASE_CUTILS_INCLUDE_CUTILS_COMPAT_H
#define AMEBA_AUDIO_BASE_CUTILS_INCLUDE_CUTILS_COMPAT_H

#if !defined(__linux__)

#ifndef __ICCARM__
#include <machine/endian.h>
#endif

typedef off_t off64_t;

#ifndef SSIZE_MAX
#define SSIZE_MAX INT_MAX // Assume 32bit system
#endif

#ifndef ntohl
#define ntohl(x)  __ntohl(x)
#endif

#ifndef htonl
#define htonl(x)  __htonl(x)
#endif

#ifndef ntohs
#define ntohs(x)  __ntohs(x)
#endif

#ifndef htons
#define htons(x)  __htons(x)
#endif

#define bswap_16(x) __builtin_bswap16(x)

#define typeof(x) __typeof(x)

#endif

/*
 * Needed for cases where something should be constexpr if possible, but not
 * being constexpr is fine if in pre-C++11 code (such as a const static float
 * member variable).
 */
#if defined(__cplusplus) && __cplusplus >= 201103L
#define CONSTEXPR constexpr
#else
#define CONSTEXPR
#endif

#endif // AMEBA_AUDIO_BASE_CUTILS_INCLUDE_CUTILS_COMPAT_H
