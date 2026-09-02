#include <vengine/particles/Particles.hpp>
#include <vengine/core/Assert.hpp>

#include <cmath>
#include <random>

namespace vengine::particles {

ParticleSystem::ParticleSystem(const EmitterConfig& cfg)
    : config_(cfg), particles_(cfg.max_particles) {
}

void ParticleSystem::spawn(math::Vec2f origin) {
    static thread_local std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<float> jitter(-1.0f, 1.0f);

    for (auto& p : particles_) {
        if (p.alive) continue;
        p.alive    = true;
        p.position = origin;
        float speed = config_.speed + jitter(rng) * config_.speed_variance;
        // Default direction: upward cone within +/- 30 degrees.
        float angle = jitter(rng) * 0.5f; // radians-ish
        p.velocity = math::Vec2f{std::sin(angle) * speed, -std::cos(angle) * speed};
        float lt = config_.lifetime + jitter(rng) * config_.lifetime_variance;
        p.lifetime = lt > 0.0f ? lt : 0.01f;
        p.age      = 0.0f;
        p.color    = config_.color_start;
        p.size     = config_.size;
        p.rotation = 0.0f;
        return;
    }
    // Pool exhausted: oldest particle is recycled.
    auto& p = particles_.front();
    p.alive = true; p.position = origin; p.age = 0.0f; p.lifetime = config_.lifetime;
}

void ParticleSystem::emit(math::Vec2f origin, int count) {
    VENGINE_ASSERT(count >= 0, "emit count must be >= 0");
    for (int i = 0; i < count; ++i) spawn(origin);
}

void ParticleSystem::update(float dt) {
    emission_accum_ += dt * config_.rate;
    int to_emit = static_cast<int>(emission_accum_);
    emission_accum_ -= static_cast<float>(to_emit);
    for (int i = 0; i < to_emit; ++i) spawn({0.0f, 0.0f});

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
    }
}

std::size_t ParticleSystem::alive_count() const noexcept {
    std::size_t n = 0;
    for (const auto& p : particles_) if (p.alive) ++n;
    return n;
}

} // namespace vengine::particles
