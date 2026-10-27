# --------------------------------------------------------------
# Public definitions
# --------------------------------------------------------------
set(public_includes)
set(public_definitions)
set(public_libraries)

ameba_list_append_if(CONFIG_AUDIO_FWK public_libraries
    ${c_CMPT_AUDIO_DIR}/libs/${c_SOC_TYPE}/${c_MCU_TYPE}/lib_audio_foundation.a
)

ameba_global_include(${public_includes})
ameba_global_define(${public_definitions})
ameba_global_library(p_NO_WHOLE_ARCHIVE ${public_libraries})
