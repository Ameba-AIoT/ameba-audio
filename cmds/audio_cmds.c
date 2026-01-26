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
        (const u8 *)"arecord", 1, arecord_cmd_thread,
        (const u8 *)"\tarecord\n"
                    "\t\ttest cmd: arecord [-r] rate [-b] record_bytes_one_time [-c] record_channels [-m] record_mode [-f] format  \n"
                    "\t\t          [-or] 1:only do record, 0:record then play [-noirq] 1:irq mode, 0:no irq mode [-ref] 1:test ref 0: not test ref\n"
                    "\t\t          [-pres] 1: pressure test, 0: record fixed time [-cxs] mic source for channelx, exp, -c0s: mic src for channel0 \n"
                    "\t\tdefault params: [-r] 16000 [-b] 8192 [-c] 2 [-m] 0 [-f] format 16 [-or] 0 [-noirq] 0 [-ref] 0\n"
                    "\t\trecord_mode: 0:no_afe_pure_data; 1:no_afe_all_data \n"
                    "\t\ttest demo: arecord -r 16000 -b 8192 \n"
                    "\t\ttest noirq, -b should be 8ms bytes: arecord -c 1 -b 256 -noirq 1 -r 16000;\n"
    },
#endif

#ifdef CONFIG_CMD_APLAY
    {
        (const u8 *)"aplay", 1, aplay_cmd_thread,
        (const u8 *)"\taplay\n"
                    "\t\ttest cmd: aplay [-r] rate [-b] write_frames_one_time [-c] track_channels [-f] format\n"
                    "\t\tdefault params: [-r] 16000 [-p] 1024 [-c] 2 [-f] format 16 \n"
                    "\t\ttest demo: aplay -r 48000 -c 1 \n"
                    "\t\tcareful: if you set SINE_GEN_EVERY_TIME as 0, please remember to set -b as [integer * rate * 1 / g_freq]\n"
    },

    {
        (const u8 *)"amixer", 1, amixer_cmd_thread,
        (const u8 *)"\tamixer\n"
                    "\t\ttest cmd: amixer [-v] volume [-m] mute\n"
    },
#endif

#ifdef CONFIG_CMD_PLAYER
    {
        (const u8 *)"player", 1, player_cmd_thread,
        (const u8 *)"\tplayer\n"
                    "\t\t[-f file]        An audio file buffer or path\n"
                    "\t\t[-s 0/1]         Use stream source flag\n"
                    "\t\tExamples:\n"
                    "\t\t1. play a http file:\n"
                    "\t\t   player -f http://aod.cos.tx.xmcdn.com/group72/M02/0A/07/wKgO0F4tEivQbT6uAEBqyNIMu88237.mp3\n"
                    "\t\t2. play stream source\n"
                    "\t\t   player -f buffer -s 1\n"
    },
#endif

#ifdef CONFIG_CMD_PCRECORD
    {
        (const u8 *)"pcrecord", 1, pcrecord_cmd_thread,
        (const u8 *)"\tpcrecord\n"
    },
#endif
};