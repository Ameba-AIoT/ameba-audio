#include "audio_dump.h"

#if SPEECHMIND_AUDIO_DUMP_EN

#define SM_LOG_TAG "AudioDump"

#include <stdlib.h>
#include <string.h>

#include "ameba_soc.h"
#include "os_wrapper.h"
#include "serial_api.h"
#include "serial_ex_api.h"
#include "PinNames.h"
#include "cJSON/cJSON.h"

#include "audio/audio_type.h"

#include "speech_log.h"
#include "speech_utils.h"
#include "audio_dump.h"

/* Output channels: mic1, mic2, [ref], [afe], interleaved per sample. */
#define AUDIO_DUMP_REF          1
#define AUDIO_DUMP_AFE          1
#define AUDIO_DUMP_CH           (2 + AUDIO_DUMP_REF + AUDIO_DUMP_AFE)

/* UART and pc msg handling follow cmds/pcrecord/pcrecord.c, so RtkAudioRecorder works unchanged. */
#define PR_UART_TX              PA_25
#define PR_UART_RX              PA_24
#define PR_UART_RTS             PA_30
#define PR_UART_CTS             PA_31
#define PR_UART_FLOW_CTRL       0
#define PR_BAUDRATE             1500000
#define PR_VERSION              "1.0"

#define PC_BUFLEN               512
#define RECORD_PAGE_NUM         2
#define RECORD_PAGE_SIZE        2048

#define AUDIO_DUMP_FIFO_FRAMES  8
#define AUDIO_DUMP_STACK_SIZE   3072
#define AUDIO_DUMP_TASK_PRI     5

#define FRAME_SAMPLES           (SAMPLE_RATE * FRAME_MS / 1000)
#define DUMP_ALIGN              64      /* dcache line, buffers are DMA'd */

enum {
    PR_MSG_CONFIG,
    PR_MSG_START,
    PR_MSG_STOP,
    PR_MSG_QUERY,
};

static const struct {
    int item;
    const char *item_str;
} pr_msg_type[] = {
    {PR_MSG_CONFIG,     "config"    },
    {PR_MSG_START,      "start"     },
    {PR_MSG_STOP,       "stop"      },
    {PR_MSG_QUERY,      "query"     },
};

/* Single producer (Feed) / single consumer (dump task) frame queue. */
typedef struct {
    int16_t *frames;
    uint32_t seq[AUDIO_DUMP_FIFO_FRAMES];
    uint32_t samples;       /* samples per frame */
    volatile uint32_t wr;
    volatile uint32_t rd;
    uint32_t drop;
} DumpFifo;

typedef struct {
    serial_t sobj;
    rtos_task_t task;
    rtos_sema_t sema;           /* wakes the task: rx msg or new frame */
    rtos_sema_t dma_tx_sema;    /* held while uart dma tx is in flight */
    rtos_mutex_t lock;          /* guards fifo lifetime against Feed */
    volatile uint8_t exit;
    volatile uint8_t recording;
    uint8_t configured;

    volatile uint8_t msg_ready; /* pc_buf holds a msg, rx irq leaves it alone until processed */
    int start_cnt;
    int end_cnt;
    int pc_datasize;
    unsigned char pc_buf[PC_BUFLEN];

    uint32_t raw_cnt;           /* frames accepted by aivoice */
    DumpFifo raw;
#if AUDIO_DUMP_AFE
    uint32_t afe_cnt;           /* afe frames output by aivoice */
    DumpFifo afe;
#endif
    void *fifo_mem;

    uint8_t fill;               /* record_buf page being filled */
    uint16_t pos;               /* samples in that page */
    int tx_cnt;
} AudioDump;

static AudioDump s_dump;
static char record_buf[RECORD_PAGE_NUM * RECORD_PAGE_SIZE] __attribute__((aligned(DUMP_ALIGN)));

/* only the dump task sends, so no tx mutex as in pcrecord */
static void pr_uart_send_string(char *pstr, int len)
{
    int32_t ret = 0;
    rtos_sema_take(s_dump.dma_tx_sema, RTOS_MAX_TIMEOUT);

    ret = serial_send_stream_dma(&s_dump.sobj, pstr, len);
    if (ret != 0) {
        SM_LOGE("%s Error(%d)", __func__, (int)ret);
        /* no tx done irq will come, reset tx state and release sema to avoid blocking later sends */
        serial_send_stream_abort(&s_dump.sobj);
        rtos_sema_give(s_dump.dma_tx_sema);
    }
}

