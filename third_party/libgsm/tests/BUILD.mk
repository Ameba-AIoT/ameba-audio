LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SOURCES := main.c
LOCAL_MODULE := gsm_test
LOCAL_INCLUDES := \
    $(LOCAL_PATH)/../inc \
    base/cutils/interfaces \
    base/media \

LOCAL_SHARED_LIBRARIES := \
    libaudio_client \
    liblog \
    libutils \
    libosal_cxx \
    libosal_c \
    libgsm

include $(BUILD_EXECUTABLE)
