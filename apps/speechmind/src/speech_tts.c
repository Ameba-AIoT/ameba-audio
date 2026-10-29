
#include <stddef.h>
#include <string.h>

#include "speech_tts.h"

typedef struct TtsContentName {
    const char *content;
    const char *name;
} TtsContentName;

/* const so this table lands in .rodata (XIP flash) instead of .data (RAM):
 * it is read-only lookup data, 2240 bytes of it. Same for tts_names below. */
static const IntentTtsInfo intents[] = {
    {1, 1, {{0, "1", NULL, NULL}, }},
    {2, 1, {{0, "2", NULL, NULL}, }},
    {3, 2, {{0, "3", NULL, NULL}, {0, "4", NULL, NULL}, }},
    {4, 2, {{0, "3", NULL, NULL}, {0, "5", NULL, NULL}, }},
    {5, 2, {{0, "3", NULL, NULL}, {0, "6", NULL, NULL}, }},
    {6, 2, {{0, "3", NULL, NULL}, {0, "7", NULL, NULL}, }},
    {7, 2, {{0, "3", NULL, NULL}, {0, "8", NULL, NULL}, }},
    {8, 2, {{0, "3", NULL, NULL}, {0, "9", NULL, NULL}, }},
    {9, 2, {{0, "3", NULL, NULL}, {0, "10", NULL, NULL}, }},
    {10, 2, {{0, "3", NULL, NULL}, {0, "11", NULL, NULL}, }},
    {11, 2, {{0, "3", NULL, NULL}, {0, "12", NULL, NULL}, }},
    {12, 2, {{0, "3", NULL, NULL}, {0, "13", NULL, NULL}, }},
    {13, 2, {{0, "3", NULL, NULL}, {0, "14", NULL, NULL}, }},
    {14, 2, {{0, "3", NULL, NULL}, {0, "15", NULL, NULL}, }},
    {15, 2, {{0, "3", NULL, NULL}, {0, "16", NULL, NULL}, }},
    {16, 2, {{0, "3", NULL, NULL}, {0, "17", NULL, NULL}, }},
    {17, 2, {{0, "3", NULL, NULL}, {0, "18", NULL, NULL}, }},
    {18, 2, {{0, "3", NULL, NULL}, {0, "19", NULL, NULL}, }},
    {19, 2, {{0, "3", NULL, NULL}, {0, "20", NULL, NULL}, }},
    {20, 2, {{0, "3", NULL, NULL}, {0, "21", NULL, NULL}, }},
    {21, 2, {{0, "3", NULL, NULL}, {0, "22", NULL, NULL}, }},
    {22, 2, {{0, "3", NULL, NULL}, {0, "23", NULL, NULL}, }},
    {23, 1, {{0, "24", NULL, NULL}, }},
    {24, 1, {{0, "24", NULL, NULL}, }},
    {25, 1, {{0, "25", NULL, NULL}, }},
    {26, 1, {{0, "25", NULL, NULL}, }},
    {27, 1, {{0, "25", NULL, NULL}, }},
    {28, 1, {{0, "25", NULL, NULL}, }},
    {29, 1, {{0, "25", NULL, NULL}, }},
    {30, 1, {{0, "25", NULL, NULL}, }},
    {31, 1, {{0, "26", NULL, NULL}, }},
    {32, 1, {{0, "26", NULL, NULL}, }},
    {33, 1, {{0, "26", NULL, NULL}, }},
    {34, 1, {{0, "26", NULL, NULL}, }},
    {35, 2, {{0, "3", NULL, NULL}, {0, "27", NULL, NULL}, }},
    {36, 2, {{0, "3", NULL, NULL}, {0, "28", NULL, NULL}, }},
    {37, 1, {{0, "29", NULL, NULL}, }},
    {38, 1, {{0, "30", NULL, NULL}, }},
    {39, 1, {{0, "31", NULL, NULL}, }},
    {40, 1, {{0, "32", NULL, NULL}, }},

};


static const TtsContentName tts_names[] = {
    {"主人我在", "10001"},
    {"主人我先退下啦有需要请再唤醒我", "10002"},
    {"空调已开机", "1"},
    {"空调已关闭", "2"},
    {"已设为", "3"},
    {"制冷模式", "4"},
    {"制热模式", "5"},
    {"加热模式", "6"},
    {"送风模式", "7"},
    {"除湿模式", "8"},
    {"十六度", "9"},
    {"十七度", "10"},
    {"十八度", "11"},
    {"十九度", "12"},
    {"二十度", "13"},
    {"二十一度", "14"},
    {"二十二度", "15"},
    {"二十三度", "16"},
    {"二十四度", "17"},
    {"二十五度", "18"},
    {"二十六度", "19"},
    {"二十七度", "20"},
    {"二十八度", "21"},
    {"二十九度", "22"},
    {"三十度", "23"},
    {"温度已调整", "24"},
    {"风速已调整", "25"},
    {"风量已调整", "26"},
    {"左右摆风", "27"},
    {"上下摆风", "28"},
    {"开始播放", "29"},
    {"已停止播放", "30"},
    {"电话已接听", "31"},
    {"电话已挂断", "32"},
};

const char *SpeechTts_GetNameByContent(const char *content)
{
    const char *name = NULL;
    for (size_t i = 0; i < sizeof(tts_names) / sizeof(tts_names[0]); i++) {
        if (strcmp(content, tts_names[i].content) == 0) {
            name = tts_names[i].name;
            break;
        }
    }
    return name;
}

const IntentTtsInfo *SpeechTts_GetIntentInfo(int id)
{
    const IntentTtsInfo *info = NULL;
    for (size_t i = 0; i < sizeof(intents) / sizeof(intents[0]); i++) {
        if (intents[i].intent_id == id) {
            info = &intents[i];
            break;
        }
    }
    return info;
}
