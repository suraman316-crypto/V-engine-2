#pragma once

// 3D and 4D vectors. Although V Engine is a 2D engine, these types are needed
// by the 4x4 matrices used for camera/view-projection transforms, by the
// particle system (3D velocity noise), and by the asset/shader pipeline
// (vec3/vec4 uniforms). They mirror the Vec2 design: trivially copyable,
// no virtuals, value semantics.

#include <vengine/core/Types.hpp>

#include <cmath>
#include <type_traits>

namespace vengine::math {

template <typename T>
struct Vec3 {
    static_assert(std::is_arithmetic_v<T>, "Vec3 requires an arithmetic type");

    T x{};
    T y{};
    T z{};

    constexpr Vec3() = default;
    constexpr Vec3(T x_, T y_, T z_) noexcept : x(x_), y(y_), z(z_) {}

    constexpr static Vec3 zero() noexcept { return Vec3{0, 0, 0}; }
    constexpr static Vec3 one()  noexcept { return Vec3{1, 1, 1}; }
    constexpr static Vec3 unit_x() noexcept { return Vec3{1, 0, 0}; }
    constexpr static Vec3 unit_y() noexcept { return Vec3{0, 1, 0}; }
    constexpr static Vec3 unit_z() noexcept { return Vec3{0, 0, 1}; }

    constexpr Vec3 operator+(const Vec3& o) const noexcept { return {x + o.x, y + o.y, z + o.z}; }
    constexpr Vec3 operator-(const Vec3& o) const noexcept { return {x - o.x, y - o.y, z - o.z}; }
    constexpr Vec3 operator*(T s) const noexcept { return {x * s, y * s, z * s}; }
    constexpr Vec3 operator/(T s) const noexcept { return {x / s, y / s, z / s}; }
    constexpr Vec3 operator-() const noexcept { return {-x, -y, -z}; }

    constexpr Vec3& operator+=(const Vec3& o) noexcept { x += o.x; y += o.y; z += o.z; return *this; }
    constexpr Vec3& operator-=(const Vec3& o) noexcept { x -= o.x; y -= o.y; z -= o.z; return *this; }
    constexpr Vec3& operator*=(T s) noexcept { x *= s; y *= s; z *= s; return *this; }
    constexpr Vec3& operator/=(T s) noexcept { x /= s; y /= s; z /= s; return *this; }

    constexpr bool operator==(const Vec3& o) const noexcept = default;

    constexpr T dot(const Vec3& o) const noexcept { return x * o.x + y * o.y + z * o.z; }
    constexpr Vec3 cross(const Vec3& o) const noexcept {
        return {y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x};
    }

    T length() const noexcept { return std::sqrt(x * x + y * y + z * z); }
    constexpr T length_squared() const noexcept { return x * x + y * y + z * z; }

    Vec3 normalized() const noexcept {
        T len = length();
        if (len == T{}) return *this;
        return {x / len, y / len, z / len};
    }

    /// Linear interpolation. t in [0,1]; unclamped to allow overshoot tweens.
    constexpr Vec3 lerp(const Vec3& o, T t) const noexcept {
        return {x + (o.x - x) * t, y + (o.y - y) * t, z + (o.z - z) * t};
    }
};

template <typename T>
struct Vec4 {
    static_assert(std::is_arithmetic_v<T>, "Vec4 requires an arithmetic type");

    T x{};
    T y{};
    T z{};
    T w{};

    constexpr Vec4() = default;
    constexpr Vec4(T x_, T y_, T z_, T w_) noexcept : x(x_), y(y_), z(z_), w(w_) {}

    constexpr static Vec4 zero() noexcept { return Vec4{0, 0, 0, 0}; }
    constexpr static Vec4 one()  noexcept { return Vec4{1, 1, 1, 1}; }

    constexpr Vec4 operator+(const Vec4& o) const noexcept { return {x + o.x, y + o.y, z + o.z, w + o.w}; }
    constexpr Vec4 operator-(const Vec4& o) const noexcept { return {x - o.x, y - o.y, z - o.z, w - o.w}; }
    constexpr Vec4 operator*(T s) const noexcept { return {x * s, y * s, z * s, w * s}; }
    constexpr Vec4 operator/(T s) const noexcept { return {x / s, y / s, z / s, w / s}; }

    constexpr Vec4& operator+=(const Vec4& o) noexcept { x += o.x; y += o.y; z += o.z; w += o.w; return *this; }
    constexpr Vec4& operator-=(const Vec4& o) noexcept { x -= o.x; y -= o.y; z -= o.z; w -= o.w; return *this; }
    constexpr Vec4& operator*=(T s) noexcept { x *= s; y *= s; z *= s; w *= s; return *this; }
    constexpr Vec4& operator/=(T s) noexcept { x /= s; y /= s; z /= s; w /= s; return *this; }

    constexpr bool operator==(const Vec4& o) const noexcept = default;

    constexpr T dot(const Vec4& o) const noexcept { return x * o.x + y * o.y + z * o.z + w * o.w; }

    constexpr Vec3<T> xyz() const noexcept { return Vec3<T>{x, y, z}; }
    constexpr T&       r() noexcept { return x; }
    constexpr T&       g() noexcept { return y; }
    constexpr T&       b() noexcept { return z; }
    constexpr T&       a() noexcept { return w; }
    constexpr const T& r() const noexcept { return x; }
    constexpr const T& g() const noexcept { return y; }
    constexpr const T& b() const noexcept { return z; }
    constexpr const T& a() const noexcept { return w; }
};

using Vec3f = Vec3<float>;
using Vec4f = Vec4<float>;

template <typename T>
constexpr Vec3<T> operator*(T s, const Vec3<T>& v) noexcept { return v * s; }
template <typename T>
constexpr Vec4<T> operator*(T s, const Vec4<T>& v) noexcept { return v * s; }

} // namespace vengine::math
