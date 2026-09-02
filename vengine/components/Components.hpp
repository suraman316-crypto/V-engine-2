#pragma once

#include <vengine/core/Types.hpp>
#include <vengine/math/Color.hpp>
#include <vengine/math/Rect.hpp>
#include <vengine/math/Transform2D.hpp>
#include <vengine/math/Vec2.hpp>

#include <string>

namespace vengine::components {

/// Transform carried on entities that have a world position. This is the
/// single source of truth that the renderer reads; physics writes back to it.
struct TransformComponent {
    math::Transform2D value;
};

/// A renderable 2D sprite. References a texture asset by id; UVs default to
/// the full texture. Color modulates the sprite.
struct SpriteRenderer {
    std::string texture_asset;          ///< asset id, resolved by the asset DB
    math::Rectf uv{0.0f, 0.0f, 1.0f, 1.0f};
    math::Color color{1.0f, 1.0f, 1.0f, 1.0f};
    bool        flip_x{false};
    bool        flip_y{false};
    int         layer{0};               ///< sorting layer; higher draws on top
    int         order_in_layer{0};      ///< tie-breaker within a layer
};

/// Camera marks an entity as a camera. The renderer uses the first active
/// camera entity in the scene.
struct Camera {
    float     zoom{1.0f};
    math::Vec2f viewport_size{1280.0f, 720.0f}; ///< virtual resolution
    bool      active{true};
    math::Color clear_color{0.1f, 0.1f, 0.12f, 1.0f};
};

/// Rigidbody2D exposed to game devs. The physics backend (Box2D) syncs to
/// this; the abstraction hides b2Body entirely (README rule: do not expose
/// the external physics library directly).
enum class BodyType { Static, Dynamic, Kinematic };

struct Rigidbody2D {
    BodyType    type{BodyType::Dynamic};
    bool        fixed_rotation{false};
    float       gravity_scale{1.0f};
    float       linear_damping{0.0f};
    float       angular_damping{0.0f};
    math::Vec2f velocity{0.0f, 0.0f};
    float       angular_velocity{0.0f};
    /// Collision group/mask bits. Defaults to "collide with everything".
    std::uint16_t collision_layer{0xFFFF};
    std::uint16_t collision_mask{0xFFFF};
    bool          is_sensor{false};
};

/// Axis-aligned box collider. The physics backend converts this to its own
/// shape representation.
struct BoxCollider {
    math::Vec2f size{1.0f, 1.0f};
    math::Vec2f offset{0.0f, 0.0f};
    bool        is_trigger{false};
};

struct CircleCollider {
    float       radius{0.5f};
    math::Vec2f offset{0.0f, 0.0f};
    bool        is_trigger{false};
};

/// Tag component: a named group label used for queries/scripting.
struct Tag {
    std::string value;
};

} // namespace vengine::components
