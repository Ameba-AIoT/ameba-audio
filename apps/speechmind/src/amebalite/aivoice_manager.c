#define SM_LOG_TAG "SpeechM"
#include <stdio.h>
#include <string.h>
#include "ameba_soc.h"

#include "../speech_log.h"
#include "../speech_utils.h"
#include "common/audio_errnos.h"
#include "osal_c/osal_mem.h"
#include "osal_c/osal_time.h"

#include "cutils/ring_buffer.h"
#include "media/parcel.h"
#include "aivoice_interface.h"

#include "aivoice_manager.h"

#include "rpc_api.h"
#include "rpc_struct.h"
#include "VoiceRPC_Agent.h"
#include "VoiceRPC_Agent_data.h"
#include "VoiceRPC_System.h"
#include "VoiceRPC_System_data.h"
#include "VoiceRPCBaseDS.h"
#include "VoiceRPCBaseDS_data.h"


//#define DUMP_DATA
#ifdef CONFIG_ARM_CORE_CM4
#define VOICE_RPC_MODE (BLOCK_MODE | (RPC_NP_DSP << 1) | (RPC_DSP_NP << 4))
#elif defined(CONFIG_RSICV_CORE_KR4)
#define VOICE_RPC_MODE (BLOCK_MODE | (RPC_AP_DSP << 1) | (RPC_DSP_AP << 4))
#endif

#if defined(CONFIG_IMQ_EN) && CONFIG_IMQ_EN
extern void ipc_app_init(int task_size, int priority, int log_enable);
#endif

/* Mic ring buffer capacity (bytes). Must be a multiple of 128 (cache line);
 * shared with the DSP via the ring_buffer_header. */
#define MIC_RING_BUFFER_SIZE (7680)

static bool g_dsp_inited = false;

struct AIVoiceManager {
    ring_buffer *mic_ring_buffer;
    AIVoiceManagerCallback callback;
    void* usr_data;
    Parcel *parcel;
};

static HRESULT *AIVoiceManager_AIVoiceCallbackProcess(VOICE_RPC_RESULT *pParam, RPC_STRUCT *pRpcStruct, HRESULT *pRes)
{
    UNUSED(pParam);
    UNUSED(pRpcStruct);

    AIVoiceManager *manager = (AIVoiceManager *)pParam->usr_data;
    int event_type = pParam->type;
    void *msg = (void *)pParam->msg_addr;
    uint32_t len = pParam->len;
    DCache_Invalidate((uint32_t)msg, 1024);


    if (manager && manager->callback) {
        manager->callback(event_type, msg, len, manager->usr_data);
    }

    *pRes = 0;
    return pRes;
}

static HRESULT *AIVoiceManager_OnState(VOICE_RPC_ERROR_STATE *pParam, RPC_STRUCT *pRpcStruct, HRESULT *pRes)
{
    UNUSED(pParam);
    UNUSED(pRpcStruct);
    *pRes = 0;
    if (pParam->type == 0) {
        g_dsp_inited = true;
        SM_LOGI("OnState Dsp Inited");
    }

    if (pParam->type == 1) {
        AIVoiceManager *manager = (AIVoiceManager *)pParam->data;
        if (manager->parcel) {
            Parcel_Destroy(manager->parcel);
            manager->parcel = NULL;
        }
    }
    return pRes;
}

static void AIVoiceManager_VoicePreInit(void)
{
    p_VOICE_RPC_ToSystem_Callback_0_svc = AIVoiceManager_AIVoiceCallbackProcess;
    p_VOICE_RPC_ToSystem_State_0_svc = AIVoiceManager_OnState;
}

static void AIVoiceManager_AdjustTo32Multiple(int num, int *ones, int *twos, int *fours, int *eights) {
    int remainder = num % 32;
    int to_add = 0;

    *ones = 0;
    *twos = 0;
    *fours = 0;
    *eights = 0;

    if (remainder != 0) {
        to_add = 32 - remainder;

        *eights = to_add / 8;
        to_add %= 8;

        *fours = to_add / 4;
        to_add %= 4;

        *twos = to_add / 2;
        *ones = to_add % 2;
    }
}

