# Android.mk for DroidFakeCam Zygisk Module
# Alternative to CMake for ndk-build

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := droidfakecam

LOCAL_SRC_FILES := \
    zygisk_main.cpp \
    camera_hook.cpp \
    frame_utils.cpp \
    media_reader.cpp

LOCAL_C_INCLUDES := $(LOCAL_PATH)

LOCAL_LDLIBS := \
    -llog \
    -landroid \
    -lmediandk \
    -ldl

LOCAL_CPPFLAGS := \
    -std=c++17 \
    -Wall \
    -Wextra \
    -fno-exceptions \
    -fno-rtti \
    -fvisibility=hidden \
    -ffunction-sections \
    -fdata-sections \
    -DNDEBUG

LOCAL_LDFLAGS := \
    -Wl,--gc-sections \
    -Wl,--exclude-libs,ALL \
    -s

include $(BUILD_SHARED_LIBRARY)
