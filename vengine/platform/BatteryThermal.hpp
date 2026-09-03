#pragma once

// V Engine 2.0 — Battery & thermal manager (Phase 19): frame pacing and
// quality scaling that reacts to device thermal state and battery level to
// avoid throttling and extend playtime.

#include <vengine/Common.hpp>

namespace vengine::power {

struct PowerState {
    int battery_percent{100};
    bool charging{false};
    float thermal_pressure{0.0f};  ///< [0,1] 1 = critical
    float cpu_usage{0.0f};         ///< [0,1]
};

/// Decides whether to scale down quality or cap the frame rate to reduce
/// heat / battery drain.
class BatteryThermalManager {
public:
    void update(const PowerState& s, float dt) {
        state_ = s;
        cooldown_ -= dt;
        if (cooldown_ <= 0.0f) {
            evaluate();
            cooldown_ = 5.0f;  // re-evaluate every 5s
        }
    }

    /// Recommended target FPS, possibly reduced under load.
    int target_fps(int base) const noexcept {
        if (state_.thermal_pressure > 0.8f || state_.battery_percent < 15)
            return std::max(30, base / 2);
        if (state_.thermal_pressure > 0.5f || (state_.battery_percent < 30 && !state_.charging))
            return std::max(30, static_cast<int>(base * 0.75f));
        return base;
    }

    /// Recommend downgrading quality one step.
    bool should_reduce_quality() const noexcept {
        return state_.thermal_pressure > 0.7f || state_.cpu_usage > 0.95f;
    }
    bool should_pause() const noexcept {
        return state_.thermal_pressure > 0.95f;
    }
    const PowerState& state() const noexcept { return state_; }

private:
    void evaluate() {
        if (should_reduce_quality()) scaled_down_ = true;
    }
    PowerState state_;
    float cooldown_{0.0f};
    bool scaled_down_{false};
};

/// Frame pacer: spreads rendering evenly to hit a target FPS without
/// busy-spin, respecting VSYNC when available.
class FramePacer {
public:
    explicit FramePacer(int target_fps) : target_ms_(1000.0f / target_fps) {}

    /// Returns the time (in seconds) to sleep before the next frame, or 0.
    float sleep_time(float frame_time_ms) const noexcept {
        float remaining = target_ms_ - frame_time_ms;
        return remaining > 1.0f ? remaining * 0.001f : 0.0f;
    }
    float target_frame_ms() const noexcept { return target_ms_; }

private:
    float target_ms_;
};

} // namespace vengine::power
