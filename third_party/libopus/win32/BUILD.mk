LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := libopus
LOCAL_CFLAGS := \
    -DNULL=0 \
    -DSOCKLEN_T=socklen_t \
    -DLOCALE_NOT_USED \
    -D_LARGEFILE_SOURCE=1 \
    -D_FILE_OFFSET_BITS=64 \
    -Drestrict= \
    -D__EMX__ \
    -DOPUS_BUILD \
    -DFIXED_POINT \
    -DUSE_ALLOCA \
    -DSIMD_EXTRA_ALLOC_BYTES=16 \
    -DHAVE_LRINT \
    -DHAVE_LRINTF \
    -DENABLE_HARDENING \
    -O2 \
    -fno-math-errno

LOCAL_SRC_FILES := \
    celt/bands.c \
    celt/celt.c \
    celt/celt_encoder.c \
    celt/celt_decoder.c \
    celt/cwrs.c \
    celt/entcode.c \
    celt/entdec.c \
    celt/entenc.c \
    celt/kiss_fft.c \
    celt/laplace.c \
    celt/mathops.c \
    celt/mdct.c \
    celt/modes.c \
    celt/pitch.c \
    celt/celt_lpc.c \
    celt/quant_bands.c \
    celt/rate.c \
    celt/vq.c

LOCAL_SRC_FILES += \
    silk/CNG.c \
    silk/code_signs.c \
    silk/init_decoder.c \
    silk/decode_core.c \
    silk/decode_frame.c \
    silk/decode_parameters.c \
    silk/decode_indices.c \
    silk/decode_pulses.c \
    silk/decoder_set_fs.c \
    silk/dec_API.c \
    silk/enc_API.c \
    silk/encode_indices.c \
    silk/encode_pulses.c \
    silk/gain_quant.c \
    silk/interpolate.c \
    silk/LP_variable_cutoff.c \
    silk/NLSF_decode.c \
    silk/NSQ.c \
    silk/NSQ_del_dec.c \
    silk/PLC.c \
    silk/shell_coder.c \
    silk/tables_gain.c \
    silk/tables_LTP.c \
    silk/tables_NLSF_CB_NB_MB.c \
    silk/tables_NLSF_CB_WB.c \
    silk/tables_other.c \
    silk/tables_pitch_lag.c \
    silk/tables_pulses_per_block.c \
    silk/VAD.c \
    silk/control_audio_bandwidth.c \
    silk/quant_LTP_gains.c \
    silk/VQ_WMat_EC.c \
    silk/HP_variable_cutoff.c \
    silk/NLSF_encode.c \
    silk/NLSF_VQ.c \
    silk/NLSF_unpack.c \
    silk/NLSF_del_dec_quant.c \
    silk/process_NLSFs.c \
    silk/stereo_LR_to_MS.c \
    silk/stereo_MS_to_LR.c \
    silk/check_control_input.c \
    silk/control_SNR.c \
    silk/init_encoder.c \
    silk/control_codec.c \
    silk/A2NLSF.c \
    silk/ana_filt_bank_1.c \
    silk/biquad_alt.c \
    silk/bwexpander_32.c \
    silk/bwexpander.c \
    silk/debug.c \
    silk/decode_pitch.c \
    silk/inner_prod_aligned.c \
    silk/lin2log.c \
    silk/log2lin.c \
    silk/LPC_analysis_filter.c \
    silk/LPC_fit.c \
    silk/LPC_inv_pred_gain.c \
    silk/table_LSF_cos.c \
    silk/NLSF2A.c \
    silk/NLSF_stabilize.c \
    silk/NLSF_VQ_weights_laroia.c \
    silk/pitch_est_tables.c \
    silk/resampler.c \
    silk/resampler_down2_3.c \
    silk/resampler_down2.c \
    silk/resampler_private_AR2.c \
    silk/resampler_private_down_FIR.c \
    silk/resampler_private_IIR_FIR.c \
    silk/resampler_private_up2_HQ.c \
    silk/resampler_rom.c \
    silk/sigm_Q15.c \
    silk/sort.c \
    silk/sum_sqr_shift.c \
    silk/stereo_decode_pred.c \
    silk/stereo_encode_pred.c \
    silk/stereo_find_predictor.c \
    silk/stereo_quant_pred.c

