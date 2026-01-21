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

/**
 * @addtogroup Audio
 * @{
 *
 * @brief Declares APIs for audio framework.
 *
 *
 * @since 1.0
 * @version 1.0
 */

/**
 * @file audio_control.h
 *
 * @brief Provides APIs of the audio control, such as volume, mute, input devices.
 *
 *
 * @since 1.0
 * @version 1.0
 */

#ifndef AMEBA_AUDIO_INTERFACES_AUDIO_AUDIO_CONTROL_H
#define AMEBA_AUDIO_INTERFACES_AUDIO_AUDIO_CONTROL_H

#include <stdint.h>
#include "audio/audio_type.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Defines all the audio playback devices.
 *
 * @since 1.0
 * @version 1.0
 */
enum {
	/* [amebalite] support AMIC1-AMIC3, DMIC1-DMIC4
	 * [amebasmart] support AMIC1-AMIC5, DMIC1-DMIC8
	 */
	AUDIO_AMIC1         = 0,
	AUDIO_AMIC2         = 1,
	AUDIO_AMIC3         = 2,
	AUDIO_AMIC4         = 3,
	AUDIO_AMIC5         = 4,
	AUDIO_DMIC1         = 5,
	AUDIO_DMIC2         = 6,
	AUDIO_DMIC3         = 7,
	AUDIO_DMIC4         = 8,
	AUDIO_DMIC5         = 9,
	AUDIO_DMIC6         = 10,
	AUDIO_DMIC7         = 11,
	AUDIO_DMIC8         = 12,
	AUDIO_MIC_MAX_NUM   = 13,
};

/**
 * @brief Defines the audio capture main usages.
 *
 * @since 1.0
 * @version 1.0
 */
enum {
	AUDIO_CAPTURE_USAGE_AMIC         = 0,
	AUDIO_CAPTURE_USAGE_DMIC         = 1,
	AUDIO_CAPTURE_USAGE_DMIC_REF_AMIC = 2,
	AUDIO_CAPTURE_USAGE_LINE_IN      = 3,
	AUDIO_CAPTURE_USAGE_MAX_NUM      = 4,
};

/**
 * @brief Defines all the audio mic bst gain values.
 *
 * @since 1.0
 * @version 1.0
 */
enum {
	AUDIO_MICBST_GAIN_0DB          = 0,
	AUDIO_MICBST_GAIN_5DB          = 1,
	AUDIO_MICBST_GAIN_10DB         = 2,
	AUDIO_MICBST_GAIN_15DB         = 3,
	AUDIO_MICBST_GAIN_20DB         = 4,
	AUDIO_MICBST_GAIN_25DB         = 5,
	AUDIO_MICBST_GAIN_30DB         = 6,
	AUDIO_MICBST_GAIN_35DB         = 7,
	AUDIO_MICBST_GAIN_40DB         = 8,
	AUDIO_MICBST_GAIN_MAX_NUM      = 9,
};

/**
 * @brief Defines all the audio PLL clock tune options.
 *
 * @since 1.0
 * @version 1.0
 */
enum {
	AUDIO_PLL_AUTO    = 0,
	AUDIO_PLL_FASTER  = 1,
	AUDIO_PLL_SLOWER  = 2,
};

typedef struct {
	uint32_t H0_Q;
	uint32_t B1_Q;
	uint32_t B2_Q;
	uint32_t A1_Q;
	uint32_t A2_Q;
} EqFilterCoef;

/**
 * @brief Set Hardware Volume of audio dac.
 *
 * @param left_volume volume of left channel, 0.0-1.0.
 * @param right_volume volume of right channel, 0.0-1.0.
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | param not supported.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioControl_SetHardwareVolume(float left_volume, float right_volume);

/**
 * @brief Get Hardware Volume of audio dac.
 *
 * @param left_volume pointer of volume of left channel to get, 0.0-1.0.
 * @param right_volume pointer of volume of right channel to get, 0.0-1.0.
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | HAL control instance not created.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioControl_GetHardwareVolume(float *left_volume, float *right_volume);

/**
 * @brief Set Amplifier En Pin.
 *
 * @param amp_pin the pin of the amplifier en, you can get your pin value from:
 * fwlib/include/ameba_pinmux.h, for example, if your pin is _PB_7, it's value is "#define _PB_7 (0x27)"
 * which is 39 of uint32_t.So the amp_pin here should set 39.
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | param not supported.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioControl_SetAmplifierEnPin(uint32_t amp_pin);

/**
 * @brief Set Amplifier En Pin.
 *
 * @return Returns amp_pin the pin of the amplifier en, value is from:
 *         fwlib/include/ameba_pinmux.h, for example, if your pin is _PB_7, it's value is "#define _PB_7 (0x27)"
 *         which is 39 of uint32_t.
 *         If the value < 0, means get fail.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioControl_GetAmplifierEnPin(void);

/**
 * @brief Set Amplifier Mute.
 * Note: this interface may take some time, see component/audio/audio_hal/ameba_audio_stream_control:
 *       ameba_audio_ctl_set_amp_state.
 *
 * @param mute true means mute amplifier, false means unmute amplifier.
 * @return  Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | HAL control instance not created.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioControl_SetAmplifierMute(bool mute);

/**
 * @brief Get Amplifier Mute State.
 *
 * @return  Returns the state of amplifier. true means amplifier is muted, false means amplifier is unmuted.
 * @since 1.0
 * @version 1.0
 */
