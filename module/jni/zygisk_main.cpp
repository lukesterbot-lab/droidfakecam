/*
 * DroidFakeCam - Zygisk Module Main Entry
 * 
 * This is the main Zygisk module entry point that handles:
 * - Module registration with Zygisk
 * - Process filtering (target camera-using apps)
 * - Library injection into target processes
 * 
 * For educational and research purposes only.
 */

#include <cstdlib>
#include <string>
#include <vector>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <android/log.h>

#include "zygisk.hpp"
#include "camera_hook.hpp"
#include "config.hpp"

#define LOG_TAG "DroidFakeCam"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

using zygisk::Api;
using zygisk::AppSpecializeArgs;
using zygisk::ServerSpecializeArgs;

class DroidFakeCamModule : public zygisk::ModuleBase {
public:
    void onLoad(Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
        LOGI("Module loaded into process");
    }

    void preAppSpecialize(AppSpecializeArgs *args) override {
        // Get app process name
        const char *process_name = nullptr;
        if (args->nice_name) {
            process_name = env->GetStringUTFChars(args->nice_name, nullptr);
        }
        
        if (process_name) {
            app_name = std::string(process_name);
            env->ReleaseStringUTFChars(args->nice_name, process_name);
            LOGD("preAppSpecialize: %s", app_name.c_str());
        }
        
        // Check if module is disabled
        if (Config::isDisabled()) {
            LOGI("Module disabled via disable.jpg");
            api->setOption(zygisk::DLCLOSE_MODULE_LIBRARY);
            return;
        }
        
        // Check if this app should be hooked (uses camera)
        if (shouldHookApp(app_name)) {
            should_hook = true;
            LOGI("Will hook camera for app: %s", app_name.c_str());
        } else {
            api->setOption(zygisk::DLCLOSE_MODULE_LIBRARY);
        }
    }

    void postAppSpecialize(const AppSpecializeArgs *args) override {
        if (!should_hook) {
            return;
        }
        
        LOGI("postAppSpecialize: Initializing camera hooks for %s", app_name.c_str());
        
        // Initialize camera hooks
        if (CameraHook::initialize(env, app_name)) {
            LOGI("Camera hooks initialized successfully");
        } else {
            LOGE("Failed to initialize camera hooks");
        }
    }

    void preServerSpecialize(ServerSpecializeArgs *args) override {
        // We don't need to hook system_server
        api->setOption(zygisk::DLCLOSE_MODULE_LIBRARY);
    }

private:
    Api *api = nullptr;
    JNIEnv *env = nullptr;
    std::string app_name;
    bool should_hook = false;
    
    bool shouldHookApp(const std::string &name) {
        // List of known camera apps and apps that use camera
        static const std::vector<std::string> camera_apps = {
            "com.android.camera",
            "com.android.camera2",
            "com.google.android.GoogleCamera",
            "com.sec.android.app.camera",
            "com.huawei.camera",
            "com.oppo.camera",
            "com.miui.camera",
            "com.oneplus.camera",
            "com.sonymobile.camera",
            "org.codeaurora.snapcam",
            "com.motorola.camera",
            "com.lge.camera",
            "com.asus.camera",
            "net.sourceforge.opencamera",
            // Video call apps
            "com.google.android.apps.meetings",
            "us.zoom.videomeetings",
            "com.microsoft.teams",
            "com.skype.raider",
            "com.discord",
            "com.whatsapp",
            "com.facebook.orca",
            "org.telegram.messenger",
            "com.viber.voip",
            "com.snapchat.android",
            "com.instagram.android",
            "com.zhiliaoapp.musically", // TikTok
        };
        
        for (const auto &app : camera_apps) {
            if (name.find(app) != std::string::npos) {
                return true;
            }
        }
        
        // Hook all apps for now (can be made configurable)
        // This allows any app using camera APIs to be hooked
        return true;
    }
};

REGISTER_ZYGISK_MODULE(DroidFakeCamModule)