AIVoiceManager *AIVoiceManager_Create(AIVoiceFlowType type, struct aivoice_config *config)
{
    ipc_app_init(1536, 6, 0);   /* ReceiverLoop stack; measured peak ~880-1232B */
    while (!RPC_IsInited()) {
        osal_msleep(5);
    }

    AIVoiceManager_VoicePreInit();
    while (!g_dsp_inited) {
        osal_msleep(1);
    }

    AIVoiceManager *manager = osal_malloc(sizeof(AIVoiceManager));
    if (!manager) {
        SM_LOGE("malloc aivoice manager fail");
        return NULL;
    }

    /* 7680 B = 128 * 60: multiple of the 128B cache line (see ring_buffer.c
     * capacity check). Trimmed from 32768 to save heap; the ring only needs to
     * absorb the mic feed jitter between KM4 write and DSP read. */
    manager->mic_ring_buffer = ring_buffer_create(MIC_RING_BUFFER_SIZE, RINGBUFFER_IPC);
    if (!manager->mic_ring_buffer) {
        SM_LOGE("create mic local ring buffer fail");
        osal_free(manager);
        return NULL;
    }

    VOICE_RPC_INIT arg;
    arg.voice_iface_flag = type;
    arg.mic_header_addr = (uint32_t)manager->mic_ring_buffer->header;
    arg.mic_header_length = sizeof(ring_buffer_header);
    DCache_Clean((uint32_t)manager->mic_ring_buffer->header, sizeof(ring_buffer_header));

    Parcel *parcel = Parcel_Create();

    //afe config
    Parcel_WriteInt32(parcel, config->afe->mic_array);
    Parcel_WriteInt32(parcel, config->afe->ref_num);
    Parcel_WriteInt32(parcel, config->afe->sample_rate);
    Parcel_WriteInt32(parcel, config->afe->frame_size);
    Parcel_WriteInt32(parcel, config->afe->afe_mode);
    Parcel_WriteBool(parcel, config->afe->enable_aec);
    Parcel_WriteBool(parcel, config->afe->enable_ns);
    Parcel_WriteBool(parcel, config->afe->enable_agc);
    Parcel_WriteBool(parcel, config->afe->enable_ssl);
    Parcel_WriteInt32(parcel, config->afe->aec_mode);
    Parcel_WriteInt32(parcel, config->afe->aec_enable_threshold);
    Parcel_WriteBool(parcel, config->afe->enable_res);
    Parcel_WriteInt32(parcel, config->afe->aec_cost);
    Parcel_WriteInt32(parcel, config->afe->res_aggressive_mode);
    Parcel_WriteInt32(parcel, config->afe->ns_mode);
    Parcel_WriteInt32(parcel, config->afe->ns_cost_mode);
    Parcel_WriteInt32(parcel, config->afe->ns_aggressive_mode);
    Parcel_WriteInt32(parcel, config->afe->agc_fixed_gain);
    Parcel_WriteBool(parcel, config->afe->enable_adaptive_agc);
    Parcel_WriteFloat(parcel, config->afe->ssl_resolution);
    Parcel_WriteInt32(parcel, config->afe->ssl_min_hz);
    Parcel_WriteInt32(parcel, config->afe->ssl_max_hz);

    //vad config
    Parcel_WriteInt32(parcel, config->vad->sensitivity);
    Parcel_WriteUint32(parcel, config->vad->left_margin);
    Parcel_WriteUint32(parcel, config->vad->right_margin);
    Parcel_WriteUint32(parcel, config->vad->min_speech_duration);

    //kws config
    int keyword_nums = 2;
    Parcel_WriteInt8(parcel, keyword_nums);
    for (int i = 0; i < keyword_nums; i++) {
        Parcel_WriteCString(parcel, (char *)config->kws->keywords[i]);
        Parcel_WriteFloat(parcel, config->kws->thresholds[i]);
    }
    Parcel_WriteInt32(parcel, config->kws->sensitivity);
    Parcel_WriteInt32(parcel, config->kws->mode);
    Parcel_WriteBool(parcel, config->kws->enable_age_gender);

    //asr config
    Parcel_WriteInt32(parcel, config->asr->sensitivity);

    //aivoice sdk config
    Parcel_WriteInt32(parcel, config->common->timeout);
    Parcel_WriteInt32(parcel, config->common->memory_alloc_mode);

    SM_LOGI("before parcel size %d", Parcel_IpcDataSize(parcel));

    bool tmp_one = 1;
    int16_t tmp_two = 2;
    int tmp_four = 3;
    int64_t tmp_eight = 4;
    int data_size = Parcel_IpcDataSize(parcel);
    int ones;
    int twos;
    int fours;
    int eights;
    AIVoiceManager_AdjustTo32Multiple(data_size, &ones, &twos, &fours, &eights);
    int i = 0;
    for(; i < eights; i++) {
        Parcel_WriteInt64(parcel, tmp_eight);
    }
    for (i = 0; i < fours; i++) {
        Parcel_WriteInt32(parcel, tmp_four);
    }
    for (i = 0; i < twos; i++) {
        Parcel_WriteInt16(parcel, tmp_two);
    }
    for (i = 0; i < ones; i++) {
        Parcel_WriteBool(parcel, tmp_one);
    }

    SM_LOGI("after parcel size %d", Parcel_IpcDataSize(parcel));

    arg.config_addr = (uint32_t)Parcel_IpcData(parcel);
    arg.config_length = Parcel_IpcDataSize(parcel);

    uint8_t* tmp_data = (uint8_t*)arg.config_addr;
    SM_LOGI("%02x %02x %02x %02x %02x %02x",
               tmp_data[0], tmp_data[1], tmp_data[2], tmp_data[3], tmp_data[4], tmp_data[5]);
    DCache_Clean((uint32_t)arg.config_addr, arg.config_length);

    manager->parcel = parcel;

    arg.usr_data = (uint32_t)manager;

    CLNT_STRUCT clnt = RPC_PrepareCLNT(VOICE_RPC_MODE, VOICE_SYSTEM, 0);
    HRESULT *res = VOICE_RPC_ToAgent_Create_0(&arg, &clnt);
    UNUSED(res);

    SM_LOGI("Agent_Init End");

    clnt = RPC_PrepareCLNT(VOICE_RPC_MODE, VOICE_SYSTEM, 0);
    long rpc_param = 1;
    HRESULT *rpc_res = VOICE_RPC_ToAgent_Start_0(&rpc_param, &clnt);
    UNUSED(rpc_res);

    return manager;
}

