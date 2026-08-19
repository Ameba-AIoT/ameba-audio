##########################################################################################
## * This part defines public part of the component
## * Public part will be used as global build configures for all component

set(public_includes)                #public include directories, NOTE: relative path is OK
set(public_definitions)             #public definitions
set(public_libraries)               #public libraries(files), NOTE: linked with whole-archive options

#----------------------------------------#
# Component public part, user config begin

ameba_list_append_if(CONFIG_AUDIO_FWK public_libraries
    ${CMAKE_CURRENT_SOURCE_DIR}/${c_SOC_TYPE}/${c_MCU_TYPE}/lib_airharmony_drc.a
    ${CMAKE_CURRENT_SOURCE_DIR}/${c_SOC_TYPE}/${c_MCU_TYPE}/lib_audio_eq.a
)

# lib_audio_spectrum.a's FFT depends on ARM CMSIS-DSP (arm_rfft_fast_f32 /
# arm_rfft_fast_init_f32), which do not exist on the RISC-V KR4 core -> linking
# it there fails with undefined references. Skip it on KR4; the spectrum
# algorithm is disabled on that core (CONFIG_AUDIO_SPECTRUM_EFFECT is left
# undefined, see audio_effect/sw_algorithm/wrapper). EQ/DRC are unaffected.
if(NOT c_MCU_TYPE STREQUAL "kr4")
    ameba_list_append_if(CONFIG_AUDIO_FWK public_libraries
        ${CMAKE_CURRENT_SOURCE_DIR}/${c_SOC_TYPE}/${c_MCU_TYPE}/lib_audio_spectrum.a
    )
endif()

# Component public part, user config end
#----------------------------------------#

#WARNING: Fixed section, DO NOT change!
ameba_global_include(${public_includes})
ameba_global_define(${public_definitions})
ameba_global_library(${public_libraries}) #default: whole-archived

##########################################################################################

