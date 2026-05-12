include(../config.cmake)

#------------------------------#
# Component private part, user config begin

ameba_list_append(private_sources
    ahandler.c
    alooper_manager.c
    alooper.c
    amessage.c
)

ameba_list_append(private_includes
    ${c_CMPT_AUDIO_DIR}/foundation/include
)

ameba_list_append(private_compile_options
    -Wno-implicit-fallthrough
)

# Component private part, user config end
#------------------------------#

#WARNING: Select right API based on your component's release/not-release/standalone

###NOTE: For closed-source component, only build before release and libs are packaged into lib/application
ameba_add_external_module_library(audio_foundation ${c_CMPT_AUDIO_DIR}/libs/${c_SOC_TYPE}/${c_MCU_TYPE}
    p_SOURCES
        ${private_sources}
    p_INCLUDES
        ${private_includes}
    p_DEFINITIONS
        ${private_definitions}
    p_COMPILE_OPTIONS
        ${private_compile_options}
)
##########################################################################################
