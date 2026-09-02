#pragma once

// Easing functions and tween utilities.
//
// Animation curves, UI transitions, and particle lifetime all need non-linear
// interpolation. Each function takes t in [0,1] and returns the eased value.
// They are branch-free (or near-branch-free) and header-only.

#include <vengine/core/Types.hpp>

#include <cmath>

namespace vengine::math {

enum class Ease {
    Linear,
    InQuad,  OutQuad,  InOutQuad,
    InCubic, OutCubic, InOutCubic,
    InQuart, OutQuart, InOutQuart,
    InExpo,  OutExpo,  InOutExpo,
    InBack,  OutBack,  InOutBack,
    InElastic, OutElastic, InOutElastic,
    InBounce, OutBounce, InOutBounce,
};

inline float linear(float t) noexcept { return t; }

inline float in_quad(float t)  noexcept { return t * t; }
inline float out_quad(float t) noexcept { return 1.0f - (1.0f - t) * (1.0f - t); }
inline float in_out_quad(float t) noexcept {
    return t < 0.5f ? 2.0f * t * t : 1.0f - 2.0f * (1.0f - t) * (1.0f - t);
}

inline float in_cubic(float t)  noexcept { return t * t * t; }
inline float out_cubic(float t) noexcept { float f = 1.0f - t; return 1.0f - f * f * f; }
inline float in_out_cubic(float t) noexcept {
    return t < 0.5f ? 4.0f * t * t * t : 1.0f - 4.0f * (1.0f - t) * (1.0f - t) * (1.0f - t);
}

inline float in_quart(float t)  noexcept { return t * t * t * t; }
inline float out_quart(float t) noexcept { float f = 1.0f - t; return 1.0f - f * f * f * f; }
inline float in_out_quart(float t) noexcept {
    return t < 0.5f ? 8.0f * t * t * t * t : 1.0f - 8.0f * (1.0f - t) * (1.0f - t) * (1.0f - t) * (1.0f - t);
}

inline float in_expo(float t)  noexcept { return t <= 0.0f ? 0.0f : std::pow(2.0f, 10.0f * (t - 1.0f)); }
inline float out_expo(float t) noexcept { return t >= 1.0f ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t); }
inline float in_out_expo(float t) noexcept {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    return t < 0.5f ? std::pow(2.0f, 20.0f * t - 10.0f) * 0.5f
                    : (2.0f - std::pow(2.0f, -20.0f * t + 10.0f)) * 0.5f;
}

inline float in_back(float t) noexcept {
    constexpr float c1 = 1.70158f;
    constexpr float c3 = c1 + 1.0f;
    return c3 * t * t * t - c1 * t * t;
}
inline float out_back(float t) noexcept {
    constexpr float c1 = 1.70158f;
    constexpr float c3 = c1 + 1.0f;
    float f = t - 1.0f;
    return 1.0f + c3 * f * f * f + c1 * f * f;
}
inline float in_out_back(float t) noexcept {
    constexpr float c1 = 1.70158f * 1.525f;
    return t < 0.5f
        ? (4.0f * t * t * ((c1 + 1.0f) * 2.0f * t - c1)) * 0.5f
        : (4.0f * (t - 1.0f) * (t - 1.0f) * ((c1 + 1.0f) * 2.0f * (t - 1.0f) + c1) + 2.0f) * 0.5f;
}

inline float in_elastic(float t) noexcept {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    constexpr float c4 = (2.0f * 3.14159265358979323846f) / 3.0f;
    return -std::pow(2.0f, 10.0f * (t - 1.0f)) * std::sin((t * 10.0f - 10.75f) * c4);
}
inline float out_elastic(float t) noexcept {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    constexpr float c4 = (2.0f * 3.14159265358979323846f) / 3.0f;
    return std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * c4) + 1.0f;
}

inline float out_bounce(float t) noexcept {
    constexpr float n1 = 7.5625f;
    constexpr float d1 = 2.75f;
    if (t < 1.0f / d1)        return n1 * t * t;
    else if (t < 2.0f / d1)   { t -= 1.5f / d1;   return n1 * t * t + 0.75f; }
    else if (t < 2.5f / d1)   { t -= 2.25f / d1;  return n1 * t * t + 0.9375f; }
    else                      { t -= 2.625f / d1; return n1 * t * t + 0.984375f; }
}
inline float in_bounce(float t)  noexcept { return 1.0f - out_bounce(1.0f - t); }
inline float in_out_bounce(float t) noexcept {
    return t < 0.5f ? (1.0f - out_bounce(1.0f - 2.0f * t)) * 0.5f
                    : out_bounce(2.0f * t - 1.0f) * 0.5f + 0.5f;
}

inline float ease(Ease e, float t) noexcept {
    switch (e) {
    case Ease::Linear:       return linear(t);
    case Ease::InQuad:       return in_quad(t);
    case Ease::OutQuad:      return out_quad(t);
    case Ease::InOutQuad:    return in_out_quad(t);
    case Ease::InCubic:      return in_cubic(t);
    case Ease::OutCubic:     return out_cubic(t);
    case Ease::InOutCubic:   return in_out_cubic(t);
    case Ease::InQuart:      return in_quart(t);
    case Ease::OutQuart:     return out_quart(t);
    case Ease::InOutQuart:   return in_out_quart(t);
    case Ease::InExpo:       return in_expo(t);
    case Ease::OutExpo:      return out_expo(t);
    case Ease::InOutExpo:    return in_out_expo(t);
    case Ease::InBack:       return in_back(t);
    case Ease::OutBack:      return out_back(t);
    case Ease::InOutBack:    return in_out_back(t);
    case Ease::InElastic:    return in_elastic(t);
    case Ease::OutElastic:   return out_elastic(t);
    case Ease::InOutElastic: return 0.5f * (in_elastic(2.0f * t) + out_elastic(2.0f * t - 1.0f));
    case Ease::InBounce:     return in_bounce(t);
    case Ease::OutBounce:    return out_bounce(t);
    case Ease::InOutBounce:  return in_out_bounce(t);
    }
    return t;
}

/// Clamp t to [0,1] then apply the easing curve.
inline float ease_clamped(Ease e, float t) noexcept {
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return ease(e, t);
}

/// Function-pointer aliases for use as Easer callbacks (tweening, UI).
inline float ease_linear(float t)      noexcept { return linear(t); }
inline float ease_in_quad(float t)     noexcept { return in_quad(t); }
inline float ease_out_quad(float t)    noexcept { return out_quad(t); }
inline float ease_in_out_quad(float t) noexcept { return in_out_quad(t); }
inline float ease_in_cubic(float t)   noexcept { return in_cubic(t); }
inline float ease_out_cubic(float t)   noexcept { return out_cubic(t); }
inline float ease_in_out_cubic(float t) noexcept { return in_out_cubic(t); }
inline float ease_in_expo(float t)     noexcept { return in_expo(t); }
inline float ease_out_expo(float t)    noexcept { return out_expo(t); }
inline float ease_in_back(float t)     noexcept { return in_back(t); }
inline float ease_out_back(float t)    noexcept { return out_back(t); }
inline float ease_out_bounce(float t) noexcept { return out_bounce(t); }

} // namespace vengine::math
