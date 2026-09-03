#pragma once

#include <vengine/Common.hpp>
#include <vengine/core/Error.hpp>
#include <vengine/core/Types.hpp>
#include <vengine/math/Vec2.hpp>

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace vengine::platform {

/// Lifecycle phases of the engine on Android. The platform layer drives these.
enum class LifecycleState {
    Uninitialized,
    Started,
    Resumed,
    Paused,
    Stopped,
    Destroyed,
};

/// A window/surface description requested by the engine.
struct WindowConfig {
    std::string title{"V Engine"};
    int          width{1280};
    int          height{720};
    bool         fullscreen{false};
    bool         resizable{true};
    int          rotation_degrees{0}; ///< 0, 90, 180, 270
};

/// Runtime display metrics reported by the platform layer.
struct DisplayMetrics {
    int    width{0};
    int    height{0};
    float  density{1.0f};        ///< DPI / 160
    float  refresh_rate_hz{60.0f};
    math::Vec2f safe_area_min{0.0f, 0.0f}; ///< insets (notches)
    math::Vec2f safe_area_max{0.0f, 0.0f};
};

/// Abstract window/surface interface. The Android platform layer implements
/// this against ANativeWindow; a desktop stub can implement it against GLFW.
/// The renderer never touches the platform directly — it goes through here.
class IWindow {
public:
    virtual ~IWindow() = default;

    virtual Result<void> create(const WindowConfig& cfg) = 0;
    virtual void         destroy() = 0;
    virtual bool         is_valid() const noexcept = 0;

    /// Native handle for the renderer (e.g. EGLSurface/ANativeWindow*). The
    /// concrete type is platform-defined; the engine treats it as opaque.
    virtual void* native_handle() const noexcept = 0;

    virtual void         get_size(int& w, int& h) const noexcept = 0;
    virtual DisplayMetrics metrics() const noexcept = 0;
    virtual void         set_rotation(int degrees) = 0;
};

/// Abstract filesystem access. Android restricts filesystem access; the
/// platform layer implements this against app-internal storage + the Storage
/// Access Framework for user-picked files (README: file system safety).
class IFileSystem {
public:
    virtual ~IFileSystem() = default;

    virtual bool         exists(std::string_view path) const = 0;
    virtual Result<std::string> read_text(std::string_view path) const = 0;
    virtual Result<void> write_text(std::string_view path, std::string_view text) = 0;
    virtual Result<void> make_dirs(std::string_view path) = 0;
    /// Resolve a project-relative path to an absolute, sandbox-safe path.
    virtual std::string  resolve(std::string_view relative) const = 0;
};

/// Clock for deterministic frame timing. The platform layer provides real
/// time; tests can inject a fake clock.
class IClock {
public:
    virtual ~IClock() = default;
    virtual double now_seconds() const noexcept = 0;
};

/// The platform facade the engine talks to. Owned by the engine; populated by
/// the Android (or desktop) platform layer at startup.
struct Platform {
    std::unique_ptr<IWindow>     window;
    std::unique_ptr<IFileSystem> filesystem;
    std::unique_ptr<IClock>      clock;
    LifecycleState               state{LifecycleState::Uninitialized};
};

} // namespace vengine::platform
