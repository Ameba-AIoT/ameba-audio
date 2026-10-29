#ifndef _EXAMPLE_EQUALIZER_TUNE_HANDLER_H_
#define _EXAMPLE_EQUALIZER_TUNE_HANDLER_H_

//if user wants to test mem, set static int32_t g_max_receive_cnt = 5; or other small values.
//lite example: using PB_4 for tx, PB_5 for rx, set EQUALIZER_TUNE_INDEX 0;
//smart example: using PB_11 for tx, PB_10 for rx, set EQUALIZER_TUNE_INDEX 1;
#define EQUALIZER_TUNE_INDEX                1
#define EQUALIZER_TUNE_TX_PIN               PB_11
#define EQUALIZER_TUNE_RX_PIN               PB_10
#define EQUALIZER_TUNE_USE_DMA_TX           1
#define EQUALIZER_TUNE_BAUDRATE             1500000
#define EQUALIZER_TUNE_RECEIVE_DATA_BUFLEN  1024

#define EQUALIZER_TUNE_MSG_ACK              0x00
#define EQUALIZER_TUNE_MSG_ERROR            0x01
#define EQUALIZER_TUNE_MSG_SET_FREQUENCY    0x02
#define EQUALIZER_TUNE_MSG_SET_GAIN         0x03
#define EQUALIZER_TUNE_MSG_SETQFACTOR       0x04
#define EQUALIZER_TUNE_MSG_SETFILTERTYPE    0x05
#define EQUALIZER_TUNE_MSG_QUERY            0x06

#define cJSON_AddDoubleToObject(object,name,n)       cJSON_AddItemToObject(object, name, cJSON_CreateNumber(n))

void audio_uart_task_start(void);

#endif