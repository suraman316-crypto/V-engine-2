#pragma once

#include <vengine/core/Types.hpp>
#include <vengine/math/Color.hpp>
#include <vengine/math/Transform2D.hpp>
#include <vengine/math/Vec2.hpp>

#include <vector>

namespace vengine::particles {

/// Emitter shape.
enum class Shape : std::uint8_t { Point, Circle, Box, Cone };

/// Particle emitter configuration (serialized as a ParticleConfig asset).
struct EmitterConfig {
    Shape         shape{Shape::Point};
    float         rate{20.0f};          ///< particles per second
    float         lifetime{1.0f};       ///< seconds
    float         lifetime_variance{0.0f};
    float         speed{100.0f};
    float         speed_variance{0.0f};
    math::Vec2f   size{8.0f, 8.0f};
    math::Color   color_start{1.0f, 1.0f, 1.0f, 1.0f};
    math::Color   color_end{1.0f, 1.0f, 1.0f, 0.0f};
    math::Vec2f   gravity{0.0f, 200.0f};
    std::uint32_t max_particles{512};
    bool          local_space{false};
};

/// A single live particle. Kept POD for cache-friendly simulation.
struct Particle {
    math::Vec2f position{};
    math::Vec2f velocity{};
    math::Color color{};
    float       age{0.0f};
    float       lifetime{1.0f};
    float       rotation{0.0f};
    math::Vec2f size{1.0f, 1.0f};
    bool        alive{false};
};

/// A particle system simulates a pool of particles from one emitter config.
class ParticleSystem {
public:
    explicit ParticleSystem(const EmitterConfig& cfg);

    void emit(math::Vec2f origin, int count);
    void update(float dt);
    /// Update and emit from a moving origin (e.g. an entity's transform).
    void update(float dt, math::Vec2f origin);

    const std::vector<Particle>& particles() const noexcept { return particles_; }
    std::size_t alive_count() const noexcept;

private:
    EmitterConfig          config_;
    std::vector<Particle>  particles_;
    float                  emission_accum_{0.0f};

    void spawn(math::Vec2f origin);
};

} // namespace vengine::particles
