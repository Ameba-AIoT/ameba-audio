#ifdef __cplusplus
extern "C" {
#endif

#define Q31_ONE         ((1U << 31) - 1)
#define M_PI 3.14159265358979323846


//==================================== Parametric Eq Usage Start ==============================================================

typedef struct
{
    int     fft_size    ;
} spectrum_config_t;



//===============================================================================================================================
// Function: AirHarmony_SpectrumCreate
// Describe: Spectrum Analyzer create & initialize & start
// input:
//  *spectrum_config:  spectrum_config input setting
//  sample_rate: sample rate
//  sys_frame_size: frame size
//  n_ch: the channel number of Spectrum Analyzer, max is 10
// return :
//  *p_Spectrum_mem: Spectrum Analyzer malloc memory pointer
//================================================================================================================================
void *AirHarmony_SpectrumCreate(spectrum_config_t *spectrum_config, float sample_rate, int sys_frame_size,int n_ch);

//===============================================================================================================================
// Function: AirHarmony_SpectrumProcess
// Describe: Spectrum Analyzer main process
// input:
//  *p_Spectrum_mem: Spectrum Analyzer malloc memory pointer
//  *input  : PCM input buffer
// return   : User can call AirHarmony_SpectrumGetData() function to get the output data of fft
//  none
//===============================================================================================================================
void AirHarmony_SpectrumProcess(void *p_Spectrum_mem,float* input);

//===============================================================================================================================
// Function: AirHarmony_SpectrumGetData
// Describe: provide the user to get the data of Spectrum Analyzer
// input:
//  *pParametricEq_Mem: Spectrum Analyzer memory pointer the data of Spectrum Analyzer
// output:
//   user set the output memory pointer,and get fft ouput,the Real and imaginary data are interleaved and real data goes first
//   the output : fft_output[0]-> X[0].re  , fft_output[1]-> X[0].im , fft_output[2]-> X[1].re  , fft_output[3]-> X[1].im .....
//                fft_output[fft_size]-> X[fft_size/2].re  , fft_output[fft_size+1]-> X[fft_size/2].im
// return :
//  none
//===============================================================================================================================
void AirHarmony_SpectrumGetData(void *p_Spectrum_mem,float* fft_output);

//===============================================================================================================================
// Function: AirHarmony_SpectrumRelease
// Describe: Spectrum Analyzer close & release memory
// input:
//  *pSpectrum_Mem: Spectrum Analyzer memory pointer
// return :
//  none
//===============================================================================================================================
void AirHarmony_SpectrumRelease(void *pSpectrum_Mem);

//===============================================================================================================================
// Function: AirHarmony_SpectrumGetVersion
// Describe: Get version
// input:
//    none
// return : ex. 0x10000000  ==> version V1.0.0.0
//===============================================================================================================================
unsigned int AirHarmony_SpectrumGetVersion(void);

#ifdef __cplusplus
}
#endif
