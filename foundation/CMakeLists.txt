##########################################################################################
## * This part defines public part of the component
## * Public part will be used as global build configures for all component

set(public_includes)                #public include directories, NOTE: relative path is OK
set(public_definitions)             #public definitions
set(public_libraries)               #public libraries(files), NOTE: linked with whole-archive options

#----------------------------------------#
# Component public part, user config begin

# You may use if-else condition to set or update predefined variable above

## Differentiated configuration based on SoC types
if (CONFIG_MEDIA_PLAYER)
    ameba_list_append(public_libraries
        ${c_CMPT_AUDIO_DIR}/libs/${c_SOC_TYPE}/${c_MCU_TYPE}/lib_audio_foundation.a
    )
endif()

list(REMOVE_DUPLICATES public_libraries)
# Component public part, user config end
#----------------------------------------#

#WARNING: Fixed section, DO NOT change!
ameba_global_include(${public_includes})
ameba_global_define(${public_definitions})
ameba_global_library(${public_libraries}) #NOTE: no_whole-archived

##########################################################################################
## * This part defines private part of the component
## * Private part is used to build target of current component
## * NOTE: The build API guarantees the global build configures(mentioned above)
## *       applied to the target automatically. So if any configure was already added
## *       to public above, it's unnecessary to add again below.

#NOTE: User defined section, add your private build configures here
# You may use if-else condition to set these predefined variable
# They are only for ameba_add_internal_library/ameba_add_external_app_library/ameba_add_external_soc_library
set(private_sources)                 #private source files, NOTE: relative path is OK
set(private_includes)                #private include directories, NOTE: relative path is OK
set(private_definitions)             #private definitions
set(private_compile_options)         #private compile_options

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
    ${c_FREERTOS_DIR}/include
    ${c_FREERTOS_DIR}/portable/GCC/ARM_CA7
    ${c_POSIX_DIR}/include
    ${c_POSIX_DIR}/FreeRTOS-Plus-POSIX/include
    ${c_POSIX_DIR}/FreeRTOS-Plus-POSIX/include/portable/realtek
    ${c_CMPT_AUDIO_DIR}/base/log/include
    ${c_CMPT_AUDIO_DIR}/base/cutils/interfaces
    ${c_CMPT_AUDIO_DIR}/base/cutils/include
    ${c_CMPT_AUDIO_DIR}/base/xlib/include
    ${c_CMPT_AUDIO_DIR}/base/osal/osal_c/include
)

ameba_list_append(private_definitions
    __RTOS__
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
