#pragma once

#include <vengine/core/Assert.hpp>
#include <vengine/core/Types.hpp>
#include <vengine/math/Vec2.hpp>

namespace vengine::math {

/// Axis-aligned rectangle using a position + size.
template <typename T>
struct Rect {
    T x{}, y{}, w{}, h{};

    constexpr Rect() = default;
    constexpr Rect(T x_, T y_, T w_, T h_) noexcept : x(x_), y(y_), w(w_), h(h_) {}

    constexpr T left()   const noexcept { return x; }
    constexpr T right()  const noexcept { return x + w; }
    constexpr T top()    const noexcept { return y; }
    constexpr T bottom() const noexcept { return y + h; }

    constexpr Vec2<T> center() const noexcept { return Vec2<T>{x + w / 2, y + h / 2}; }
    constexpr Vec2<T> size()   const noexcept { return Vec2<T>{w, h}; }

    constexpr bool contains(T px, T py) const noexcept {
        return px >= x && px < x + w && py >= y && py < y + h;
    }
    constexpr bool contains(const Vec2<T>& p) const noexcept { return contains(p.x, p.y); }

    constexpr bool intersects(const Rect& o) const noexcept {
        return !(o.x >= x + w || x >= o.x + o.w || o.y >= y + h || y >= o.y + o.h);
    }
};

using Rectf = Rect<float>;
using Recti = Rect<int>;

} // namespace vengine::math
