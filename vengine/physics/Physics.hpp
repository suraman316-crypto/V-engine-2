#pragma once

#include <vengine/Common.hpp>
#include <vengine/components/Components.hpp>
#include <vengine/math/Vec2.hpp>
#include <vengine/scene/Registry.hpp>

#include <functional>
#include <memory>

namespace vengine::physics {

/// Collision result passed to callbacks.
struct Collision {
    scene::Entity entity_a{};
    scene::Entity entity_b{};
    bool          is_trigger{false};
};

/// Raycast hit info.
struct RaycastHit {
    scene::Entity entity{};
    math::Vec2f   point{};
    math::Vec2f   normal{};
    float         fraction{1.0f}; ///< 0..1 along the ray
};

/// World-space ray for raycasts.
struct Ray {
    math::Vec2f origin{};
    math::Vec2f direction{}; ///< expected normalized
    float       max_distance{1000.0f};
};

/// Abstract 2D physics world. The engine wraps Box2D behind this interface so
/// game code never sees b2World/b2Body directly (README rule).
class IPhysicsWorld {
public:
    virtual ~IPhysicsWorld() = default;

    virtual Result<void> initialize(math::Vec2f gravity) = 0;
    virtual void         shutdown() = 0;

    /// Step the simulation. dt in seconds; fixed sub-stepping recommended.
    virtual void         step(float dt, int velocity_iterations, int position_iterations) = 0;

    /// Create/destroy a body for an entity, driven by a Rigidbody2D + collider.
    virtual Result<void> create_body(scene::Entity e, const components::Rigidbody2D& rb,
                                     const components::BoxCollider* box,
                                     const components::CircleCollider* circle) = 0;
    virtual void         destroy_body(scene::Entity e) = 0;

    /// Sync the simulation's transforms back into the given Transform2D.
    virtual void         readback_transform(scene::Entity e, math::Transform2D& out) const = 0;
    /// Push a transform into the simulation (for kinematic/manual moves).
    virtual void         write_transform(scene::Entity e, const math::Transform2D& t) = 0;

    virtual Result<RaycastHit> raycast(const Ray& ray) const = 0;

    using CollisionCallback = std::function<void(const Collision&)>;
    virtual void on_collision(CollisionCallback cb) = 0;
    virtual void on_trigger(CollisionCallback cb) = 0;
};

} // namespace vengine::physics