int AIVoiceManager_Feed(AIVoiceManager *manager, const void* data, int len)
{
    if (!manager) {
        SM_LOGE("manager is NULL");
        return AUDIO_ERR_INVALID_PARAM;
    }

    int res = manager->mic_ring_buffer->write(manager->mic_ring_buffer, data, len);
    return res;
}

int AIVoiceManager_Destroy(AIVoiceManager *manager)
{
    if (!manager) {
        return AUDIO_ERR_INVALID_PARAM;
    }

    CLNT_STRUCT clnt = RPC_PrepareCLNT(VOICE_RPC_MODE, VOICE_SYSTEM, 0);
    long param = 1;
    HRESULT *res = VOICE_RPC_ToAgent_destroy_0(&param, &clnt);
    UNUSED(res);

    /* The blocking destroy RPC above tears down the DSP agent, so the config
     * parcel is no longer read by the DSP. Free it here if the OnState(type==1)
     * callback hasn't already reclaimed it. */
    if (manager->parcel) {
        Parcel_Destroy(manager->parcel);
        manager->parcel = NULL;
    }

    ring_buffer_destroy(manager->mic_ring_buffer);
    osal_free(manager);

    return AUDIO_OK;
}

int AIVoiceManager_SetCallback(AIVoiceManager *manager, AIVoiceManagerCallback callback, void* usr_data)
{
    if (!manager) {
        return AUDIO_ERR_INVALID_PARAM;
    }

    manager->callback = callback;
    manager->usr_data = usr_data;

    return AUDIO_OK;
}
