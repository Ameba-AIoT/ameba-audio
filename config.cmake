##########################################################################################
## * This part defines public part of the component
## * Public part will be used as global build configures for all component

set(public_includes)                # public include directories, NOTE: relative path is OK
set(public_definitions)             # public definitions
set(public_libraries)               # public libraries(files), NOTE: linked with whole-archive options

# ------------------------------------------------------------------#
# Component public part, user config begin(DO NOT remove this line)
# You may use if-else condition to set or update predefined variable above

# Component public part, user config end(DO NOT remove this line)
# ------------------------------------------------------------------#

# WARNING: Fixed section, DO NOT change!
ameba_global_include(${public_includes})
ameba_global_define(${public_definitions})
ameba_global_library(${public_libraries})

##########################################################################################
## * This part defines private part of the component
## * Private part is used to build target of current component
## * NOTE: The build API guarantees the global build configures(mentioned above)
## *       applied to the target automatically. So if any configure was already added
## *       to public, it's unnecessary to add again below.

# They are only for ameba_add_internal_library/ameba_add_external_app_library/ameba_add_external_soc_library
set(private_sources)                 # private source files, NOTE: relative path is OK
set(private_includes)                # private include directories, NOTE: relative path is OK
set(private_definitions)             # private definitions
set(private_compile_options)         # private compile_options

# Enable definition of various functions used throughout the testsuite
# (gethostname, strdup, fileno...) even when compiling with -std=c99. Harmless
# on non-POSIX platforms.
# add_definitions("-D_POSIX_C_SOURCE=199309")

ameba_list_append(private_includes
    ${c_CMPT_SOC_DIR}
    ${c_CMPT_AUDIO_DIR}/base/xlib/include
    ${c_CMPT_AUDIO_DIR}/base/log/include
    ${c_CMPT_AUDIO_DIR}/base/osal/osal_c/include
    ${c_CMPT_AUDIO_DIR}/base/osal/osal_cxx/include
    ${c_CMPT_AUDIO_DIR}/base/cutils/include
    ${c_CMPT_AUDIO_DIR}/base/audio_utils/include
    ${c_CMPT_AUDIO_DIR}/interfaces
    ${c_POSIX_DIR}/include
    ${c_POSIX_DIR}/FreeRTOS-Plus-POSIX/include
    ${c_POSIX_DIR}/FreeRTOS-Plus-POSIX/include/portable/realtek
)

ameba_list_append(private_compile_options
    -Wno-unused-parameter
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
)

ameba_list_append(private_definitions
    _POSIX_C_SOURCE=199309
    __RTOS__
)
