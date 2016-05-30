LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	com_tencentbox_gamepad_MainService.c \
	inject_touch.c \
	inject_mouse.c \
	simu_event.c

LOCAL_C_INCLUDES += \
    bionic/libc/include

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libc \
	libnativehelper

#LOCAL_CFLAGS += -Wall -g3 -std=gnu99
LOCAL_CFLAGS += -Wall -g3 -std=gnu99 -DDEBUG_ENABLED

LOCAL_MODULE := libtranslation_routine

#LOCAL_MODULE_PATH := ${LOCAL_PATH}

LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
