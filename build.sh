#!/bin/bash
#
# DroidFakeCam Build Script
# Builds the native Zygisk library and packages the Magisk module
#
# Usage:
#   ./build.sh          - Build for all architectures
#   ./build.sh arm64    - Build for arm64-v8a only
#   ./build.sh clean    - Clean build artifacts
#
# Requirements:
#   - Android NDK r23+ installed
#   - ANDROID_NDK or ANDROID_NDK_HOME environment variable set
#
# For educational and research purposes only.

set -e

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MODULE_DIR="$SCRIPT_DIR/module"
JNI_DIR="$MODULE_DIR/jni"
ZYGISK_DIR="$MODULE_DIR/zygisk"
BUILD_DIR="$SCRIPT_DIR/build"
OUTPUT_DIR="$SCRIPT_DIR/out"

# NDK path detection
if [ -z "$ANDROID_NDK" ]; then
    if [ -n "$ANDROID_NDK_HOME" ]; then
        ANDROID_NDK="$ANDROID_NDK_HOME"
    elif [ -n "$ANDROID_SDK_ROOT" ]; then
        # Try to find NDK in SDK
        NDK_DIRS=("$ANDROID_SDK_ROOT/ndk" "$ANDROID_SDK_ROOT/ndk-bundle")
        for dir in "${NDK_DIRS[@]}"; do
            if [ -d "$dir" ]; then
                # Find latest version
                ANDROID_NDK=$(ls -1d "$dir"/*/ 2>/dev/null | tail -1 || echo "$dir")
                break
            fi
        done
    fi
fi

if [ -z "$ANDROID_NDK" ] || [ ! -d "$ANDROID_NDK" ]; then
    echo "Error: Android NDK not found"
    echo "Please set ANDROID_NDK or ANDROID_NDK_HOME environment variable"
    exit 1
fi

echo "Using NDK: $ANDROID_NDK"

# Architectures to build
ARCHS=("armeabi-v7a" "arm64-v8a" "x86" "x86_64")

# Parse arguments
if [ "$1" == "clean" ]; then
    echo "Cleaning build artifacts..."
    rm -rf "$BUILD_DIR" "$OUTPUT_DIR" "$ZYGISK_DIR"/*.so
    echo "Clean complete"
    exit 0
elif [ -n "$1" ]; then
    # Build specific architecture
    case "$1" in
        arm64|arm64-v8a)
            ARCHS=("arm64-v8a")
            ;;
        arm|armeabi-v7a)
            ARCHS=("armeabi-v7a")
            ;;
        x86)
            ARCHS=("x86")
            ;;
        x86_64|x64)
            ARCHS=("x86_64")
            ;;
        *)
            echo "Unknown architecture: $1"
            echo "Supported: arm64, arm, x86, x86_64"
            exit 1
            ;;
    esac
fi

# Create directories
mkdir -p "$ZYGISK_DIR" "$BUILD_DIR" "$OUTPUT_DIR"

# Build for each architecture using ndk-build
echo "Building native library..."

cd "$JNI_DIR"

"$ANDROID_NDK/ndk-build" \
    NDK_PROJECT_PATH="$MODULE_DIR" \
    NDK_OUT="$BUILD_DIR/obj" \
    NDK_LIBS_OUT="$BUILD_DIR/libs" \
    APP_BUILD_SCRIPT="$JNI_DIR/Android.mk" \
    NDK_APPLICATION_MK="$JNI_DIR/Application.mk" \
    -j$(nproc) \
    V=1

# Copy built libraries to zygisk directory
echo "Copying libraries to zygisk directory..."
for arch in "${ARCHS[@]}"; do
    LIB_PATH="$BUILD_DIR/libs/$arch/libdroidfakecam.so"
    if [ -f "$LIB_PATH" ]; then
        cp "$LIB_PATH" "$ZYGISK_DIR/$arch.so"
        echo "  $arch.so"
    else
        echo "  Warning: $arch library not found"
    fi
done

# Create module ZIP
echo "Packaging Magisk module..."

MODULE_ZIP="$OUTPUT_DIR/DroidFakeCam-v1.0.0.zip"

cd "$MODULE_DIR"
zip -r "$MODULE_ZIP" \
    module.prop \
    system.prop \
    customize.sh \
    service.sh \
    zygisk/

# Add README if exists
if [ -f "$SCRIPT_DIR/README.md" ]; then
    zip -j "$MODULE_ZIP" "$SCRIPT_DIR/README.md"
fi

echo ""
echo "=========================================="
echo " Build Complete!"
echo "=========================================="
echo ""
echo " Module ZIP: $MODULE_ZIP"
echo ""
echo " Architectures built:"
for arch in "${ARCHS[@]}"; do
    if [ -f "$ZYGISK_DIR/$arch.so" ]; then
        SIZE=$(ls -lh "$ZYGISK_DIR/$arch.so" | awk '{print $5}')
        echo "   - $arch ($SIZE)"
    fi
done
echo ""
echo " Install via:"
echo "   adb push $MODULE_ZIP /sdcard/"
echo "   Then install via Magisk Manager"
echo ""
