#pragma once

// Tween system: property interpolation with easing for UI, cameras, gameplay.
//
// Spritesheet animation lives in Animation.hpp; this module provides the
// generic value-tween layer (float/Vec2/Color) with start/update/complete
// callbacks, looping, and ping-pong. TweenManager owns active tweens, advances
// them with a per-frame dt, and retires finished ones.

#include <vengine/core/Types.hpp>
#include <vengine/math/Color.hpp>
#include <vengine/math/Easing.hpp>
#include <vengine/math/Vec2.hpp>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <vector>

namespace vengine::animation {

using Easer = float (*)(float);

struct Tween {
    using Setter   = std::function<void(float)>;
    using Callback = std::function<void()>;

    Setter   setter;
    float    from{0.0f};
    float    to{1.0f};
    float    duration{1.0f};
    float    elapsed{0.0f};
    Easer    ease{math::ease_linear};
    bool     loop{false};
    bool     ping_pong{false};
    bool     alive{true};
    bool     started{false};
    Callback on_start;
    Callback on_complete;

    void update(float dt) {
        if (!alive) return;
        if (!started) { started = true; if (on_start) on_start(); }
        elapsed += dt;
        float t = duration > 0.0f ? std::clamp(elapsed / duration, 0.0f, 1.0f) : 1.0f;
        if (setter) setter(from + (to - from) * ease(t));
        if (elapsed >= duration) {
            if (loop) {
                elapsed = 0.0f;
                if (ping_pong) std::swap(from, to);
            } else {
                alive = false;
                if (on_complete) on_complete();
            }
        }
    }
};

struct TweenVec2 {
    using Setter   = std::function<void(const math::Vec2f&)>;
    using Callback = std::function<void()>;

    Setter      setter;
    math::Vec2f from{}, to{};
    float       duration{1.0f}, elapsed{0.0f};
    Easer       ease{math::ease_linear};
    bool        loop{false}, alive{true};
    Callback    on_complete;

    void update(float dt) {
        if (!alive) return;
        elapsed += dt;
        float t = duration > 0.0f ? std::clamp(elapsed / duration, 0.0f, 1.0f) : 1.0f;
        if (setter) setter(from.lerp(to, ease(t)));
        if (elapsed >= duration) {
            if (loop) elapsed = 0.0f;
            else { alive = false; if (on_complete) on_complete(); }
        }
    }
};

struct TweenColor {
    using Setter   = std::function<void(const math::Color&)>;
    using Callback = std::function<void()>;

    Setter     setter;
    math::Color from{1,1,1,1}, to{1,1,1,1};
    float      duration{1.0f}, elapsed{0.0f};
    Easer      ease{math::ease_linear};
    bool       loop{false}, alive{true};
    Callback   on_complete;

    void update(float dt) {
        if (!alive) return;
        elapsed += dt;
        float t = duration > 0.0f ? std::clamp(elapsed / duration, 0.0f, 1.0f) : 1.0f;
        float e = ease(t);
        if (setter) setter(math::Color{
            from.r + (to.r - from.r) * e,
            from.g + (to.g - from.g) * e,
            from.b + (to.b - from.b) * e,
            from.a + (to.a - from.a) * e,
        });
        if (elapsed >= duration) {
            if (loop) elapsed = 0.0f;
            else { alive = false; if (on_complete) on_complete(); }
        }
    }
};

class TweenManager {
public:
    Tween&      add(Tween t)      { tweens_.push_back(std::move(t)); return tweens_.back(); }
    TweenVec2&  add(TweenVec2 t)  { vec2_.push_back(std::move(t));   return vec2_.back(); }
    TweenColor& add(TweenColor t) { colors_.push_back(std::move(t)); return colors_.back(); }

    void update(float dt) {
        for (auto& t : tweens_) t.update(dt);
        for (auto& t : vec2_)   t.update(dt);
        for (auto& t : colors_) t.update(dt);
        std::erase_if(tweens_, [](const Tween& t){ return !t.alive; });
        std::erase_if(vec2_,   [](const TweenVec2& t){ return !t.alive; });
        std::erase_if(colors_, [](const TweenColor& t){ return !t.alive; });
    }
    void clear() noexcept { tweens_.clear(); vec2_.clear(); colors_.clear(); }
    std::size_t count() const noexcept { return tweens_.size() + vec2_.size() + colors_.size(); }

private:
    std::vector<Tween>      tweens_;
    std::vector<TweenVec2>  vec2_;
    std::vector<TweenColor> colors_;
};

} // namespace vengine::animation
