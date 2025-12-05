/*
 * DroidFakeCam - Camera Hook Header
 * 
 * Declares the camera hooking interface for intercepting and replacing
 * camera frames with custom content.
 * 
 * For educational and research purposes only.
 */

#pragma once

#include <jni.h>
#include <string>

namespace CameraHook {

// Initialize camera hooks for the current process
bool initialize(JNIEnv* env, const std::string& appName);

// Cleanup and release resources
void cleanup();

// Check if hooks are active
bool isActive();

// Set the custom video source
bool setVideoSource(const std::string& path);

// Set the custom photo source
bool setPhotoSource(const std::string& path);

// Get current hook status
struct HookStatus {
    bool initialized;
    bool videoSourceReady;
    bool photoSourceReady;
    int frameWidth;
    int frameHeight;
    int frameCount;
};

HookStatus getStatus();

} // namespace CameraHook
