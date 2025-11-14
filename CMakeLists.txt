if(CONFIG_MEDIA_PLAYER)
    ameba_add_subdirectory_if_exist(media_utils)
    ameba_add_subdirectory_if_exist(media_common)
    ameba_add_subdirectory_if_exist(media_source)
    ameba_add_subdirectory_if_exist(media_demux)

    ameba_add_subdirectory_if_exist(media_codec)
    ameba_add_subdirectory_if_exist(media_core)
    ameba_add_subdirectory_if_exist(media_libmedia)

    include(configs/media_configs.cmake)
    include(play_back.cmake)
endif()

if(CONFIG_AUDIO_FWK)
    ameba_add_subdirectory_if_exist(audio_algorithm)
    ameba_add_subdirectory_if_exist(audio_effect)
    ameba_add_subdirectory_if_exist(audio_control)
    ameba_add_subdirectory_if_exist(audio_stream)
    ameba_add_subdirectory_if_exist(audio_policy)
    include(configs/audio_configs.cmake)
    include(audio_mixer.cmake)
    include(audio_passthrough.cmake)
endif()

#ameba_add_subdirectory_if(CONFIG_MEDIA_PLAYER media_test)
ameba_add_subdirectory_if(CONFIG_AUDIO_FWK audio_cmds)
ameba_add_subdirectory_if(CONFIG_AUDIO_FWK audio_hal)
ameba_add_subdirectory_if(CONFIG_AUDIO_FWK audio_driver)

ameba_add_subdirectory(third_party)

if(CONFIG_AUDIO_FWK)
    ameba_add_subdirectory_if_exist(foundation)
    ameba_add_subdirectory_if_exist(base)
    include(base.cmake)
endif()

if(CONFIG_AUDIO_DEVICE_USB)
    ameba_add_subdirectory_if_exist(usb_audio)
endif()

ameba_submodule_register(audio
p_EXAMPLE_DIRS
    examples/audio_track
    examples/audio_record
    examples/audio_manager
    examples/audio_time
    examples/audio_hal_render
    examples/audio_hal_capture
    examples/audio_converter
    examples/mydata_source_player
    examples/pc_recorder
    examples/player
    examples/speexdsp
    examples/voice
)
