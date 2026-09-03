#pragma once

#include <vengine/core/Types.hpp>

#include <chrono>
#include <string>
#include <vector>

namespace vengine::profiler {

/// A named timing scope. Use the VENGINE_PROFILE macro for RAII timing.
struct Scope {
    std::string         name;
    double              seconds{0.0};
    std::uint64_t       calls{0};
};

/// Lightweight frame profiler. Records per-scope timings so the editor can
/// show a frame timeline (README Profiler module). Not thread-safe; intended
/// to be used from the main/render thread.
class Profiler {
public:
    static Profiler& instance();

    void reset() noexcept;

    /// Begin a scope; returns an opaque handle to end() with.
    std::size_t begin(std::string name);
    void        end(std::size_t handle);

    const std::vector<Scope>& scopes() const noexcept { return scopes_; }

    double frame_seconds() const noexcept { return frame_seconds_; }
    void   set_frame_seconds(double s) noexcept { frame_seconds_ = s; }

private:
    Profiler() = default;

    struct Active { std::string name; std::chrono::steady_clock::time_point start; };
    std::vector<Active>  active_;
    std::vector<Scope>   scopes_;
    double               frame_seconds_{0.0};
};

class ScopedProfile {
public:
    ScopedProfile(std::string name)
        : handle_(Profiler::instance().begin(std::move(name))) {}
    ~ScopedProfile() { if (handle_ != kInvalid) Profiler::instance().end(handle_); }
    ScopedProfile(const ScopedProfile&) = delete;
    ScopedProfile& operator=(const ScopedProfile&) = delete;
private:
    static constexpr std::size_t kInvalid = static_cast<std::size_t>(-1);
    std::size_t handle_{kInvalid};
};

#define VENGINE_PROFILE(name) ::vengine::profiler::ScopedProfile _vengine_prof_(name)

} // namespace vengine::profiler
