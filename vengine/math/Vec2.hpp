#pragma once

#include <vengine/core/Types.hpp>
#include <cmath>
#include <type_traits>

namespace vengine::math {

/// 2D vector. Trivially copyable; no virtual functions.
template <typename T>
struct Vec2 {
    static_assert(std::is_arithmetic_v<T>, "Vec2 requires an arithmetic type");

    T x{};
    T y{};

    constexpr Vec2() = default;
    constexpr Vec2(T x_, T y_) noexcept : x(x_), y(y_) {}

    constexpr static Vec2 zero() noexcept { return Vec2{0, 0}; }
    constexpr static Vec2 one()  noexcept { return Vec2{1, 1}; }

    constexpr Vec2 operator+(const Vec2& o) const noexcept { return {x + o.x, y + o.y}; }
    constexpr Vec2 operator-(const Vec2& o) const noexcept { return {x - o.x, y - o.y}; }
    constexpr Vec2 operator-() const noexcept { return {-x, -y}; }
    constexpr Vec2 operator*(T s) const noexcept { return {x * s, y * s}; }
    constexpr Vec2 operator/(T s) const noexcept { return {x / s, y / s}; }
    constexpr Vec2 operator*(const Vec2& o) const noexcept { return {x * o.x, y * o.y}; }

    constexpr Vec2& operator+=(const Vec2& o) noexcept { x += o.x; y += o.y; return *this; }
    constexpr Vec2& operator-=(const Vec2& o) noexcept { x -= o.x; y -= o.y; return *this; }
    constexpr Vec2& operator*=(T s) noexcept { x *= s; y *= s; return *this; }
    constexpr Vec2& operator/=(T s) noexcept { x /= s; y /= s; return *this; }

    constexpr bool operator==(const Vec2& o) const noexcept = default;

    constexpr T dot(const Vec2& o) const noexcept { return x * o.x + y * o.y; }
    constexpr T cross(const Vec2& o) const noexcept { return x * o.y - y * o.x; }

    T length() const noexcept {
        return std::sqrt(x * x + y * y);
    }
    constexpr T length_squared() const noexcept { return x * x + y * y; }

    Vec2 normalized() const noexcept {
        T len = length();
        if (len == T{}) return *this;
        return {x / len, y / len};
    }

    /// Linear interpolation. t in [0,1]; unclamped to allow overshoot tweens.
    constexpr Vec2 lerp(const Vec2& o, T t) const noexcept {
        return {x + (o.x - x) * t, y + (o.y - y) * t};
    }
};

using Vec2f = Vec2<float>;
using Vec2i = Vec2<int>;

template <typename T>
constexpr Vec2<T> operator*(T s, const Vec2<T>& v) noexcept { return v * s; }

} // namespace vengine::math