/* wait until dma tx done, call it before freeing the buffer passed to pr_uart_send_string */
static void pr_uart_wait_tx_done(void)
{
    rtos_sema_take(s_dump.dma_tx_sema, RTOS_MAX_TIMEOUT);
    rtos_sema_give(s_dump.dma_tx_sema);
}

static void pr_uart_irq(uint32_t id, SerialIrq event)
{
    serial_t *sobj = (serial_t *)id;
    unsigned char rc = 0;

    if (event == RxIrq) {
        rc = serial_getc(sobj);

        /* the pc tool waits for the reply before sending the next msg */
        if (s_dump.msg_ready) {
            return;
        }

        /* unbalanced '{'/'}' from line noise keeps accumulating, drop the partial msg instead of overrunning pc_buf.
         * keep 1 byte spare for '\0' */
        if (s_dump.pc_datasize >= PC_BUFLEN - 1) {
            s_dump.pc_datasize = 0;
            s_dump.start_cnt = 0;
            s_dump.end_cnt = 0;
        }

        if (rc == '{') {
            s_dump.start_cnt++;
        } else if (rc == '}') {
            s_dump.end_cnt++;
        }

        s_dump.pc_buf[s_dump.pc_datasize++] = rc;

        if ((s_dump.start_cnt != 0) && s_dump.start_cnt == s_dump.end_cnt) {
            s_dump.start_cnt = 0;
            s_dump.end_cnt = 0;
            s_dump.pc_buf[s_dump.pc_datasize] = '\0';
            s_dump.pc_datasize = 0;
            s_dump.msg_ready = 1;
            rtos_sema_give(s_dump.sema);
        }
    }
}

static void uart_dma_tx_done(uint32_t id)
{
    (void) id;
    rtos_sema_give(s_dump.dma_tx_sema);
}

static void pr_uart_init_mbed(void)
{
    s_dump.sobj.uart_idx = 0;
    serial_init(&s_dump.sobj, PR_UART_TX, PR_UART_RX);
    serial_baud(&s_dump.sobj, PR_BAUDRATE);
    serial_format(&s_dump.sobj, 8, ParityNone, 1);

    serial_rx_fifo_level(&s_dump.sobj, FifoLvHalf);
#if PR_UART_FLOW_CTRL
    serial_set_flow_control(&s_dump.sobj, FlowControlRTSCTS, PR_UART_RTS, PR_UART_CTS);
#else
    serial_set_flow_control(&s_dump.sobj, FlowControlNone, 0, 0);
#endif
    serial_irq_handler(&s_dump.sobj, pr_uart_irq, (uint32_t)&s_dump.sobj);
    serial_irq_set(&s_dump.sobj, RxIrq, ENABLE);
    serial_send_comp_handler(&s_dump.sobj, (void *)uart_dma_tx_done, (uint32_t)&s_dump.sobj);
}

static int pr_audiorecord_query(void)
{
    cJSON *msg_obj;
    char *msg_js = NULL;

    if ((msg_obj = cJSON_CreateObject()) == NULL) {
        return -1;
    }

    cJSON_AddStringToObject(msg_obj, "type", "query");
    cJSON_AddStringToObject(msg_obj, "version", PR_VERSION);
    cJSON_AddStringToObject(msg_obj, "status", s_dump.recording ? "busy" : "idle");

    msg_js = cJSON_Print(msg_obj);
    cJSON_Delete(msg_obj);
    if (!msg_js) {
        return -1;
    }

    SM_LOGI("%s, %s", __func__, msg_js);
    pr_uart_send_string(msg_js, strlen(msg_js));
    pr_uart_wait_tx_done();
    cJSON_free(msg_js);

    return 0;
}

static int pc_msg_response_ack(int opt)
{
    cJSON *msg_obj;
    char *msg_js = NULL;

    if ((msg_obj = cJSON_CreateObject()) == NULL) {
        return -1;
    }

    if (opt >= 0) {
        cJSON_AddStringToObject(msg_obj, "type", "ack");
    } else {
        cJSON_AddStringToObject(msg_obj, "type", "error");
    }

    msg_js = cJSON_Print(msg_obj);
    cJSON_Delete(msg_obj);
    if (!msg_js) {
        return -1;
    }

    SM_LOGI("%s, %s", __func__, msg_js);
    pr_uart_send_string(msg_js, strlen(msg_js));
    pr_uart_wait_tx_done();
    cJSON_free(msg_js);

    return 0;
}

