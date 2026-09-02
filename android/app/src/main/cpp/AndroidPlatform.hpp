#pragma once

// Android platform layer for V Engine.
//
// Implements vengine::platform::IWindow and IFileSystem on Android:
//   * IWindow      -> ANativeWindow (render surface)
//   * IFileSystem  -> AAssetManager for reads, internal storage for writes
// Compiled with the Android NDK; not part of the host (Linux) build.

#include <vengine/platform/Platform.hpp>

#include <android/asset_manager.h>
#include <android/input.h>
#include <android/native_window.h>

#include <string>

namespace vengine::android {

/// IWindow backed by an ANativeWindow handed in from Java via JNI.
class AndroidWindow : public platform::IWindow {
public:
    AndroidWindow() = default;

    Result<void> create(const platform::WindowConfig& cfg) override;
    void         destroy() override;
    bool         is_valid() const noexcept override { return window_ != nullptr; }
    void*        native_handle() const noexcept override { return reinterpret_cast<void*>(window_); }
    void         get_size(int& w, int& h) const noexcept override;
    platform::DisplayMetrics metrics() const noexcept override;
    void         set_rotation(int degrees) override { rotation_ = degrees; }

    /// Inject the surface from Java (SurfaceHolder.Callback.surfaceCreated).
    void set_native_window(ANativeWindow* win) noexcept;

private:
    ANativeWindow* window_{nullptr};
    int            rotation_{0};
};

/// IFileSystem backed by AAssetManager (reads) + app-internal storage (writes).
class AndroidFileSystem : public platform::IFileSystem {
public:
    explicit AndroidFileSystem(std::string files_dir) : files_dir_(std::move(files_dir)) {}

    bool                 exists(std::string_view path) const override;
    Result<std::string>  read_text(std::string_view path) const override;
    Result<void>         write_text(std::string_view path, std::string_view text) override;
    Result<void>         make_dirs(std::string_view path) override;
    std::string          resolve(std::string_view relative) const override;

    void set_asset_manager(AAssetManager* mgr) noexcept { asset_manager_ = mgr; }

private:
    AAssetManager* asset_manager_{nullptr};
    std::string    files_dir_;
};

} // namespace vengine::android
