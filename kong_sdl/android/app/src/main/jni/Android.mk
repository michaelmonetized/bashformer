LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := platformergame

# Source files
LOCAL_SRC_FILES := \
    platformer_game_jni.c \
    ../../shared/platformer_sdl.c

# Include directories
LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/../../shared

# Compiler flags
LOCAL_CFLAGS := -Wall -Wextra -O2

# Libraries
LOCAL_LDLIBS := -llog -landroid -lm

# Build shared library
include $(BUILD_SHARED_LIBRARY)


