/*
 * Copyright (c) 2026 Realtek, LLC.
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

#ifndef AMEBA_AUDIO_CMDS_AUDIO_CMD_COMMON_H
#define AMEBA_AUDIO_CMDS_AUDIO_CMD_COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include "ameba_soc.h"

#define AUDIO_CMD_TAG "AudioCmd"

typedef struct {
    int argc;
    char **argv;
    char **original_argv;
} cmd_params_t;

typedef uint32_t (*cmd_handler_t)(cmd_params_t *params);

typedef struct {
    const char *name;
    cmd_handler_t handler;
    const char *help;
} cmd_entry_t;

typedef struct {
    unsigned int start;
    unsigned int end;
    unsigned int min_ever_free;
} mem_debug_info_t;

#define MEM_DEBUG_INIT(info) \
    do { \
        (info).start = rtos_mem_get_free_heap_size(); \
        RTK_LOGD(AUDIO_CMD_TAG, "[Mem] start (%d-0x%x)\n", (int)(info).start, (unsigned int)(info).start); \
    } while (0)

#define MEM_DEBUG_DUMP(info) \
    do { \
        (info).end = rtos_mem_get_free_heap_size(); \
        (info).min_ever_free = rtos_mem_get_minimum_ever_free_heap_size(); \
        RTK_LOGD(AUDIO_CMD_TAG, "[Mem] start (0x%x), end (0x%x), ", (info).start, (info).end); \
        RTK_LOGD(AUDIO_CMD_TAG, "diff (%d), peak (%d)\n", \
               (info).start - (info).end, \
               (info).start - (info).min_ever_free); \
    } while (0)

#define MEM_DEBUG_DECLARE(name) mem_debug_info_t name

#define CMD_PARSE_INT(var, arg_name, default_val) \
    do { \
        var = default_val; \
        for (int i = 0; i < params->argc; i++) { \
            if (strcmp(params->argv[i], arg_name) == 0 && i + 1 < params->argc) { \
                var = atoi(params->argv[i + 1]); \
                break; \
            } \
        } \
    } while (0)

#define CMD_PARSE_STRING(var, max_len, arg_name, default_val) \
    do { \
        strncpy(var, default_val, max_len - 1); \
        var[max_len - 1] = '\0'; \
        for (int i = 0; i < params->argc; i++) { \
            if (strcmp(params->argv[i], arg_name) == 0 && i + 1 < params->argc) { \
                strncpy(var, params->argv[i + 1], max_len - 1); \
                var[max_len - 1] = '\0'; \
                break; \
            } \
        } \
    } while (0)

#define CMD_PARSE_BOOL(var, arg_name) \
    do { \
        for (int i = 0; i < params->argc; i++) { \
            if (strcmp(params->argv[i], arg_name) == 0) { \
                var = true; \
                break; \
            } \
        } \
    } while (0)

#define CMD_PARSE_FLOAT(var, arg_name, default_val) \
    do { \
        var = default_val; \
        for (int i = 0; i < params->argc; i++) { \
            if (strcmp(params->argv[i], arg_name) == 0 && i + 1 < params->argc) { \
                var = atof(params->argv[i + 1]); \
                break; \
            } \
        } \
    } while (0)

void cmd_dispatch_task(void *param);

#define DEFINE_CMD_WRAPPER(cmd_name, handler_func, priority) \
    uint32_t cmd_name##_cmd_thread(uint16_t argc, uint8_t *argv[]) { \
        return cmd_create_task(#cmd_name "_task", handler_func, argc, argv, priority); \
    }

uint32_t cmd_create_task(const char *task_name,
                        cmd_handler_t handler,
                        uint16_t argc,
                        uint8_t **argv,
                        uint32_t priority);

#endif