# Application.mk for DroidFakeCam Zygisk Module

APP_ABI := armeabi-v7a arm64-v8a x86 x86_64

# Minimum Android API level - keep in sync with CMakeLists.txt
APP_PLATFORM := android-26

APP_STL := c++_static

APP_CPPFLAGS := -std=c++17

APP_OPTIM := release

APP_THIN_ARCHIVE := true