LOCAL_SRC_FILES += \
    silk/fixed/LTP_analysis_filter_FIX.c \
    silk/fixed/LTP_scale_ctrl_FIX.c \
    silk/fixed/corrMatrix_FIX.c \
    silk/fixed/encode_frame_FIX.c \
    silk/fixed/find_LPC_FIX.c \
    silk/fixed/find_LTP_FIX.c \
    silk/fixed/find_pitch_lags_FIX.c \
    silk/fixed/find_pred_coefs_FIX.c \
    silk/fixed/noise_shape_analysis_FIX.c \
    silk/fixed/process_gains_FIX.c \
    silk/fixed/regularize_correlations_FIX.c \
    silk/fixed/residual_energy16_FIX.c \
    silk/fixed/residual_energy_FIX.c \
    silk/fixed/warped_autocorrelation_FIX.c \
    silk/fixed/apply_sine_window_FIX.c \
    silk/fixed/autocorr_FIX.c \
    silk/fixed/burg_modified_FIX.c \
    silk/fixed/k2a_FIX.c \
    silk/fixed/k2a_Q16_FIX.c \
    silk/fixed/pitch_analysis_core_FIX.c \
    silk/fixed/vector_ops_FIX.c \
    silk/fixed/schur64_FIX.c \
    silk/fixed/schur_FIX.c

LOCAL_SRC_FILES += \
    src/mapping_matrix.c \
    src/opus.c \
    src/opus_decoder.c \
    src/opus_encoder.c \
    src/opus_multistream.c \
    src/opus_multistream_encoder.c \
    src/opus_multistream_decoder.c \
    src/opus_projection_encoder.c \
    src/opus_projection_decoder.c \
    src/repacketizer.c

LOCAL_SRC_FILES += \
    src/analysis.c \
    src/mlp.c \
    src/mlp_data.c

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/include \
    $(LOCAL_PATH)/src \
    $(LOCAL_PATH)/silk \
    $(LOCAL_PATH)/celt \
    $(LOCAL_PATH)/silk/fixed

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include

ifeq ($(TARGET_ARCH),arm)

LOCAL_SRC_FILES += \
    celt/arm/armcpu.c \
    celt/arm/arm_celt_map.c \
    celt/arm/celt_pitch_xcorr_arm_gnu.s \
    celt/arm/armopts_gnu.s

LOCAL_CFLAGS += \
    -DOPUS_ARM_ASM \
    -DOPUS_ARM_INLINE_ASM \
    -DOPUS_ARM_MAY_HAVE_EDSP \
    -DOPUS_ARM_INLINE_EDSP \
    -DOPUS_ARM_MAY_HAVE_MEDIA \
    -DOPUS_ARM_INLINE_MEDIA \
    -DOPUS_ARM_MAY_HAVE_NEON \
    -DOPUS_HAVE_RTCD

LOCAL_SRC_FILES += \
    celt/arm/celt_neon_intr.c \
    celt/arm/pitch_neon_intr.c \
    silk/arm/arm_silk_map.c \
    silk/arm/biquad_alt_neon_intr.c \
    silk/arm/LPC_inv_pred_gain_neon_intr.c \
    silk/arm/NSQ_del_dec_neon_intr.c \
    silk/arm/NSQ_neon.c \
    silk/fixed/arm/warped_autocorrelation_FIX_neon_intr.c

LOCAL_CFLAGS += \
    -DOPUS_ARM_MAY_HAVE_NEON \
    -DOPUS_ARM_MAY_HAVE_NEON_INTR \
    -DOPUS_ARM_PRESUME_NEON \
    -DOPUS_ARM_INLINE_NEON

else ifeq ($(TARGET_ARCH),x86)

endif

include $(BUILD_SHARED_LIBRARY)
