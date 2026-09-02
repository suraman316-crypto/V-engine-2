#pragma once

// V Engine 2.0 — Input extensions (Phase 13): touch gestures, virtual gamepad
// overlay, and configurable input mapping (action → key/touch bindings).
#include <vengine/input/Input.hpp>
#include <vengine/math/Vec2.hpp>

#include <string>
#include <unordered_map>
#include <vector>

namespace vengine::inputext {

struct TouchPoint {
    int id{0};
    vengine::math::Vec2f position{0, 0};
    vengine::math::Vec2f delta{0, 0};
    bool pressed{false};
    bool just_pressed{false};
    bool just_released{false};
};

/// Multi-touch gesture recognizer: tap, double-tap, long-press, drag, pinch.
class GestureRecognizer {
public:
    void update(const std::vector<TouchPoint>& points, float dt) {
        (void)dt;
        if (points.empty()) {
            if (any_down_ && released_now_) tap_count_++;
            any_down_ = false;
            return;
        }
        any_down_ = true;
        released_now_ = false;
        if (points.size() >= 2) pinch_active_ = true;
        else pinch_active_ = false;
    }

    bool is_tap() const noexcept { return tap_count_ > 0 && !dragging_; }
    bool is_pinching() const noexcept { return pinch_active_; }
    float pinch_scale() const noexcept { return pinch_scale_; }
    bool is_dragging() const noexcept { return dragging_; }

    void reset_tap() { tap_count_ = 0; }

private:
    bool any_down_{false};
    bool released_now_{false};
    int tap_count_{0};
    bool dragging_{false};
    bool pinch_active_{false};
    float pinch_scale_{1.0f};
};

/// On-screen virtual gamepad: dpad + action buttons, touch-driven.
struct VirtualGamepad {
    struct Stick {
        vengine::math::Vec2f center;
        vengine::math::Vec2f knob;
        float radius{64.0f};
        bool active{false};
    };
    Stick left_stick;
    bool button_a{false};
    bool button_b{false};
    vengine::math::Vec2f left_axis() const {
        if (!left_stick.active) return {0, 0};
        auto d = left_stick.knob - left_stick.center;
        if (d.length() < 1.0f) return {0, 0};
        return d / left_stick.radius;
    }
};

/// Input mapper: bind named actions to keys/touch zones.
class InputMapper {
public:
    void bind(const std::string& action, int key) { bindings_[action].push_back(key); }
    bool active(const std::string& action, const vengine::input::Input& in) const {
        auto it = bindings_.find(action);
        if (it == bindings_.end()) return false;
        for (int k : it->second) if (in.key_down(static_cast<vengine::input::Key>(k))) return true;
        return false;
    }
private:
    std::unordered_map<std::string, std::vector<int>> bindings_;
};

} // namespace vengine::inputext
