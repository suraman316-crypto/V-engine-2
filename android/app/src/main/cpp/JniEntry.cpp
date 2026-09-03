// JNI entry point for V Engine on Android.
//
// The Java side (GameActivity) calls here at lifecycle points:
//   nativeOnCreate(assetManager)   -> construct Engine + AndroidPlatform
//   nativeOnSurfaceCreated(window) -> hand the window to the renderer
//   nativeOnResume()/nativeOnPause()
//   nativeOnDestroy()              -> Engine::shutdown()
// Each frame is driven by the Java Choreographer, which calls nativeTick(dt).

#include "AndroidPlatform.hpp"

#include <vengine/Engine.hpp>
#include <vengine/core/Logging.hpp>
#include <vengine/input/Input.hpp>
#include <vengine/platform/Platform.hpp>

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <jni.h>

#include <memory>

namespace {
struct EngineState {
    std::unique_ptr<vengine::Engine> engine;
    // window/filesystem are owned by the Engine's Platform after nativeOnCreate.
};
std::unique_ptr<EngineState> g_state;
} // namespace

extern "C" {

JNIEXPORT void JNICALL
Java_com_vengine_app_GameActivity_nativeOnCreate(JNIEnv* env, jobject, jobject jasset_mgr,
                                                   jstring jfiles_dir) {
    AAssetManager* mgr = AAssetManager_fromJava(env, jasset_mgr);
    const char* fd = jfiles_dir ? env->GetStringUTFChars(jfiles_dir, nullptr) : "";
    std::string files_dir = fd ? fd : "";
    if (jfiles_dir) env->ReleaseStringUTFChars(jfiles_dir, fd);

    g_state = std::make_unique<EngineState>();
    g_state->engine = std::make_unique<vengine::Engine>();
    auto& platform = g_state->engine->platform();
    platform.window     = std::make_unique<vengine::android::AndroidWindow>();
    platform.filesystem = std::make_unique<vengine::android::AndroidFileSystem>(files_dir);
    static_cast<vengine::android::AndroidFileSystem*>(platform.filesystem.get())
        ->set_asset_manager(mgr);
    platform.state = vengine::platform::LifecycleState::Started;

    auto res = g_state->engine->initialize();
    if (!res.ok()) {
        VENGINE_LOG_ERROR("Android", "engine initialize failed: %s",
                           res.error().format().c_str());
    }
}

JNIEXPORT void JNICALL
Java_com_vengine_app_GameActivity_nativeOnSurfaceCreated(JNIEnv* env, jobject, jobject jsurface) {
    if (!g_state) return;
    ANativeWindow* win = ANativeWindow_fromSurface(env, jsurface);
    auto* w = static_cast<vengine::android::AndroidWindow*>(
        g_state->engine->platform().window.get());
    if (w) w->set_native_window(win);
}

JNIEXPORT void JNICALL
Java_com_vengine_app_GameActivity_nativeOnResume(JNIEnv*, jobject) {
    if (g_state && g_state->engine) {
        g_state->engine->platform().state = vengine::platform::LifecycleState::Resumed;
    }
}

JNIEXPORT void JNICALL
Java_com_vengine_app_GameActivity_nativeOnPause(JNIEnv*, jobject) {
    if (g_state && g_state->engine) {
        g_state->engine->platform().state = vengine::platform::LifecycleState::Paused;
    }
}

JNIEXPORT void JNICALL
Java_com_vengine_app_GameActivity_nativeTick(JNIEnv*, jobject, jfloat dt) {
    if (!g_state || !g_state->engine) return;
    g_state->engine->update(dt);
    g_state->engine->render();
}

JNIEXPORT void JNICALL
Java_com_vengine_app_GameActivity_nativeOnDestroy(JNIEnv*, jobject) {
    if (g_state) {
        if (g_state->engine) g_state->engine->shutdown();
        g_state.reset();
    }
}

} // extern "C"
