#pragma once

// Axis-aligned bounding box in 2D. The renderer uses AABBs for culling and
// the physics system uses them for broad-phase collision detection and
// raycasts. The type is trivially copyable and branch-free where it matters.

#include <vengine/math/Vec2.hpp>

#include <algorithm>

namespace vengine::math {

struct AABB {
    Vec2f min{0.0f, 0.0f};
    Vec2f max{0.0f, 0.0f};

    constexpr AABB() = default;
    constexpr AABB(Vec2f mn, Vec2f mx) noexcept : min(mn), max(mx) {}
    constexpr AABB(float x, float y, float w, float h) noexcept
        : min(x, y), max(x + w, y + h) {}

    constexpr Vec2f size()     const noexcept { return max - min; }
    constexpr Vec2f center()   const noexcept { return (min + max) * 0.5f; }
    constexpr float width()    const noexcept { return max.x - min.x; }
    constexpr float height()   const noexcept { return max.y - min.y; }
    constexpr float half_w()   const noexcept { return (max.x - min.x) * 0.5f; }
    constexpr float half_h()   const noexcept { return (max.y - min.y) * 0.5f; }

    constexpr bool contains(Vec2f p) const noexcept {
        return p.x >= min.x && p.x <= max.x && p.y >= min.y && p.y <= max.y;
    }

    constexpr bool intersects(const AABB& o) const noexcept {
        return min.x <= o.max.x && max.x >= o.min.x &&
               min.y <= o.max.y && max.y >= o.min.y;
    }

    /// Smallest AABB containing both this and `o`.
    constexpr AABB merged(const AABB& o) const noexcept {
        return AABB{Vec2f{std::min(min.x, o.min.x), std::min(min.y, o.min.y)},
                    Vec2f{std::max(max.x, o.max.x), std::max(max.y, o.max.y)}};
    }

    /// Expand the box so it contains the point.
    constexpr void expand(Vec2f p) noexcept {
        if (p.x < min.x) min.x = p.x;
        if (p.y < min.y) min.y = p.y;
        if (p.x > max.x) max.x = p.x;
        if (p.y > max.y) max.y = p.y;
    }

    /// Closest point on the box to `p`; inside the box returns `p`.
    constexpr Vec2f closest_point(Vec2f p) const noexcept {
        return Vec2f{std::clamp(p.x, min.x, max.x), std::clamp(p.y, min.y, max.y)};
    }
};

} // namespace vengine::math
