/*
 * DroidFakeCam - Zygisk API Header
 * 
 * This header defines the Zygisk API interface that modules must implement.
 * Based on the official Zygisk API specification.
 * 
 * For educational and research purposes only.
 */

#pragma once

#include <jni.h>

namespace zygisk {

struct Api;
struct AppSpecializeArgs;
struct ServerSpecializeArgs;

// Module version - must match Zygisk version
#define ZYGISK_API_VERSION 4

// Module option flags
enum Option {
    // Request Zygisk to close the library after post[App|Server]Specialize
    DLCLOSE_MODULE_LIBRARY = 0,
    // Force deny any functionality that depends on forks from zygote
    FORCE_DENYLIST_UNMOUNT = 1
};

// Type of request when connecting to companion
enum {
    ZYGISK_CONNECT_COMPANION = 0,
};

// Module base class - modules must inherit from this
class ModuleBase {
public:
    // Called when the module is loaded
    virtual void onLoad(Api *api, JNIEnv *env) {}
    
    // Called before app process specialization
    virtual void preAppSpecialize(AppSpecializeArgs *args) {}
    
    // Called after app process specialization
    virtual void postAppSpecialize(const AppSpecializeArgs *args) {}
    
    // Called before server process specialization
    virtual void preServerSpecialize(ServerSpecializeArgs *args) {}
    
    // Called after server process specialization
    virtual void postServerSpecialize(const ServerSpecializeArgs *args) {}
};

// Arguments passed to app specialize
struct AppSpecializeArgs {
    jint *uid;
    jint *gid;
    jintArray *gids;
    jint *runtime_flags;
    jobjectArray *rlimits;
    jint *mount_external;
    jstring *se_info;
    jstring *nice_name;
    jstring *instruction_set;
    jstring *app_data_dir;
    
    // Optional fields (API 4+)
    jintArray *fds_to_ignore;
    jboolean *is_child_zygote;
    jboolean *is_top_app;
    jobjectArray *pkg_data_info_list;
    jobjectArray *whitelisted_data_info_list;
    jboolean *mount_data_dirs;
    jboolean *mount_storage_dirs;
};

// Arguments passed to server specialize
struct ServerSpecializeArgs {
    jint *uid;
    jint *gid;
    jintArray *gids;
    jint *runtime_flags;
    jlong *permitted_capabilities;
    jlong *effective_capabilities;
};

// Zygisk API interface
struct Api {
    // Connect to companion daemon
    int connectCompanion();
    
    // Get module directory
    void setOption(Option opt);
    
    // Get module library path
    int getModuleDir();
    
    // Hook JNI function
    void hookJniNativeMethods(JNIEnv *env, const char *className, 
                              JNINativeMethod *methods, int numMethods);
    
    // PLT hook
    void pltHookRegister(const char *name, void *hook, void **old);
    void pltHookExclude(const char *name, const char *library);
    bool pltHookCommit();
    
    // Transparent library loading
    int exemptFd(int fd);
};

} // namespace zygisk

// Module registration macro
#define REGISTER_ZYGISK_MODULE(clazz) \
    void *zygisk_module_entry = (void *) []() -> zygisk::ModuleBase * { return new clazz(); }; \
    int zygisk_module_api_version = ZYGISK_API_VERSION;