bool AudioControl_GetAmplifierMute(void);

/**
 * @brief Set Audio Codec Mute.
 *
 * @param mute true means mute dac, false means unmute dac.
 * @return  Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | HAL control instance not created.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioControl_SetPlaybackMute(bool mute);

/**
 * @brief Get Audio Codec Mute State.
 *
 * @return  Returns the state of dac. true means dac is muted, false means dac is unmuted.
 * @since 1.0
 * @version 1.0
 */
bool AudioControl_GetPlaybackMute(void);

/**
 * @brief Set Playback Device. Please set it before create AudioTrack.
 *
 * @param device_category the device of playback, maybe DEVICE_OUT_SPEAKER or DEVICE_OUT_HEADPHONE.
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | param not supported.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioControl_SetPlaybackDevice(uint32_t device_category);

/**
 * @brief Get Playback Device.
 *
 * @return Returns the device of playback, maybe DEVICE_OUT_SPEAKER or DEVICE_OUT_HEADPHONE.
 *         If the value < 0, means get fail.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioControl_GetPlaybackDevice(void);

/**
 * @brief Set Capture Mic type for channel.
 *
 * @param channel_num the channel number to set mic type.
 * @param mic_category the mic type, can be AUDIO_AMIC1...AUDIO_DMIC8.
 *       [amebalite] support AMIC1-AMIC3, DMIC1-DMIC4
 *       [amebasmart] support AMIC1-AMIC5, DMIC1-DMIC8
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | param not supported.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioControl_SetChannelMicCategory(uint32_t channel_num, uint32_t mic_category);

/**
 * @brief Get Mic Category.
 * @param channel_num the channel number to get mic type.
 * @return Returns the mic type, can be AUDIO_AMIC1...AUDIO_DMIC8.
 *         [amebalite] support AMIC1-AMIC3, DMIC1-DMIC4
 *         [amebasmart] support AMIC1-AMIC5, DMIC1-DMIC8
 *         If the value < 0, means get fail.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioControl_GetChannelMicCategory(uint32_t channel_num);

/**
 * @brief Set Capture volume for channel.
 *
 * @param channel_num the channel number to set volume.
 * @param volume the value of volume, can be 0x00-0xaf.
  *          This parameter can be -17.625dB~48dB in 0.375dB step
  *          0x00 -17.625dB
  *          0xaf 48dB
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | param not supported.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioControl_SetCaptureChannelVolume(uint32_t channel_num, uint32_t volume);

/**
 * @brief Get Capture volume for channel.
 *
 * @param channel_num the channel number to get volume.
 * @return the value of volume, can be 0x00-0xaf.
  *          This parameter can be -17.625dB~48dB in 0.375dB step
  *          0x00 -17.625dB
  *          0xaf 48dB
 * @since 1.0
 * @version 1.0
 */
int32_t AudioControl_GetCaptureChannelVolume(uint32_t channel_num);

/**
 * @brief Set Capture volume for channel.
 *
 * @param channels the total channels number to set volume, also the channels to capture.
 * @param volume the value of volume, can be 0x00-0xaf.
  *          This parameter can be -17.625dB~48dB in 0.375dB step
  *          0x00 -17.625dB
  *          0xaf 48dB
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | param not supported.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioControl_SetCaptureVolume(uint32_t channels, uint32_t volume);

/**
 * @brief Set Micbst Gain.
 *
 * @param mic_category the mic type, can be AUDIO_AMIC1...AUDIO_DMIC8.
 *       [amebalite] support AMIC1-AMIC3, DMIC1-DMIC4
 *       [amebasmart] support AMIC1-AMIC5, DMIC1-DMIC8
 * @param gain can be AUDIO_MICBST_GAIN_0DB-AUDIO_MICBST_GAIN_40DB
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | param not supported.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioControl_SetMicBstGain(uint32_t mic_category, uint32_t gain);

/**
 * @brief Get MicBst gain of microphone.
 *
 * @param mic_category the mic type, can be AUDIO_AMIC1...AUDIO_DMIC8.
 *       [amebalite] support AMIC1-AMIC3, DMIC1-DMIC4
 *       [amebasmart] support AMIC1-AMIC5, DMIC1-DMIC8
 * @return gain of mic_category, can be AUDIO_MICBST_GAIN_0DB-AUDIO_MICBST_GAIN_40DB
 * @since 1.0
 * @version 1.0
 */
int32_t AudioControl_GetMicBstGain(uint32_t mic_category);

