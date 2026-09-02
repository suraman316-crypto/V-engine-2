#pragma once

#include <vengine/core/Types.hpp>
#include <vengine/math/Color.hpp>
#include <vengine/math/Rect.hpp>
#include <vengine/math/Transform2D.hpp>
#include <vengine/math/Vec2.hpp>
#include <vengine/scene/Registry.hpp>

#include <string>
#include <vector>

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

// ---------------------------------------------------------------------------
// Hierarchy
// ---------------------------------------------------------------------------

/// Parent/child relationship. Entities carry a `parent` entity handle (null if
/// root) and a list of direct children. The transform system composes children
/// over parents each frame. Stored as a component so hierarchy is data-driven
/// and serializable.
struct Hierarchy {
    scene::Entity parent{};
    std::vector<scene::Entity> children;
    bool dirty{true}; ///< recompute world transforms when true
};

// ---------------------------------------------------------------------------
// Animation
// ---------------------------------------------------------------------------

/// A single frame of a sprite-sheet animation: UV rect in atlas coordinates +
/// duration in seconds.
struct AnimFrame {
    math::Rectf uv{0.0f, 0.0f, 1.0f, 1.0f};
    float       duration{0.1f};
};

/// A named clip: ordered list of frames. Loop wraps; ping-pong reverses.
struct AnimClip {
    std::string            name;
    std::vector<AnimFrame> frames;
    bool                   loop{true};
    bool                   ping_pong{false};
};

/// Animator drives the SpriteRenderer's uv rect from a playing clip.
struct Animator {
    std::vector<AnimClip> clips;
    std::size_t           current_clip{0};
    std::size_t           current_frame{0};
    float                 time_accum{0.0f};
    bool                  playing{false};
    float                 speed{1.0f};
    int                   ping_dir{1};

    /// Find a clip by name; returns nullptr if absent.
    const AnimClip* find(std::string_view n) const {
        for (const auto& c : clips) if (c.name == n) return &c;
        return nullptr;
    }
};

// ---------------------------------------------------------------------------
// Text
// ---------------------------------------------------------------------------

/// A renderable text label. References a font asset by id; the renderer
/// rasterizes/quads the glyphs.
struct TextLabel {
    std::string  font_asset;
    std::string  text;
    math::Color  color{1.0f, 1.0f, 1.0f, 1.0f};
    float        font_size{16.0f};
    bool         word_wrap{false};
    int          layer{0};
    int          order_in_layer{0};
    enum class Anchor : u8 { TopLeft, Center, BottomLeft } anchor{Anchor::TopLeft};
};

// ---------------------------------------------------------------------------
// Particles
// ---------------------------------------------------------------------------

/// Particle emitter configuration. The particle system (cpu sim) reads this
/// and spawns particles from the entity's transform.
struct ParticleEmitter {
    std::string texture_asset;          ///< sprite used per particle
    float       rate{30.0f};            ///< emissions per second
    float       lifetime{2.0f};         ///< particle lifetime (seconds)
    float       spawn_radius{0.0f};    ///< circular spawn area radius
    float       start_speed{50.0f};
    float       start_size{4.0f};
    float       end_size{1.0f};
    math::Color start_color{1.0f, 1.0f, 1.0f, 1.0f};
    math::Color end_color{0.0f, 0.0f, 0.0f, 0.0f};
    math::Vec2f gravity{0.0f, -50.0f};
    float       spread_radians{6.2831853f}; ///< full circle by default
    float       direction_radians{0.0f};    ///< center of the spread cone
    int         max_particles{500};
    bool        emit{true};
    float       emit_accum{0.0f};          ///< runtime: fractional emission
};

// ---------------------------------------------------------------------------
// Audio source/listener
// ---------------------------------------------------------------------------

/// Plays an audio asset. The audio system resolves the asset id and routes
/// through the requested bus.
struct AudioSource {
    std::string asset;
    enum class Bus : u8 { Master, SFX, Music, UI } bus{Bus::SFX};
    float       volume{1.0f};
    float       pitch{1.0f};
    bool        loop{false};
    bool        spatial{false};      ///< 2D positional audio
    float       min_distance{100.0f};
    float       max_distance{800.0f};
    bool        playing{false};
};

/// Marks the active listener for spatialized 2D audio (the camera entity).
struct AudioListener {};

// ---------------------------------------------------------------------------
// Naming helper
// ---------------------------------------------------------------------------

/// Name component stored separately from Registry metadata so it survives
/// serialization round-trips with rich text.
struct Name {
    std::string value;
};

} // namespace vengine::components
