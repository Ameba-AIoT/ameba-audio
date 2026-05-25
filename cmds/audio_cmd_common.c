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

#include "audio_cmd_common.h"
#include "ameba_soc.h"
#include "os_wrapper.h"
#include <string.h>
#include <stdlib.h>

typedef struct {
    cmd_params_t params;
    cmd_handler_t handler;
    const char *cmd_name;
} cmd_task_param_t;

void cmd_dispatch_task(void *param)
{
    cmd_task_param_t *task_param = (cmd_task_param_t *)param;

    RTK_LOGD(AUDIO_CMD_TAG, "[%s] task started\n", task_param->cmd_name);

    task_param->handler(&task_param->params);

    if (task_param->params.original_argv) {
        for (int i = 0; i < task_param->params.argc; i++) {
            if (task_param->params.original_argv[i]) {
                free(task_param->params.original_argv[i]);
            }
        }
        free(task_param->params.original_argv);
    }

    RTK_LOGD(AUDIO_CMD_TAG, "[%s] task exited\n", task_param->cmd_name);

    free(task_param);

    rtos_task_delete(NULL);
}

uint32_t cmd_create_task(const char *task_name,
                                cmd_handler_t handler,
                                uint16_t argc,
                                uint8_t **argv,
                                uint32_t priority)
{
    cmd_task_param_t *task_param = malloc(sizeof(cmd_task_param_t));
    if (!task_param) {
        RTK_LOGE(AUDIO_CMD_TAG, "Failed to allocate task param for %s\n", task_name);
        return FALSE;
    }

    task_param->params.argc = argc;
    task_param->handler = handler;
    task_param->cmd_name = task_name;

    if (argc != 0) {
        task_param->params.original_argv = malloc(argc * sizeof(char *));
        if (!task_param->params.original_argv) {
            free(task_param);
            return FALSE;
        }

        char **char_argv = (char **)argv;
        for (int i = 0; i < argc; i++) {
            task_param->params.original_argv[i] = malloc(strlen(char_argv[i]) + 1);
            if (!task_param->params.original_argv[i]) {
                for (int j = 0; j < i; j++) {
                    free(task_param->params.original_argv[j]);
                }
                free(task_param->params.original_argv);
                free(task_param);
                return FALSE;
            }
            strcpy(task_param->params.original_argv[i], char_argv[i]);
        }

        task_param->params.argv = task_param->params.original_argv;
    } else {
        task_param->params.argv = task_param->params.original_argv = NULL;
    }

    if (rtos_task_create(NULL, task_name, cmd_dispatch_task, task_param,
                        2560, priority) != RTK_SUCCESS) {
        RTK_LOGE(AUDIO_CMD_TAG, "Failed to create task %s\n", task_name);

        for (int i = 0; i < argc; i++) {
            free(task_param->params.original_argv[i]);
        }
        free(task_param->params.original_argv);
        free(task_param);
        return FALSE;
    }

    return TRUE;
}