static void pc_msg_audio_data_binary(int seq, char *data, int data_len)
{
    unsigned char buf[16];
    unsigned char checksum = 0;
    unsigned int offset = 0;
    short len = data_len;

    for (int i = 0; i < data_len; i++) {
        checksum ^= data[i];
    }

    memcpy(buf, "data", strlen("data"));
    offset += strlen("data");
    memcpy(buf + offset, &seq, 4);
    offset += 4;
    memcpy(buf + offset, &len, 2);
    offset += 2;
    buf[offset] = checksum;
    offset += 1;
    /* buf is on stack, but sending data waits for it to finish */
    pr_uart_send_string((char *)buf, offset);
    pr_uart_send_string(data, data_len);
}

static void DumpFifo_Push(DumpFifo *fifo, const int16_t *pcm, uint32_t seq)
{
    uint32_t idx = fifo->wr % AUDIO_DUMP_FIFO_FRAMES;

    if (fifo->wr - fifo->rd >= AUDIO_DUMP_FIFO_FRAMES) {
        fifo->drop++;
        return;
    }

    memcpy(fifo->frames + idx * fifo->samples, pcm, fifo->samples * sizeof(int16_t));
    fifo->seq[idx] = seq;
    __sync_synchronize();
    fifo->wr++;
}

static void AudioDump_Feed(DumpFifo *fifo, const int16_t *pcm, uint32_t seq)
{
    rtos_mutex_take(s_dump.lock, RTOS_MAX_TIMEOUT);
    if (s_dump.recording) {
        DumpFifo_Push(fifo, pcm, seq);
    }
    rtos_mutex_give(s_dump.lock);
    rtos_sema_give(s_dump.sema);
}

void AudioDump_FeedRaw(const int16_t *pcm)
{
    uint32_t seq = s_dump.raw_cnt++;

    if (s_dump.recording) {
        AudioDump_Feed(&s_dump.raw, pcm, seq);
    }
}

void AudioDump_FeedAfe(const int16_t *pcm)
{
#if AUDIO_DUMP_AFE
    uint32_t seq = s_dump.afe_cnt++;

    if (s_dump.recording) {
#if defined(CONFIG_AMEBALITE)
        /* written by dsp */
        DCache_Invalidate((uint32_t)pcm, FRAME_SAMPLES * sizeof(int16_t));
#endif
        AudioDump_Feed(&s_dump.afe, pcm, seq);
    }
#else
    UNUSED(pcm);
#endif
}

static void AudioDump_PutSample(int16_t s)
{
    char *page = record_buf + s_dump.fill * RECORD_PAGE_SIZE;

    ((int16_t *)page)[s_dump.pos++] = s;
    if (s_dump.pos < RECORD_PAGE_SIZE / sizeof(int16_t)) {
        return;
    }

    /* the other page may still be in flight, sending this one waits for it */
    pc_msg_audio_data_binary(++s_dump.tx_cnt, page, RECORD_PAGE_SIZE);
    s_dump.fill ^= 1;
    s_dump.pos = 0;
}

