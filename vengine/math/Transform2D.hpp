#pragma once

#include <vengine/math/Vec2.hpp>
#include <vengine/core/Types.hpp>
#include <cmath>

namespace vengine::math {

/// 2D transform: position, rotation (radians), uniform-or-axial scale.
/// This is the value the SpriteRenderer reads; physics syncs to it.
struct Transform2D {
    Vec2f position{0.0f, 0.0f};
    float rotation{0.0f};   // radians
    Vec2f scale{1.0f, 1.0f};

    constexpr Transform2D() = default;
    constexpr Transform2D(Vec2f pos, float rot = 0.0f, Vec2f scl = {1.0f, 1.0f}) noexcept
        : position(pos), rotation(rot), scale(scl) {}

    /// Compose this transform over a parent (parent * child), used for
    /// hierarchical entities. Avoids per-frame allocation.
    Transform2D compose(const Transform2D& parent) const noexcept {
        const float c = std::cos(parent.rotation);
        const float s = std::sin(parent.rotation);
        Vec2f rotated{
            parent.position.x + (position.x * c - position.y * s) * parent.scale.x,
            parent.position.y + (position.x * s + position.y * c) * parent.scale.y,
        };
        return Transform2D{
            rotated,
            parent.rotation + rotation,
            Vec2f{parent.scale.x * scale.x, parent.scale.y * scale.y}
        };
    }
};

} // namespace vengine::math
