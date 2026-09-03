#pragma once

// V Engine 2.0 — Quaternion (for 3D camera and billboard math).

#include <vengine/math/Vec3.hpp>

#include <cmath>

namespace vengine::math {

struct Quat {
    float x{0}, y{0}, z{0}, w{1};

    static Quat identity() { return Quat{}; }

    static Quat from_axis_angle(const Vec3f& axis, float radians) {
        const float half = radians * 0.5f;
        const float s = std::sin(half);
        return Quat{axis.x * s, axis.y * s, axis.z * s, std::cos(half)};
    }

    /// Minimal rotation from direction a to direction b (unit vectors).
    static Quat from_to(const Vec3f& a, const Vec3f& b) {
        const float d = a.dot(b);
        if (d >= 1.0f - 1e-6f) return Quat{};
        if (d <= -1.0f + 1e-6f) {
            // opposite: any perpendicular axis works
            Vec3f axis = std::abs(a.x) < 0.9f ? Vec3f{1,0,0} : Vec3f{0,1,0};
            axis = a.cross(axis).normalized();
            return Quat{axis.x, axis.y, axis.z, 0.0f};
        }
        const Vec3f c = a.cross(b);
        return Quat{c.x, c.y, c.z, 1.0f + d}.normalized();
    }

    Quat operator*(const Quat& o) const {
        return Quat{
            w * o.x + x * o.w + y * o.z - z * o.y,
            w * o.y - x * o.z + y * o.w + z * o.x,
            w * o.z + x * o.y - y * o.x + z * o.w,
            w * o.w - x * o.x - y * o.y - z * o.z,
        };
    }

    Quat conjugate() const { return Quat{-x, -y, -z, w}; }

    float length_squared() const { return x*x + y*y + z*z + w*w; }
    float length() const { return std::sqrt(length_squared()); }

    Quat normalized() const {
        const float l = length();
        if (l < 1e-8f) return Quat{};
        return Quat{x/l, y/l, z/l, w/l};
    }

    Vec3f rotate(const Vec3f& v) const {
        // v' = q * v * q^-1 (optimized, no quat-vec quat multiply)
        const Vec3f u{x, y, z};
        const Vec3f t = u.cross(v) * 2.0f;
        return v + t * w + u.cross(t);
    }

    /// Spherical linear interpolation.
    static Quat slerp(const Quat& a, const Quat& b, float t) {
        float d = a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
        Quat end = b;
        if (d < 0.0f) { d = -d; end = Quat{-b.x, -b.y, -b.z, -b.w}; }
        if (d > 1.0f - 1e-5f) {
            // nearly parallel: plain lerp
            return Quat{a.x + (end.x - a.x) * t,
                        a.y + (end.y - a.y) * t,
                        a.z + (end.z - a.z) * t,
                        a.w + (end.w - a.w) * t}.normalized();
        }
        const float theta_0 = std::acos(std::clamp(d, -1.0f, 1.0f));
        const float sin_theta_0 = std::sin(theta_0);
        const float s0 = std::sin((1.0f - t) * theta_0) / sin_theta_0;
        const float s1 = std::sin(t * theta_0) / sin_theta_0;
        return Quat{
            a.x*s0 + end.x*s1, a.y*s0 + end.y*s1,
            a.z*s0 + end.z*s1, a.w*s0 + end.w*s1,
        };
    }
};

} // namespace vengine::math
