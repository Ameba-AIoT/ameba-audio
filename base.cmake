# --------------------------------------------------------------
# Public definitions
# --------------------------------------------------------------
set(public_includes)
set(public_definitions)
set(public_libraries)

ameba_list_append(public_libraries
    ${c_CMPT_AUDIO_DIR}/libs/${c_SOC_TYPE}/${c_MCU_TYPE}/lib_base.a
)

ameba_global_include(${public_includes})
ameba_global_define(${public_definitions})
ameba_global_library(p_NO_WHOLE_ARCHIVE ${public_libraries})

# --------------------------------------------------------------
# Private definitions
# --------------------------------------------------------------
set(private_sources)
set(private_includes)
set(private_definitions)
set(private_compile_options)

# --------------------------------------------------------------
# Library
# --------------------------------------------------------------
ameba_add_merge_module_library(base ${c_CMPT_AUDIO_DIR}/libs/${c_SOC_TYPE}/${c_MCU_TYPE}
    audio_base_xlib
    audio_base_cutils_src
    audio_base_osal
    audio_base_osal_cxx
    audio_base_audio_utils
)
