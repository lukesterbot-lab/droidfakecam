/*
 * DroidFakeCam - Configuration Header
 * 
 * Handles configuration settings loaded from control files.
 * 
 * For educational and research purposes only.
 */

#pragma once

#include <string>
#include <unistd.h>
#include <sys/stat.h>

namespace Config {

// Configuration paths
static constexpr const char* MEDIA_DIR = "/sdcard/DCIM/Camera1";
static constexpr const char* VIDEO_FILE = "/sdcard/DCIM/Camera1/virtual.mp4";
static constexpr const char* PHOTO_FILE = "/sdcard/DCIM/Camera1/1000.bmp";
static constexpr const char* DISABLE_FILE = "/sdcard/DCIM/Camera1/disable.jpg";
static constexpr const char* NO_TOAST_FILE = "/sdcard/DCIM/Camera1/no_toast.jpg";
static constexpr const char* PRIVATE_DIR_FILE = "/sdcard/DCIM/Camera1/private_dir.jpg";

// Check if a file exists
inline bool fileExists(const char* path) {
    struct stat st;
    return stat(path, &st) == 0;
}

// Check if module is disabled
inline bool isDisabled() {
    return fileExists(DISABLE_FILE);
}

// Check if toasts/logs should be suppressed
inline bool shouldSuppressLogs() {
    return fileExists(NO_TOAST_FILE);
}

// Check if app-specific directories should be used
inline bool usePrivateDir() {
    return fileExists(PRIVATE_DIR_FILE);
}

// Get media directory for an app
inline std::string getMediaDir(const std::string& appName) {
    if (usePrivateDir()) {
        return std::string(MEDIA_DIR) + "/" + appName;
    }
    return MEDIA_DIR;
}

// Get video file path
inline std::string getVideoPath(const std::string& appName = "") {
    if (usePrivateDir() && !appName.empty()) {
        std::string path = getMediaDir(appName) + "/virtual.mp4";
        if (fileExists(path.c_str())) {
            return path;
        }
    }
    return VIDEO_FILE;
}

// Get photo file path
inline std::string getPhotoPath(const std::string& appName = "") {
    if (usePrivateDir() && !appName.empty()) {
        std::string path = getMediaDir(appName) + "/1000.bmp";
        if (fileExists(path.c_str())) {
            return path;
        }
    }
    return PHOTO_FILE;
}

} // namespace Config
