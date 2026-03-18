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

#include <stdio.h>

#include "ameba_soc.h"

#define AUDIO_MEM_DEBUG_INFO_INIT() \
    unsigned int heap_start;\
    unsigned int heap_end;\
    unsigned int heap_min_ever_free;\
    heap_start = rtos_mem_get_free_heap_size();\
    printf("[Mem]start (%d-0x%x)\n", (int)heap_start, (unsigned int)heap_start);
#define AUDIO_MEM_DEBUG_INFO_DUMP() { \
    heap_end = rtos_mem_get_free_heap_size();\
    heap_min_ever_free = rtos_mem_get_minimum_ever_free_heap_size();\
    printf("[Mem]start (0x%x), end (0x%x), ", heap_start, heap_end);\
    printf(" diff (%d), peak (%d) \n", heap_start - heap_end, heap_start - heap_min_ever_free);\
}


// ----------------------------------------------------------------------
// arecord_cmd
#ifdef CONFIG_CMD_ARECORD
extern uint32_t arecord_cmd_handle(int argc, char *argv[]);

uint32_t arecord_cmd_thread(uint16_t argc, u8 *argv[]) {
    printf("arecord_cmd_thread start.\n");

    AUDIO_MEM_DEBUG_INFO_INIT();

    arecord_cmd_handle(argc, (char **)argv);

    rtos_time_delay_ms(1 * 1000);

    AUDIO_MEM_DEBUG_INFO_DUMP();
    printf("arecord_cmd_thread exit.\n\n\n");

    return TRUE;
}
#endif


// ----------------------------------------------------------------------
// aplay_cmd
#ifdef CONFIG_CMD_APLAY
extern uint32_t aplay_cmd_handle(int argc, char *argv[]);
extern uint32_t amixer_cmd_handle(int argc, char *argv[]);

uint32_t aplay_cmd_thread(uint16_t argc, u8 *argv[]) {
    printf("aplay_cmd_thread start.\n");

    AUDIO_MEM_DEBUG_INFO_INIT();

    aplay_cmd_handle(argc, (char **)argv);

    rtos_time_delay_ms(1 * 1000);

    AUDIO_MEM_DEBUG_INFO_DUMP();
    printf("aplay_cmd_thread exit.\n\n\n");

    return TRUE;
}

uint32_t amixer_cmd_thread(uint16_t argc, u8 *argv[]) {
    printf("amixer_cmd_thread start.\n");

    AUDIO_MEM_DEBUG_INFO_INIT();

    amixer_cmd_handle(argc, (char **)argv);

    rtos_time_delay_ms(1 * 1000);

    AUDIO_MEM_DEBUG_INFO_DUMP();
    printf("amixer_cmd_thread exit.\n\n\n");

    return TRUE;
}
#endif


// ----------------------------------------------------------------------
// player_cmd
#ifdef CONFIG_CMD_PLAYER
extern uint32_t player_cmd_handle(int argc, char *argv[]);

uint32_t player_cmd_thread(uint16_t argc, u8 *argv[]) {
    printf("player_cmd_thread start.\n");

    AUDIO_MEM_DEBUG_INFO_INIT();

    player_cmd_handle(argc, (char **)argv);

    rtos_time_delay_ms(1 * 1000);

    AUDIO_MEM_DEBUG_INFO_DUMP();
    printf("player_cmd_thread exit.\n\n\n");

    return TRUE;
}
#endif


// ----------------------------------------------------------------------
// pcrecord_cmd
#ifdef CONFIG_CMD_PCRECORD
extern uint32_t pcrecord_cmd_handle(int argc, char *argv[]);

uint32_t pcrecord_cmd_thread(uint16_t argc, u8 *argv[]) {
    printf("pcrecord_cmd_thread start.\n");

    AUDIO_MEM_DEBUG_INFO_INIT();

    pcrecord_cmd_handle(argc, (char **)argv);

    rtos_time_delay_ms(1 * 1000);

    AUDIO_MEM_DEBUG_INFO_DUMP();
    printf("pcrecord_cmd_thread exit.\n\n\n");

    return TRUE;
}
#endif


// ----------------------------------------------------------------------
// audio_cmds_table
CMD_TABLE_DATA_SECTION
const COMMAND_TABLE audio_cmd_table[] = {
#ifdef CONFIG_CMD_ARECORD
    {
        "arecord", arecord_cmd_thread
    },
#endif

#ifdef CONFIG_CMD_APLAY
    {
        "aplay", aplay_cmd_thread
    },

    {
        "amixer", amixer_cmd_thread
    },
#endif

#ifdef CONFIG_CMD_PLAYER
    {
        "player", player_cmd_thread
    },
#endif

#ifdef CONFIG_CMD_PCRECORD
    {
        "pcrecord", pcrecord_cmd_thread
    },
#endif
};