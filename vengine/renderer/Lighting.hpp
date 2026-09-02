#pragma once

// V Engine 2.0 — 2D lighting: point lights, directional lights, ambient,
// shadows (blob shadows), and a light list that the lit sprite shader
// samples.
//
// 2D lighting works by tinting sprites with a per-pixel light contribution.
// We support up to 64 dynamic lights per frame (configurable) plus a global
// directional + ambient term. Soft blob shadows are approximated by
// darkening behind occluder AABBs.

#include <vengine/math/AABB.hpp>
#include <vengine/math/Color.hpp>
#include <vengine/math/Vec2.hpp>

#include <cstdint>
#include <vector>

namespace vengine::renderer {

struct PointLight {
    math::Vec2f position{};
    float radius{256.0f};
    float intensity{1.0f};
    math::Color color{1.0f, 1.0f, 1.0f, 1.0f};
    float falloff{2.0f}; ///< 1=linear, 2=quadratic
    bool cast_shadows{false};
};

struct DirectionalLight {
    math::Vec2f direction{0.0f, -1.0f}; ///< normalized
    math::Color color{1.0f, 1.0f, 1.0f, 1.0f};
    float intensity{0.6f};
};

struct AmbientLight {
    math::Color color{0.25f, 0.25f, 0.3f, 1.0f};
    float intensity{0.3f};
};

class LightSystem {
public:
    void clear() { point_lights_.clear(); }

    PointLight& add_point(PointLight l) {
        if (point_lights_.size() < max_lights_) point_lights_.push_back(l);
        else point_lights_[0] = std::move(l); // recycle oldest
        return point_lights_.back();
    }

    /// Compute the contribution of all lights at a world position.
    math::Color sample(const math::Vec2f& pos) const {
        math::Color result = ambient_.color * ambient_.intensity;
        // directional
        result = result + directional_.color * (directional_.intensity
                * std::max(0.0f, -directional_.direction.y * 0.5f + 0.5f));
        // points
        for (const auto& l : point_lights_) {
            const math::Vec2f d = pos - l.position;
            const float dist = d.length();
            if (dist > l.radius) continue;
            const float t = 1.0f - (dist / l.radius);
            const float atten = std::pow(t, l.falloff) * l.intensity;
            result = result + l.color * atten;
        }
        result.r = std::min(result.r, 1.0f);
        result.g = std::min(result.g, 1.0f);
        result.b = std::min(result.b, 1.0f);
        result.a = 1.0f;
        return result;
    }

    /// Blob shadow: returns the shadow factor [0,1] at `pos` given occluders.
    float shadow_factor(const math::Vec2f& pos,
                        const std::vector<math::AABB>& occluders) const {
        float shadow = 0.0f;
        const math::Vec2f light_dir = directional_.direction.normalized();
        for (const auto& o : occluders) {
            // Project the occluder along the light direction; if pos falls in
            // the projected footprint, darken.
            const math::Vec2f c = o.center();
            const math::Vec2f half = o.max - c;
            const float proj_len = (half.x * std::abs(light_dir.x) + half.y * std::abs(light_dir.y)) * 2.0f;
            const math::Vec2f to_pos = pos - c;
            const float along = to_pos.dot(-light_dir);
            const float perp = std::abs(to_pos.dot(math::Vec2f{-light_dir.y, light_dir.x}));
            if (along > 0.0f && along < proj_len + 64.0f && perp < half.length()) {
                shadow = std::max(shadow, 0.5f * (1.0f - along / (proj_len + 64.0f)));
            }
        }
        return shadow;
    }

    std::vector<PointLight>& point_lights() noexcept { return point_lights_; }
    const std::vector<PointLight>& point_lights() const noexcept { return point_lights_; }
    DirectionalLight& directional() noexcept { return directional_; }
    AmbientLight& ambient() noexcept { return ambient_; }
    std::size_t max_lights() const noexcept { return max_lights_; }
    void set_max_lights(std::size_t n) noexcept { max_lights_ = n; }

private:
    std::vector<PointLight> point_lights_;
    DirectionalLight directional_{};
    AmbientLight ambient_{};
    std::size_t max_lights_{64};
};

} // namespace vengine::renderer
