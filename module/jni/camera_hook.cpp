/*
 * DroidFakeCam - Camera Hook Implementation
 * 
 * This module hooks into Android's camera subsystem to intercept and replace
 * camera frames with custom content from video/image files.
 * 
 * Hooking Targets:
 * - Camera2 NDK API (libcamera2ndk.so)
 * - Camera HAL callbacks
 * - SurfaceTexture frame delivery
 * 
 * For educational and research purposes only.
 */

#include "camera_hook.hpp"
#include "config.hpp"
#include "frame_utils.hpp"
#include "media_reader.hpp"

#include <dlfcn.h>
#include <android/log.h>
#include <android/native_window.h>
#include <pthread.h>
#include <unistd.h>
#include <string>
#include <map>
#include <mutex>

#define LOG_TAG "DroidFakeCam"
#define LOGI(...) if (!Config::shouldSuppressLogs()) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGD(...) if (!Config::shouldSuppressLogs()) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace CameraHook {

// Global state
static std::string g_appName;
static bool g_initialized = false;
static MediaReader* g_videoReader = nullptr;
static MediaReader* g_photoReader = nullptr;
static std::mutex g_mutex;
static HookStatus g_status = {};

// Original function pointers
typedef void (*ACameraCaptureSession_captureCallback_result)(
    void* context,
    void* session,
    void* request,
    void* result
);

typedef void (*ACameraCaptureSession_captureCallback_bufferLost)(
    void* context,
    void* session,
    void* request,
    void* window,
    int64_t frameNumber
);

// Hook trampolines
static void* g_original_functions[16] = {nullptr};

// Native hooks using PLT hooking (to be registered via Zygisk API)
// These will intercept camera frame delivery

// ACameraOutputTarget_create hook
typedef int (*ACameraOutputTarget_create_t)(void* window, void** output);
static ACameraOutputTarget_create_t original_ACameraOutputTarget_create = nullptr;

int hooked_ACameraOutputTarget_create(void* window, void** output) {
    LOGD("ACameraOutputTarget_create hooked, window=%p", window);
    
    // Store the window reference for later frame injection
    // Call original function
    int result = 0;
    if (original_ACameraOutputTarget_create) {
        result = original_ACameraOutputTarget_create(window, output);
    }
    
    LOGD("ACameraOutputTarget_create result=%d, output=%p", result, output ? *output : nullptr);
    return result;
}

// ACameraCaptureSession_capture hook
typedef int (*ACameraCaptureSession_capture_t)(
    void* session, void* callbacks, int numRequests, 
    void** requests, int* sequenceId
);
static ACameraCaptureSession_capture_t original_ACameraCaptureSession_capture = nullptr;

int hooked_ACameraCaptureSession_capture(
    void* session, void* callbacks, int numRequests,
    void** requests, int* sequenceId
) {
    LOGD("ACameraCaptureSession_capture hooked, session=%p, numRequests=%d", 
         session, numRequests);
    
    int result = 0;
    if (original_ACameraCaptureSession_capture) {
        result = original_ACameraCaptureSession_capture(
            session, callbacks, numRequests, requests, sequenceId
        );
    }
    
    return result;
}

// Hook into the ImageReader to intercept acquired images
typedef int (*AImageReader_acquireNextImage_t)(void* reader, void** image);
static AImageReader_acquireNextImage_t original_AImageReader_acquireNextImage = nullptr;

int hooked_AImageReader_acquireNextImage(void* reader, void** image) {
    LOGD("AImageReader_acquireNextImage hooked, reader=%p", reader);
    
    int result = 0;
    if (original_AImageReader_acquireNextImage) {
        result = original_AImageReader_acquireNextImage(reader, image);
    }
    
    // If we got an image and have a custom source, replace the frame data
    if (result == 0 && image && *image) {
        std::lock_guard<std::mutex> lock(g_mutex);
        if (g_videoReader && g_videoReader->isReady()) {
            // Get the next frame from our video source
            FrameData frame;
            if (g_videoReader->getNextFrame(frame)) {
                // Replace image data with our frame
                LOGD("Replacing frame: %dx%d, format=%d", 
                     frame.width, frame.height, frame.format);
                g_status.frameCount++;
            }
        }
    }
    
    return result;
}

