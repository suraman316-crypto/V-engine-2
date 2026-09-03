#include "AndroidPlatform.hpp"

#include <vengine/core/Logging.hpp>

#include <android/asset_manager.h>
#include <android/native_window.h>
#include <sys/stat.h>

#include <fstream>
#include <cstring>

namespace vengine::android {

// ------------------------------ AndroidWindow ------------------------------

Result<void> AndroidWindow::create(const platform::WindowConfig& cfg) {
    (void)cfg; // the surface is supplied later via set_native_window()
    return Result<void>::Ok();
}

void AndroidWindow::destroy() {
    // ANativeWindow is owned by the Java side; just drop our reference.
    window_ = nullptr;
}

void AndroidWindow::set_native_window(ANativeWindow* win) noexcept {
    window_ = win;
}

void AndroidWindow::get_size(int& w, int& h) const noexcept {
    if (window_) {
        w = ANativeWindow_getWidth(window_);
        h = ANativeWindow_getHeight(window_);
    } else { w = 0; h = 0; }
}

platform::DisplayMetrics AndroidWindow::metrics() const noexcept {
    platform::DisplayMetrics m;
    get_size(m.width, m.height);
    return m;
}

// ---------------------------- AndroidFileSystem ----------------------------

bool AndroidFileSystem::exists(std::string_view path) const {
    if (!asset_manager_) return false;
    std::string p{path};
    AAsset* a = AAssetManager_open(asset_manager_, p.c_str(), AASSET_MODE_STREAMING);
    if (a) { AAsset_close(a); return true; }
    // Fall back to internal-storage writes.
    struct stat st{};
    return ::stat(p.c_str(), &st) == 0;
}

Result<std::string> AndroidFileSystem::read_text(std::string_view path) const {
    if (!asset_manager_) {
        return Error::from(ErrorCode::IoFailure, "no Android asset manager");
    }
    std::string p{path};
    AAsset* asset = AAssetManager_open(asset_manager_, p.c_str(), AASSET_MODE_BUFFER);
    if (!asset) {
        return Error::from(ErrorCode::FileNotFound, p, RecoveryHint::ReimportAsset);
    }
    std::size_t len = static_cast<std::size_t>(AAsset_getLength(asset));
    std::string out;
    out.resize(len);
    AAsset_read(asset, out.data(), len);
    AAsset_close(asset);
    return Result<std::string>::Ok(std::move(out));
}

Result<void> AndroidFileSystem::write_text(std::string_view path, std::string_view text) {
    std::string full = resolve(path);
    std::ofstream o(full, std::ios::binary | std::ios::trunc);
    if (!o) {
        return Error::from(ErrorCode::IoFailure, "cannot open " + full,
                            RecoveryHint::CheckPermissions);
    }
    o.write(text.data(), static_cast<std::streamsize>(text.size()));
    if (!o) return Error::from(ErrorCode::IoFailure, "write failed");
    return Result<void>::Ok();
}

Result<void> AndroidFileSystem::make_dirs(std::string_view path) {
    std::string p = resolve(path);
    for (std::size_t i = 1; i < p.size(); ++i) {
        if (p[i] == '/') { p[i] = '\0'; ::mkdir(p.c_str(), 0700); p[i] = '/'; }
    }
    ::mkdir(p.c_str(), 0700);
    return Result<void>::Ok();
}

std::string AndroidFileSystem::resolve(std::string_view relative) const {
    if (!files_dir_.empty() && relative.size() > 0 && relative[0] != '/') {
        return files_dir_ + "/" + std::string{relative};
    }
    return std::string{relative};
}

} // namespace vengine::android