static void AudioDump_Drain(void)
{
    DumpFifo *raw = &s_dump.raw;
    const int16_t *r;
    uint32_t i;
#if AUDIO_DUMP_AFE
    DumpFifo *afe = &s_dump.afe;
    const int16_t *a;
    int32_t diff;
#endif

    while (raw->wr != raw->rd) {
#if AUDIO_DUMP_AFE
        if (afe->wr == afe->rd) {
            break;
        }
        __sync_synchronize();
        /* pair by frame index; drop the older one if its partner was dropped or still in the afe pipeline at start */
        diff = (int32_t)(raw->seq[raw->rd % AUDIO_DUMP_FIFO_FRAMES] - afe->seq[afe->rd % AUDIO_DUMP_FIFO_FRAMES]);
        if (diff < 0) {
            raw->rd++;
            continue;
        }
        if (diff > 0) {
            afe->rd++;
            continue;
        }
        a = afe->frames + (afe->rd % AUDIO_DUMP_FIFO_FRAMES) * FRAME_SAMPLES;
#else
        __sync_synchronize();
#endif
        r = raw->frames + (raw->rd % AUDIO_DUMP_FIFO_FRAMES) * FRAME_SAMPLES * IN_CHANNEL;
        for (i = 0; i < FRAME_SAMPLES; i++, r += IN_CHANNEL) {
            AudioDump_PutSample(r[0]);
            AudioDump_PutSample(r[1]);
#if AUDIO_DUMP_REF
            AudioDump_PutSample(r[2]);
#endif
#if AUDIO_DUMP_AFE
            AudioDump_PutSample(a[i]);
#endif
        }
        __sync_synchronize();
        raw->rd++;
#if AUDIO_DUMP_AFE
        afe->rd++;
#endif
    }
}

static int pr_audiorecord_config(const cJSON *root)
{
    cJSON *record, *samplerate, *format, *chnum;

    record = cJSON_GetObjectItem(root, "record");
    samplerate = cJSON_GetObjectItem(record, "sample_rate");
    format = cJSON_GetObjectItem(record, "format");
    chnum = cJSON_GetObjectItem(record, "chl_num");
    if (s_dump.recording || !samplerate || !format || !chnum) {
        return -1;
    }

    SM_LOGI("samplerate %d format %d chnum %d", samplerate->valueint, format->valueint, chnum->valueint);
    if (samplerate->valueint != SAMPLE_RATE || format->valueint != AUDIO_FORMAT_PCM_16_BIT || chnum->valueint != AUDIO_DUMP_CH) {
        SM_LOGE("need %d ch, 16k 16bit", AUDIO_DUMP_CH);
        return -1;
    }

    s_dump.configured = 1;
    return 0;
}

static int pr_audiorecord_start(void)
{
    uint32_t raw_bytes = AUDIO_DUMP_FIFO_FRAMES * FRAME_SAMPLES * IN_CHANNEL * sizeof(int16_t);
    uint32_t size = raw_bytes;

    if (!s_dump.configured || s_dump.recording) {
        return -1;
    }

#if AUDIO_DUMP_AFE
    size += AUDIO_DUMP_FIFO_FRAMES * FRAME_SAMPLES * sizeof(int16_t);
#endif
    s_dump.fifo_mem = rtos_mem_malloc(size);
    if (!s_dump.fifo_mem) {
        SM_LOGE("no mem %u", size);
        return -1;
    }

    s_dump.raw.frames = (int16_t *)s_dump.fifo_mem;
    s_dump.raw.samples = FRAME_SAMPLES * IN_CHANNEL;
    s_dump.raw.wr = s_dump.raw.rd = s_dump.raw.drop = 0;
#if AUDIO_DUMP_AFE
    s_dump.afe.frames = (int16_t *)((uint8_t *)s_dump.fifo_mem + raw_bytes);
    s_dump.afe.samples = FRAME_SAMPLES;
    s_dump.afe.wr = s_dump.afe.rd = s_dump.afe.drop = 0;
#endif
    s_dump.fill = 0;
    s_dump.pos = 0;
    s_dump.tx_cnt = 0;

    rtos_mutex_take(s_dump.lock, RTOS_MAX_TIMEOUT);
    s_dump.recording = 1;
    rtos_mutex_give(s_dump.lock);
    SM_LOGI("start %d ch", AUDIO_DUMP_CH);
    return 0;
}

static int pr_audiorecord_stop(void)
{
    if (!s_dump.recording) {
        return 0;
    }

    rtos_mutex_take(s_dump.lock, RTOS_MAX_TIMEOUT);
    s_dump.recording = 0;
    rtos_mutex_give(s_dump.lock);

    pr_uart_wait_tx_done();
    rtos_mem_free(s_dump.fifo_mem);
    s_dump.fifo_mem = NULL;
#if AUDIO_DUMP_AFE
    SM_LOGI("stop, pages %d drop raw %u afe %u", s_dump.tx_cnt, s_dump.raw.drop, s_dump.afe.drop);
#else
    SM_LOGI("stop, pages %d drop raw %u", s_dump.tx_cnt, s_dump.raw.drop);
#endif
    return 0;
}

