#ifndef DRC_H
#define DRC_H

//#include "type_def.h"
#ifdef __cplusplus
extern "C" {
#endif
//======================================================
// Function: Version
// Describe:
//  MAJOR:      Major version
//  MINOR:      Minor version
//  REVISION:   Revision version
//  BUILD:      Build version
//======================================================
#define MAJOR               1
#define MINOR               0
#define REVISION            3
#define BUILD               0


#define KNEE_POINT_NUM_MAX  4


typedef struct {
    float threshold_db;     /* threshold (dB) */
    float ratio;            /* ratio */
} knee_point_t;

typedef struct {
    int num_of_knee_point;  /* number of knee point */
    float attack_time;      /* Attack time (sec) */
    float release_time;     /* Release time (sec) */
    float makeup_gain;      /* Makeup Gain (dB) */
    knee_point_t knee_point[KNEE_POINT_NUM_MAX];
} drc_config_t;

typedef struct {
    int fc_lo;              /* crossover freq. low (Hz) */
    int fc_hi;              /* crossover freq. high (Hz) */
} split_freq_t;

typedef struct {
    split_freq_t split_freq;
    drc_config_t drc_band_low;
    drc_config_t drc_band_mid;
    drc_config_t drc_band_high;
} mbdrc_config_t;

//======================================================
// Function: AirHarmony_DrcGetVersion
// Describe: Get version
// input: 
//    none
// return : ex. 0x10000000  ==> version V1.0.0.0
//======================================================
unsigned int AirHarmony_DrcGetVersion(void);

//======================================================
// Function: AirHarmony_MbdrcCreate
// Describe: MB-DRC create & initialize & start
// input: 
//  *drc_config: input MB-DRC config setting
//  sample_rate: sample rate (support: 8K, 16K, 32K, 44.1K, 48K, 88.2K, 96K, 192K)
//  sys_frame_size: frame size (support: min. >= 48samples and must be 48multiple)
//  n_ch: number of channels
// return :
//  *p_mbdrc_mem: MB-DRC malloc memory pointer
//======================================================
void *AirHarmony_MbdrcCreate(mbdrc_config_t *drc_config, float sample_rate, int sys_frame_size, int n_ch);

//======================================================
// Function: AirHarmony_MbdrcRelease
// Describe: MB-DRC close & release memory
// input: 
//  *p_mbdrc_mem: MB-DRC malloc memory pointer
// return :
//  none
//======================================================
void AirHarmony_MbdrcRelease(void *p_mbdrc_mem);

//======================================================
// Function: AirHarmony_MbdrcProcess
// Describe: MB-DRC main process
// input: 
//  *p_mbdrc_mem: MB-DRC malloc memory pointer
//  *in_buf: PCM input buffer
//  *out_buf: PCM output buffer
// return :
//  none
//======================================================
void AirHarmony_MbdrcProcess(void *p_mbdrc_mem, float *in_buf, float *out_buf);

//======================================================
// Function: AirHarmony_MbdrcParaSet
// Describe: MB-DRC Parameter setting in real-time. The API must be after AirHarmony_MbdrcCreate().  
// input: 
//  *p_mem: MB-DRC malloc memory pointer
//  *drc_config: input DRC config setting
// return :
//  none
//======================================================
void AirHarmony_MbdrcParaSet(void *p_mem, mbdrc_config_t *drc_config);

//======================================================
// Function: AirHarmony_DrcCreate
// Describe: DRC create & initialize & start
// input: 
//  *drc_config: input DRC config setting
//  sample_rate: sample rate (support: 8K, 16K, 32K, 44.1K, 48K, 88.2K, 96K, 192K)
//  sys_frame_size: frame size (support: min. >= 48samples and must be 48multiple)
//  n_ch: number of channels
// return :
//  *p_drc_mem: DRC malloc memory pointer
//======================================================
void *AirHarmony_DrcCreate(drc_config_t *drc_config, float sample_rate, int sys_frame_size, int n_ch);

//======================================================
// Function: AirHarmony_DrcRelease
// Describe: DRC close & release memory
// input: 
//  *p_mem: DRC malloc memory pointer
// return :
//  none
//======================================================
void AirHarmony_DrcRelease(void *p_mem);

//======================================================
// Function: AirHarmony_DrcProcess
// Describe: DRC main process
// input: 
//  *p_mem: MB-DRC malloc memory pointer
//  *in_buf: PCM input buffer
//  *out_buf: PCM output buffer
// return :
//  none
//======================================================
void AirHarmony_DrcProcess(void *p_mem, float* input, float* output);

//======================================================
// Function: AirHarmony_DrcParaSet
// Describe: DRC Parameter setting in real-time. The API must be after AirHarmony_DrcCreate(). 
// input: 
//  *p_mem: DRC malloc memory pointer
//  *drc_config: input DRC config setting
// return :
//  none
//======================================================
void AirHarmony_DrcParaSet(void *p_mem, drc_config_t *drc_config);

#ifdef __cplusplus
}
#endif

#endif // DRC_H
