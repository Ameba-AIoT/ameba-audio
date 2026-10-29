#ifdef __cplusplus
extern "C" {
#endif

#define BIQUAD_NUN 15             // BU spec , the max biquad number is 15

//===================================== Parametric Eq and Biquad Eq common Usage Start ====================
typedef enum {
    LOWPASS,
    HIGHPASS,
    BANDPASS_TYPE,  // constant 0 dB peak gain
    PEAKING,
    NOTCH,
    LOW_SHELF,
    HIGH_SHELF,
} FilterType;


typedef struct
{
    int numStages;     /**< number of 2nd order stages in the filter.  Overall order is 2*numStages. */
    float *pState;       /**< Points to the array of state coefficients.  The array is of length 4*numStages. */
    float *pCoeffs;      /**< Points to the array of coefficients.  The array is of length 5*numStages. */
} Biquad_t;

typedef struct
{
    float b0;
    float b1;
    float b2;
    float a1;
    float a2;
} BiquadCoeff_t;
//===================================== Parametric Eq and Biquad Eq common Usage End=======================


//===================================== Parametric Eq Usage Start ========================================

typedef struct
{ 
    int enable ; 
    int filter_type;
    float fc;
    float q ;
    float gain;
} parametric_eq_para_t;

typedef struct
{ 
    int     limiter_enable ;
    float   global_gain_dB;
    float   global_gain_linear;
    parametric_eq_para_t parametric_eq_para[BIQUAD_NUN] ;
} parametric_eq_config_t;

//===================================== Parametric Eq Usage End==========================================


//===================================== Biquad Eq Usage Start ===========================================
typedef struct
{
  int            limiter_enable ;
  float          global_gain_dB ;
  float          global_gain_linear ;
  int            biquad_num ;
  Biquad_t       biquad;
  float          biquad_State[BIQUAD_NUN*4];
  BiquadCoeff_t  biquadCoeff[BIQUAD_NUN];
  void           *p_biq_iir_handle;;
}biquad_eq_t;


typedef struct
{
  int            limiter_enable ;
  float          global_gain_dB ;
  int            biquad_num ;
  BiquadCoeff_t  biquadCoeff[BIQUAD_NUN];
}biquad_eq_config_t;
//===================================== Biquad Eq Usage End================================================

//====================================================================================
// Function: ADT_ParametricEqCreate
// Describe: Parametric Equalizer create & initialize & start
// input:
//  *parametric_eq_config: input Parametric eq config setting
//  sample_rate: sample rate (support: 8K, 16K, 32K, 44.1K, 48K, 88.2K, 96K, 192K)
//  sys_frame_size: frame size
//  n_ch          : the number of multi-channel Parametric Equalizer
// return :
//  *p_ParametricEq_mem: Parametric Equalizer malloc memory pointer
//====================================================================================
//void *ADT_ParametricEqCreate(parametric_eq_t *parametric_eq_config, float sample_rate, int sys_frame_size);
void *ADT_ParametricEqCreate(parametric_eq_config_t *parametric_eq_config, float sample_rate, int sys_frame_size,int n_ch);

//====================================================================================
// Function: ADT_BiquadEqCreate
// Describe: Biquad Equalizer create & initialize & start
// input:
//  *biquad_eq_config: input biquad eq coeff and config setting
//  sys_frame_size: frame size
//  n_ch          : the number of multi-channel Biquad Equalizer
//  return :
//  *p_BiquadEq_mem: Biquad Equalizer malloc memory pointer
//====================================================================================
//void *ADT_BiquadEqCreate(biquad_eq_config_t *biquad_eq_config,int sys_frame_size);
void *ADT_BiquadEqCreate(biquad_eq_config_t *biquad_eq_config,int sys_frame_size,int n_ch);
//======================================================
// Function: ADT_ParametricEqRelease
// Describe: Parametric Equalizer close & release memory
// input:
//  *pParametricEq_Mem: Parametric Equalizer memory pointer
// return :
//  none
//======================================================
void ADT_ParametricEqRelease(void *pParametricEq_Mem);

//======================================================
// Function: ADT_BiquadEqRelease
// Describe: Biquad Equalizer close & release memory
// input:
//  *pBiquadEq_Mem: Biquad Equalizer memory pointer
// return :
//  none
//======================================================
void ADT_BiquadEqRelease(void *pBiquadEq_Mem);

//======================================================
// Function: ADT_ParametricEqProcess
// Describe: Parametric Equalizer main process
// input:
//  *p_parametric_eq_mem: Parametric Equalizer malloc memory pointer
//  *in_buf: PCM input buffer
//  *out_buf: PCM output buffer
//  FrameSize: frame size
// return :
//  none
//======================================================
//void ADT_ParametricEqProcess(parametric_eq_t *p_parametric_eq_mem,float* input, float* output,int FrameSize);
void ADT_ParametricEqProcess(void *p_parametric_eq_mem,float* input, float* output,int FrameSize);
//======================================================
// Function: ADT_BiquadEqProcess
// Describe: Biquad Equalizer main process
// input:
//  *p_biquad_eq_mem: Biquad Equalizer malloc memory pointer
//  *in_buf: PCM input buffer
//  *out_buf: PCM output buffer
//  FrameSize: frame size
// return :
//  none
//======================================================
//void ADT_BiquadEqProcess(biquad_eq_t *p_biquad_eq_mem,float* input, float* output,int FrameSize) ;
void ADT_BiquadEqProcess(void *p_biquad_eq_mem,float* input, float* output,int FrameSize);
//======================================================
// Function: ADT_ParametricEqParaSet
// Describe: Parametric Equalizer Parameter setting for Parametric Equalizer switch in real-time
// input:
//  *p_biquad_eq_mem: Biquad Equalizer malloc memory pointer
//  *in_buf: PCM input buffer
//  *out_buf: PCM output buffer
//  FrameSize: frame size
// return :
//  none
//======================================================
//void ADT_ParametricEqParaSet(parametric_eq_t* p_parametric_eq_mem ,parametric_eq_t * parametric_eq_config,float sample_rate);
void ADT_ParametricEqParaSet(void* p_parametric_eq_mem ,parametric_eq_config_t * parametric_eq_config,float sample_rate);
//======================================================
// Function: ADT_EqGetVersion
// Describe: Get version
// input:
//    none
// return : ex. 0x10000000  ==> version V1.0.0.0
//======================================================
unsigned int ADT_EqGetVersion(void);

#ifdef __cplusplus
}
#endif