static void pc_msg_process(void)
{
    cJSON *root, *typeobj;
    int type = -1;
    int opt = -1;

    SM_LOGI("%s, msg: %s", __func__, s_dump.pc_buf);

    if ((root = cJSON_Parse((const char *)s_dump.pc_buf)) != NULL) {
        if ((typeobj = cJSON_GetObjectItem(root, "type")) != NULL && cJSON_IsString(typeobj)) {
            for (unsigned int i = 0; i < sizeof(pr_msg_type) / sizeof(pr_msg_type[0]); i++) {
                if (!strcmp(typeobj->valuestring, pr_msg_type[i].item_str)) {
                    type = pr_msg_type[i].item;
                    break;
                }
            }
        }
    }

    switch (type) {
    case PR_MSG_CONFIG:
        opt = pr_audiorecord_config(root);
        pc_msg_response_ack(opt);
        break;
    case PR_MSG_START:
        opt = pr_audiorecord_start();
        pc_msg_response_ack(opt);
        break;
    case PR_MSG_STOP:
        opt = pr_audiorecord_stop();
        pc_msg_response_ack(opt);
        break;
    case PR_MSG_QUERY:
        opt = pr_audiorecord_query();
        if (opt != 0) {
            pc_msg_response_ack(opt);
        }
        break;
    default:
        SM_LOGE("%s, unsupport type", __func__);
        pc_msg_response_ack(opt);
    }

    cJSON_Delete(root);
}

static void AudioDump_Task(void *param)
{
    UNUSED(param);

    while (!s_dump.exit) {
        rtos_sema_take(s_dump.sema, RTOS_MAX_TIMEOUT);

        if (s_dump.msg_ready) {
            pc_msg_process();
            s_dump.msg_ready = 0;
        }

        if (s_dump.recording) {
            AudioDump_Drain();
        }
    }

    pr_audiorecord_stop();
    s_dump.task = NULL;
    rtos_task_delete(NULL);
}

static void AudioDump_FreeSync(void)
{
    if (s_dump.sema) {
        rtos_sema_delete(s_dump.sema);
        s_dump.sema = NULL;
    }
    if (s_dump.dma_tx_sema) {
        rtos_sema_delete(s_dump.dma_tx_sema);
        s_dump.dma_tx_sema = NULL;
    }
    if (s_dump.lock) {
        rtos_mutex_delete(s_dump.lock);
        s_dump.lock = NULL;
    }
}

void AudioDump_Init(void)
{
    if (s_dump.task) {
        return;
    }

    s_dump.exit = 0;
    s_dump.configured = 0;
    s_dump.msg_ready = 0;
    s_dump.start_cnt = 0;
    s_dump.end_cnt = 0;
    s_dump.pc_datasize = 0;
    /* aivoice is created along with dump, both counters start from its first frame */
    s_dump.raw_cnt = 0;
#if AUDIO_DUMP_AFE
    s_dump.afe_cnt = 0;
#endif

    if (rtos_sema_create(&s_dump.sema, 0, 1) != RTK_SUCCESS || rtos_sema_create(&s_dump.dma_tx_sema, 1, 1) != RTK_SUCCESS ||
        rtos_mutex_create(&s_dump.lock) != RTK_SUCCESS) {
        goto fail;
    }

    pr_uart_init_mbed();

    if (rtos_task_create(&s_dump.task, "audio_dump", AudioDump_Task, NULL, AUDIO_DUMP_STACK_SIZE, AUDIO_DUMP_TASK_PRI) != RTK_SUCCESS) {
        s_dump.task = NULL;
        serial_free(&s_dump.sobj);
        goto fail;
    }

    SM_LOGI("init, uart %d baud", PR_BAUDRATE);
    return;

fail:
    SM_LOGE("init failed");
    AudioDump_FreeSync();
}

void AudioDump_Deinit(void)
{
    if (!s_dump.task) {
        return;
    }

    serial_irq_set(&s_dump.sobj, RxIrq, DISABLE);
    s_dump.exit = 1;
    rtos_sema_give(s_dump.sema);
    while (s_dump.task) {
        rtos_time_delay_ms(10);
    }

    serial_free(&s_dump.sobj);
    AudioDump_FreeSync();
}

#endif /* SPEECHMIND_AUDIO_DUMP_EN */
