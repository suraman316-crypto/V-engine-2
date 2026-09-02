#pragma once

// Camera2D: a view-projection builder with follow, dead-zone, shake, and zoom.
//
// The renderer needs a view matrix and a screen-space AABB for culling. This
// struct computes both from a 2D camera description, plus adds gameplay
// conveniences (follow a target, a dead-zone where the camera doesn't move,
// exponential screen-shake) used by the Camera system and the editor.

#include <vengine/math/AABB.hpp>
#include <vengine/math/Mat4.hpp>
#include <vengine/math/Random.hpp>
#include <vengine/math/Vec2.hpp>

#include <cmath>

namespace vengine::renderer {

struct Camera2D {
    math::Vec2f position{0.0f, 0.0f};
    float       zoom{1.0f};
    float       rotation{0.0f};        ///< radians
    math::Vec2f viewport{1280.0f, 720.0f}; ///< virtual resolution

    // follow / dead-zone
    math::Vec2f target{0.0f, 0.0f};
    bool        follow{false};
    float       follow_lerp{8.0f};     ///< higher = snappier
    math::Vec2f dead_zone{0.0f, 0.0f}; ///< half-extents; camera won't move within this of target

    // shake
    float shake_magnitude{0.0f};
    float shake_decay{5.0f};
    float shake_time{0.0f};

    math::Color clear_color{0.1f, 0.1f, 0.12f, 1.0f};

    /// Add a screen shake impulse. Magnitudes stack; decay is exponential.
    void add_shake(float magnitude) {
        shake_magnitude = std::max(shake_magnitude, magnitude);
        shake_time = 1.0f;
    }

    /// Advance the camera (call from the LateUpdate system). dt in seconds.
    void update(float dt, math::Rng& rng) {
        if (follow) {
            if (dead_zone.x > 0.0f || dead_zone.y > 0.0f) {
                math::Vec2f delta = target - position;
                if (std::abs(delta.x) > dead_zone.x) {
                    position.x = target.x - (dead_zone.x) * (delta.x > 0 ? 1.0f : -1.0f);
                }
                if (std::abs(delta.y) > dead_zone.y) {
                    position.y = target.y - (dead_zone.y) * (delta.y > 0 ? 1.0f : -1.0f);
                }
            } else {
                // exponential smoothing: framerate-independent lerp.
                float t = 1.0f - std::exp(-follow_lerp * dt);
                position = position.lerp(target, t);
            }
        }
        if (shake_time > 0.0f) {
            shake_time -= dt;
            if (shake_time <= 0.0f) shake_magnitude = 0.0f;
        }
        (void)rng;
    }

    /// Effective position with shake offset applied. Call once per frame.
    math::Vec2f shaken_position(math::Rng& rng) const {
        if (shake_magnitude <= 0.0f) return position;
        float mag = shake_magnitude * std::exp(-shake_decay * (1.0f - shake_time));
        return position + math::Vec2f{rng.range(-mag, mag), rng.range(-mag, mag)};
    }

    /// Build the view matrix (world -> camera) — inverse of the camera TRS.
    math::Mat4 view_matrix(math::Rng& rng) const {
        math::Vec2f p = shaken_position(rng);
        // view = inverse(translate(p) * rotate(r) * scale(zoom))
        math::Mat4 t = math::Mat4::translate(-p);
        math::Mat4 r = math::Mat4::rotate_z(-rotation);
        math::Mat4 s = math::Mat4::scale(1.0f / zoom);
        return s * r * t;
    }

    /// Orthographic projection matrix mapping the virtual viewport to clip
    /// space, centered on the camera position.
    math::Mat4 projection_matrix() const {
        float hw = viewport.x * 0.5f;
        float hh = viewport.y * 0.5f;
        return math::Mat4::ortho(-hw, hw, -hh, hh, -10.0f, 10.0f);
    }

    /// View-space AABB (in world coordinates) for culling. Centered on the
    /// shaken position, sized by the viewport / zoom.
    math::AABB view_aabb(math::Rng& rng) const {
        math::Vec2f p = shaken_position(rng);
        math::Vec2f half = viewport * (0.5f / zoom);
        return math::AABB{p - half, p + half};
    }

    /// Convert a screen-space point (pixels, origin top-left) to world space.
    math::Vec2f screen_to_world(math::Vec2f screen, math::Rng& rng) const {
        math::Vec2f p = shaken_position(rng);
        math::Vec2f ndc = math::Vec2f{
            (screen.x / viewport.x) * 2.0f - 1.0f,
            1.0f - (screen.y / viewport.y) * 2.0f
        };
        math::Vec2f half = viewport * (0.5f / zoom);
        return p + ndc * half;
    }
};

} // namespace vengine::renderer