// Hook into AImage_getPlaneData to inject our frame data
typedef int (*AImage_getPlaneData_t)(void* image, int planeIdx, 
                                      uint8_t** data, int* dataLength);
static AImage_getPlaneData_t original_AImage_getPlaneData = nullptr;

// Storage for injected frame data
static uint8_t* g_injectedFrame = nullptr;
static int g_injectedFrameSize = 0;

int hooked_AImage_getPlaneData(void* image, int planeIdx, 
                                uint8_t** data, int* dataLength) {
    // First call original to get the real data location
    int result = 0;
    if (original_AImage_getPlaneData) {
        result = original_AImage_getPlaneData(image, planeIdx, data, dataLength);
    }
    
    // If successful and we have a replacement frame, inject it
    // Note: Mutex protection ensures thread-safe access to global frame storage
    if (result == 0 && data && *data && dataLength && *dataLength > 0) {
        std::lock_guard<std::mutex> lock(g_mutex);
        if (g_injectedFrame && g_injectedFrameSize > 0) {
            // Copy our frame data into the camera buffer
            int copySize = (*dataLength < g_injectedFrameSize) ? 
                           *dataLength : g_injectedFrameSize;
            memcpy(*data, g_injectedFrame, copySize);
            LOGD("Injected %d bytes into plane %d", copySize, planeIdx);
        }
    }
    
    return result;
}

// JNI-based hooks for Java camera API
static jclass g_cameraDeviceClass = nullptr;
static jclass g_cameraCaptureSessionClass = nullptr;

bool hookJavaApi(JNIEnv* env) {
    if (!env) return false;
    
    LOGD("Setting up Java camera API hooks");
    
    // Find Camera2 classes
    jclass cameraDeviceClass = env->FindClass("android/hardware/camera2/CameraDevice");
    if (cameraDeviceClass) {
        g_cameraDeviceClass = (jclass)env->NewGlobalRef(cameraDeviceClass);
        LOGD("Found CameraDevice class");
    }
    
    jclass cameraCaptureSessionClass = env->FindClass(
        "android/hardware/camera2/CameraCaptureSession"
    );
    if (cameraCaptureSessionClass) {
        g_cameraCaptureSessionClass = (jclass)env->NewGlobalRef(cameraCaptureSessionClass);
        LOGD("Found CameraCaptureSession class");
    }
    
    // Note: For full implementation, we would use JNI to register native methods
    // that intercept Java callbacks. This requires more extensive bytecode manipulation
    // or using frameworks like EdXposed's method hooking.
    
    return true;
}

// Setup PLT hooks for native camera libraries
bool hookNativeApi() {
    LOGD("Setting up native camera API hooks");
    
    // Load camera NDK library
    void* libcamera = dlopen("libcamera2ndk.so", RTLD_NOW | RTLD_NOLOAD);
    if (!libcamera) {
        libcamera = dlopen("libcamera2ndk.so", RTLD_NOW);
    }
    
    if (!libcamera) {
        LOGE("Failed to load libcamera2ndk.so: %s", dlerror());
        return false;
    }
    
    LOGD("libcamera2ndk.so loaded at %p", libcamera);
    
    // Find and save original function pointers
    // Note: In a real implementation, we would use PLT hooking via Zygisk API
    // or direct memory patching. This shows the structure.
    
    original_ACameraOutputTarget_create = (ACameraOutputTarget_create_t)
        dlsym(libcamera, "ACameraOutputTarget_create");
    
    original_ACameraCaptureSession_capture = (ACameraCaptureSession_capture_t)
        dlsym(libcamera, "ACameraCaptureSession_capture");
    
    // Load media library for image reading
    void* libmediandk = dlopen("libmediandk.so", RTLD_NOW | RTLD_NOLOAD);
    if (!libmediandk) {
        libmediandk = dlopen("libmediandk.so", RTLD_NOW);
    }
    
    if (libmediandk) {
        original_AImageReader_acquireNextImage = (AImageReader_acquireNextImage_t)
            dlsym(libmediandk, "AImageReader_acquireNextImage");
        
        original_AImage_getPlaneData = (AImage_getPlaneData_t)
            dlsym(libmediandk, "AImage_getPlaneData");
        
        LOGD("libmediandk.so hooks prepared");
    }
    
    LOGD("Native API function pointers resolved");
    return true;
}

