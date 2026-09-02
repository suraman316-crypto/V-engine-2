#include <vengine/particles/Particles.hpp>
#include <vengine/core/Assert.hpp>
#include <vengine/math/Random.hpp>

#include <cmath>

namespace vengine::particles {

ParticleSystem::ParticleSystem(const EmitterConfig& cfg)
    : config_(cfg), particles_(cfg.max_particles) {
}

void ParticleSystem::spawn(math::Vec2f origin) {
    // Deterministic xorshift so recorded tests/particle seeds are stable.
    static thread_local math::Rng rng{0xC0FFEEull};

    for (auto& p : particles_) {
        if (p.alive) continue;
        p.alive = true;
        // Emitter shape: point, circle, box, cone.
        switch (config_.shape) {
        case Shape::Point:
            p.position = origin;
            break;
        case Shape::Circle: {
            float ang = rng.range(0.0f, 6.2831853f);
            float r = rng.range(0.0f, 1.0f);
            p.position = origin + math::Vec2f{std::cos(ang), std::sin(ang)} * r;
            break;
        }
        case Shape::Box: {
            p.position = origin + math::Vec2f{rng.range(-0.5f, 0.5f) * config_.size.x,
                                              rng.range(-0.5f, 0.5f) * config_.size.y};
            break;
        }
        case Shape::Cone: {
            p.position = origin;
            break;
        }
        }

        float speed = config_.speed + rng.next_normal(0.0f, 1.0f) * config_.speed_variance;
        speed = std::max(speed, 0.0f);
        float angle = rng.range(-0.5f, 0.5f);
        p.velocity = math::Vec2f{std::sin(angle) * speed, -std::cos(angle) * speed};
        float lt = config_.lifetime + rng.next_normal(0.0f, 1.0f) * config_.lifetime_variance;
        p.lifetime = lt > 0.0f ? lt : 0.01f;
        p.age      = 0.0f;
        p.color    = config_.color_start;
        p.size     = config_.size;
        p.rotation = 0.0f;
        return;
    }
    // Pool exhausted: recycle the oldest particle.
    auto& p = particles_.front();
    p.alive = true; p.position = origin; p.age = 0.0f; p.lifetime = config_.lifetime;
}

void ParticleSystem::emit(math::Vec2f origin, int count) {
    VENGINE_ASSERT(count >= 0, "emit count must be >= 0");
    for (int i = 0; i < count; ++i) spawn(origin);
}

void ParticleSystem::update(float dt) {
    update(dt, math::Vec2f{0.0f, 0.0f});
}

void ParticleSystem::update(float dt, math::Vec2f origin) {
    emission_accum_ += dt * config_.rate;
    int to_emit = static_cast<int>(emission_accum_);
    emission_accum_ -= static_cast<float>(to_emit);
    for (int i = 0; i < to_emit; ++i) spawn(origin);

    for (auto& p : particles_) {
        if (!p.alive) continue;
        p.age += dt;
        if (p.age >= p.lifetime) { p.alive = false; continue; }
        p.velocity += config_.gravity * dt;
        p.position += p.velocity * dt;
        float t = p.age / p.lifetime;
        p.color.r = config_.color_start.r + (config_.color_end.r - config_.color_start.r) * t;
        p.color.g = config_.color_start.g + (config_.color_end.g - config_.color_start.g) * t;
        p.color.b = config_.color_start.b + (config_.color_end.b - config_.color_start.b) * t;
        p.color.a = config_.color_start.a + (config_.color_end.a - config_.color_start.a) * t;
        // size eases toward zero so particles shrink out.
        p.size = config_.size * (1.0f - t);
    }
}

std::size_t ParticleSystem::alive_count() const noexcept {
    std::size_t n = 0;
    for (const auto& p : particles_) if (p.alive) ++n;
    return n;
}

} // namespace vengine::particles
