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
    ${CMAKE_CURRENT_SOURCE_DIR}/${c_SOC_TYPE}/${c_MCU_TYPE}/lib_audio_spectrum.a
)

# Component public part, user config end
#----------------------------------------#

#WARNING: Fixed section, DO NOT change!
ameba_global_include(${public_includes})
ameba_global_define(${public_definitions})
ameba_global_library(${public_libraries}) #default: whole-archived

##########################################################################################

