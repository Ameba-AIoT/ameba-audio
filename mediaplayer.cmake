# --------------------------------------------------------------
# Public definitions
# --------------------------------------------------------------
set(public_includes)
set(public_definitions)
set(public_libraries)

ameba_list_append(public_libraries
    ${c_CMPT_AUDIO_DIR}/libs/${c_SOC_TYPE}/${c_MCU_TYPE}/lib_playback.a
)

ameba_global_include(${public_includes})
ameba_global_define(${public_definitions})
ameba_global_library(${public_libraries})

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
ameba_add_merge_module_library(playback ${c_CMPT_AUDIO_DIR}/libs/${c_SOC_TYPE}/${c_MCU_TYPE}
    audio_media_common
    audio_media_codec_omx
    audio_media_codec_common_amrnb
    audio_media_codec_common_amrwb
    audio_media_codec_common_pvmp3
    audio_media_core_standard
    audio_media_demux
    audio_media_libmedia_standard
    audio_media_source
    audio_media_utils
)