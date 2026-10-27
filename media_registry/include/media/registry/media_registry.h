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

#ifndef AMEBA_COMPONENT_AUDIO_MEDIA_REGISTRY_INCLUDE_MEDIA_REGISTRY_MEDIA_REGISTRY_H
#define AMEBA_COMPONENT_AUDIO_MEDIA_REGISTRY_INCLUDE_MEDIA_REGISTRY_MEDIA_REGISTRY_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ----------------------------------------------------------------------
//MediaSourceConfig
typedef void *(*CreateAudioSourceFunc)(const char *);

typedef struct MediaSourceConfig {
    const char *name;
    int prefix_num;
    CreateAudioSourceFunc audio_source_func;
} MediaSourceConfig;

extern MediaSourceConfig gMediaSourceConfigs[];
extern size_t gNumMediaSourceConfigs;


// ----------------------------------------------------------------------
//MediaExtractorConfig
typedef struct MediaExtractorConfig {
	const char *name;
	void *extractor_type;
} MediaExtractorConfig;

extern MediaExtractorConfig gMediaExtractorConfigs[];
extern size_t gNumMediaExtractorConfigs;


// ----------------------------------------------------------------------
//MediaDecoderConfig
typedef void *(*CreateComponentFunc)(
	const char *, const void *,
	void *, void **);

typedef struct MediaDecoderConfig {
	const char *name;
	CreateComponentFunc codec_func;
} MediaDecoderConfig;

extern MediaDecoderConfig gMediaDecoderConfigs[];
extern size_t gNumMediaDecoderConfigs;


// ----------------------------------------------------------------------
//MediaCache
extern int64_t gMediaCacheSizeSingleMax;
extern int64_t gMediaCacheSizeTotalMax;
extern int8_t gMediaCacheable;
extern char *gMediaCacheRegionsStart;

#ifdef __cplusplus
}
#endif

#endif  // AMEBA_COMPONENT_AUDIO_MEDIA_REGISTRY_INCLUDE_MEDIA_REGISTRY_MEDIA_REGISTRY_H