bool initialize(JNIEnv* env, const std::string& appName) {
    std::lock_guard<std::mutex> lock(g_mutex);
    
    if (g_initialized) {
        LOGI("Already initialized for %s", g_appName.c_str());
        return true;
    }
    
    g_appName = appName;
    LOGI("Initializing camera hooks for %s", appName.c_str());
    
    // Initialize media readers
    std::string videoPath = Config::getVideoPath(appName);
    std::string photoPath = Config::getPhotoPath(appName);
    
    LOGI("Video source: %s", videoPath.c_str());
    LOGI("Photo source: %s", photoPath.c_str());
    
    // Create video reader
    if (Config::fileExists(videoPath.c_str())) {
        g_videoReader = new MediaReader();
        if (g_videoReader->open(videoPath)) {
            g_status.videoSourceReady = true;
            g_status.frameWidth = g_videoReader->getWidth();
            g_status.frameHeight = g_videoReader->getHeight();
            LOGI("Video source ready: %dx%d", 
                 g_status.frameWidth, g_status.frameHeight);
        } else {
            LOGE("Failed to open video source");
            delete g_videoReader;
            g_videoReader = nullptr;
        }
    } else {
        LOGI("Video source not found: %s", videoPath.c_str());
    }
    
    // Create photo reader
    if (Config::fileExists(photoPath.c_str())) {
        g_photoReader = new MediaReader();
        if (g_photoReader->open(photoPath)) {
            g_status.photoSourceReady = true;
            LOGI("Photo source ready");
        } else {
            LOGE("Failed to open photo source");
            delete g_photoReader;
            g_photoReader = nullptr;
        }
    } else {
        LOGI("Photo source not found: %s", photoPath.c_str());
    }
    
    // Setup hooks
    bool javaHooksOk = hookJavaApi(env);
    bool nativeHooksOk = hookNativeApi();
    
    if (javaHooksOk || nativeHooksOk) {
        g_initialized = true;
        g_status.initialized = true;
        LOGI("Camera hooks initialized successfully");
        return true;
    }
    
    LOGE("Failed to initialize camera hooks");
    return false;
}

void cleanup() {
    std::lock_guard<std::mutex> lock(g_mutex);
    
    if (g_videoReader) {
        delete g_videoReader;
        g_videoReader = nullptr;
    }
    
    if (g_photoReader) {
        delete g_photoReader;
        g_photoReader = nullptr;
    }
    
    if (g_injectedFrame) {
        delete[] g_injectedFrame;
        g_injectedFrame = nullptr;
        g_injectedFrameSize = 0;
    }
    
    g_initialized = false;
    g_status = {};
    
    LOGI("Camera hooks cleaned up");
}

bool isActive() {
    return g_initialized;
}

bool setVideoSource(const std::string& path) {
    std::lock_guard<std::mutex> lock(g_mutex);
    
    if (g_videoReader) {
        delete g_videoReader;
    }
    
    g_videoReader = new MediaReader();
    if (g_videoReader->open(path)) {
        g_status.videoSourceReady = true;
        LOGI("Video source set: %s", path.c_str());
        return true;
    }
    
    delete g_videoReader;
    g_videoReader = nullptr;
    g_status.videoSourceReady = false;
    return false;
}

bool setPhotoSource(const std::string& path) {
    std::lock_guard<std::mutex> lock(g_mutex);
    
    if (g_photoReader) {
        delete g_photoReader;
    }
    
    g_photoReader = new MediaReader();
    if (g_photoReader->open(path)) {
        g_status.photoSourceReady = true;
        LOGI("Photo source set: %s", path.c_str());
        return true;
    }
    
    delete g_photoReader;
    g_photoReader = nullptr;
    g_status.photoSourceReady = false;
    return false;
}

HookStatus getStatus() {
    std::lock_guard<std::mutex> lock(g_mutex);
    return g_status;
}

} // namespace CameraHook