/**
 * @brief Set adc's hfp fc.
 *
 * @param channel_num the channel to adjust hfp's fc, ranges: 0~8.
 * @param fc : select high pass filter fc
 *	           This parameter can be one of the following values:
 *	           @arg 0：5e10^-3 Fs
 *	           @arg 1：2.5e10^-3 Fs
 *	           @arg 2：1.125e10^-3 Fs
 *	           @arg 3: 6.25e10^-4 Fs
 *	           @arg 4: 3.125e10^-4 Fs
 *	           @arg 5: 1.5625e10^-4 Fs
 *	           @arg 6: 7.8125e10^-5 Fs
 *	           @arg 7: 3.90625e10^-5 Fs
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | param not supported.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioControl_SetCaptureHpfFc(uint32_t channel_num, uint32_t fc);

/**
 * @brief Get adc's hfp fc.
 *
 * @param channel_num the channel to adjust hfp's fc, ranges 0 ~ (total_channel-1).
 * @return Returns a value listed below: \n
 * 0：5e10^-3 Fs
 * 1：2.5e10^-3 Fs
 * 2：1.125e10^-3 Fs
 * 3: 6.25e10^-4 Fs
 * 4: 3.125e10^-4 Fs
 * 5: 1.5625e10^-4 Fs
 * 6: 7.8125e10^-5 Fs
 * 7: 3.90625e10^-5 Fs
 * @since 1.0
 * @version 1.0
 */
int32_t AudioControl_GetCaptureHpfFc(uint32_t channel_num);

/**
 * @brief Set capture eq enable.
 *
 * @param channel_num channel to enable eq, ranges 0 ~ (total_channel-1).
 * @param state true means enable eq, false means disable eq for this channel.
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | param not supported.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioControl_SetCaptureEqEnable(uint32_t channel_num, bool state);

/**
 * @brief Set capture eq coef.
 *
 * @param channel_num channel to set eq filter, ranges 0 ~ (total_channel-1).
 * @param band_sel band to set eq filter, ranges 0 ~ 4.
 * @param eq_filter_coef eq filter coefficient.
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | param not supported.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioControl_SetCaptureEqFilter(uint32_t channel_num, uint32_t band_sel, EqFilterCoef *eq_filter_coef);

/**
 * @brief Set capture eq band state.
 *
 * @param channel_num channel to set eq filter, ranges 0 ~ (total_channel-1).
 * @param band_sel band to set eq filter, ranges 0 ~ 4.
 * @param state true to enable band, false to disable band.
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | param not supported.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioControl_SetCaptureEqBand(uint32_t channel_num, uint32_t band_sel, bool state);

/**
 * @brief Set mic usage.
 *
 * @param mic_usage AUDIO_CAPTURE_USAGE_AMIC, or AUDIO_CAPTURE_USAGE_DMIC, if user's
 *        microphone data is recorded from amic, then set AUDIO_CAPTURE_USAGE_AMIC, if
 *        user's microphone data is recorded from dmic, then set AUDIO_CAPTURE_USAGE_DMIC.
 *        The reference data is always recorded from amic, even if the usage is
 *        AUDIO_CAPTURE_USAGE_DMIC, the default setting is AUDIO_CAPTURE_USAGE_AMIC.
 * @return Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | param not supported.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioControl_SetMicUsage(uint32_t mic_usage);

/**
 * @brief Get mic usage.
 *
 * @param Returns AUDIO_CAPTURE_USAGE_AMIC, or AUDIO_CAPTURE_USAGE_DMIC, if user's
 *        microphone data is recorded from amic, then set AUDIO_CAPTURE_USAGE_AMIC, if
 *        user's microphone data is recorded from dmic, then set AUDIO_CAPTURE_USAGE_DMIC.
 *        If mic data is from dmic, the reference data is recorded from amic, then set
 *        AUDIO_CAPTURE_USAGE_DMIC_REF_AMIC, the default setting is AUDIO_CAPTURE_USAGE_AMIC.
 *        if the return value<0, then get fail.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioControl_GetMicUsage(void);

/**
 * @brief Adjust PLL clk .
 *
 * @param rate sample rate of current stream
 * @param ppm ~1.55ppm per FOF step
 * @param action can be AUDIO_PLL_AUTO AUDIO_PLL_FASTER AUDIO_PLL_SLOWER
 * @return Returns the ppm adjusted.
 * @since 1.0
 * @version 1.0
 */
float AudioControl_AdjustPLLClock(uint32_t rate, float ppm, uint32_t action);

/**
 * @brief Set Audio Record Mute.
 *
 * @param channel the channel to mute or unmute record.
 * @param mute true means mute record, false means unmute record.
 * @return  Returns a value listed below: \n
 * int32_t | Description
 * ----------------------| -----------------------
 * AUDIO_OK | the operation is successful.
 * AUDIO_ERR_INVALID_OPERATION | HAL control instance not created.
 * @since 1.0
 * @version 1.0
 */
int32_t AudioControl_SetRecordMute(uint32_t channel, bool mute);

/**
 * @brief Get Audio Record Mute State.
 *
 * @param channel the channel to mute or unmute state.
 * @return  Returns the state of record. true means record is muted, false means record is unmuted.
 * @since 1.0
 * @version 1.0
 */
bool AudioControl_GetRecordMute(uint32_t channel);

#ifdef __cplusplus
}
#endif


#endif
