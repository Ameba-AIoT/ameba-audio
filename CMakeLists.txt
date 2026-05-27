# --------------------------------------------------------------
# Base Utilities
# --------------------------------------------------------------
ameba_add_subdirectory_if_exist(base)
ameba_add_subdirectory_if_exist(foundation)
include(base.cmake)
include(foundation.cmake)

# --------------------------------------------------------------
# Audio Framework (CONFIG_AUDIO_FWK)
# --------------------------------------------------------------
if(CONFIG_AUDIO_FWK)
    ameba_add_subdirectory_if_exist(audio_hal)
    ameba_add_subdirectory_if_exist(audio_driver)
    ameba_add_subdirectory_if_exist(audio_effect)
    ameba_add_subdirectory_if_exist(audio_control)
    ameba_add_subdirectory_if_exist(audio_stream)
    ameba_add_subdirectory_if_exist(audio_policy)
    ameba_add_subdirectory_if_exist(audio_algorithm)

    include(configs/audio_configs.cmake)
    include(audio_mixer.cmake)
    include(audio_passthrough.cmake)

    if (CONFIG_AMEBASMART OR CONFIG_AMEBALITE OR CONFIG_AMEBADPLUS)
        ameba_add_subdirectory_if_exist(audio_prebuilts)
    endif ()

endif()

# --------------------------------------------------------------
# Media Player (CONFIG_MEDIA_PLAYER)
# --------------------------------------------------------------
if(CONFIG_MEDIA_PLAYER)
    ameba_add_subdirectory_if_exist(media_source)
    ameba_add_subdirectory_if_exist(media_demux)
    ameba_add_subdirectory_if_exist(media_codec)
    ameba_add_subdirectory_if_exist(media_core)
    ameba_add_subdirectory_if_exist(media_common)
    ameba_add_subdirectory_if_exist(media_utils)
    ameba_add_subdirectory_if_exist(media_libmedia)
    ameba_add_subdirectory_if_exist(media_registry)

    include(mediaplayer.cmake)
endif()

# --------------------------------------------------------------
# USB Audio
# --------------------------------------------------------------
if(CONFIG_AUDIO_DEVICE_USB)
    ameba_add_subdirectory_if_exist(usb_audio)
endif()

# --------------------------------------------------------------
# Audio Commands
# --------------------------------------------------------------
if(CONFIG_SUPPORT_AUDIO_CMD)
    ameba_add_subdirectory_if_exist(cmds)
endif()

# --------------------------------------------------------------
# Third Party
# --------------------------------------------------------------
ameba_add_subdirectory_if_exist(third_party)

# --------------------------------------------------------------
# Audio Example
# --------------------------------------------------------------
ameba_submodule_register(audio p_EXAMPLE_DIRS
    examples/audio_manager
    examples/audio_time
    examples/audio_hal_render
    examples/audio_hal_capture
    examples/audio_converter
    examples/speexdsp
    examples/voice
)
