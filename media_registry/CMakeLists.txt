# --------------------------------------------------------------
# Public definitions
# --------------------------------------------------------------
set(public_includes)
set(public_definitions)
set(public_libraries)

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

ameba_list_append(private_sources
    media_registry.cpp
)
ameba_list_append(private_includes
    include
)

ameba_list_append_if(CONFIG_MEDIA_CODEC_HAAC private_definitions
    MEDIA_DEMUX_AAC_DELIVER_ADTS_HEADER
)

ameba_list_append_if(CONFIG_MEDIA_DEMUX_AAC private_definitions
    MEDIA_DEMUX_AAC
)

ameba_list_append_if(CONFIG_MEDIA_DEMUX_WAV private_definitions
    MEDIA_DEMUX_WAV
)

ameba_list_append_if(CONFIG_MEDIA_DEMUX_MP3 private_definitions
    MEDIA_DEMUX_MP3
)

ameba_list_append_if(CONFIG_MEDIA_DEMUX_AMR private_definitions
    MEDIA_DEMUX_AMR
)

ameba_list_append_if(CONFIG_MEDIA_DEMUX_OGG private_definitions
    MEDIA_DEMUX_OGG
)

ameba_list_append_if(CONFIG_MEDIA_DEMUX_FLAC private_definitions
    MEDIA_DEMUX_FLAC
)

ameba_list_append_if(CONFIG_MEDIA_DEMUX_MP4 private_definitions
    MEDIA_DEMUX_MP4
)

ameba_list_append_if(CONFIG_MEDIA_CODEC_PCM private_definitions
    MEDIA_CODEC_PCM
)

ameba_list_append_if(CONFIG_MEDIA_CODEC_AAC private_definitions
    MEDIA_CODEC_AAC
)

ameba_list_append_if(CONFIG_MEDIA_CODEC_HAAC private_definitions
    MEDIA_CODEC_HAAC
)

ameba_list_append_if(CONFIG_MEDIA_CODEC_MP3 private_definitions
    MEDIA_CODEC_MP3
)

ameba_list_append_if(CONFIG_MEDIA_CODEC_VORBIS private_definitions
    MEDIA_CODEC_VORBIS
)

ameba_list_append_if(CONFIG_MEDIA_CODEC_OPUS private_definitions
   MEDIA_CODEC_OPUS
)

ameba_list_append_if(CONFIG_MEDIA_CODEC_AMR private_definitions
    MEDIA_CODEC_AMR
)

ameba_list_append_if(CONFIG_MEDIA_CODEC_G711 private_definitions
    MEDIA_CODEC_G711
)

ameba_list_append_if(CONFIG_MEDIA_CODEC_GSM private_definitions
    MEDIA_CODEC_GSM
)

ameba_list_append_if(CONFIG_WLAN private_definitions
    MEDIA_SOURCE_HTTP
)

# --------------------------------------------------------------
# Library
# --------------------------------------------------------------
ameba_add_internal_library(media_registry
    p_SOURCES
        ${private_sources}
    p_INCLUDES
        ${private_includes}
    p_DEFINITIONS
        ${private_definitions}
    p_COMPILE_OPTIONS
        ${private_compile_options}
)