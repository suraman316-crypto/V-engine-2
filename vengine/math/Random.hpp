#pragma once

// Deterministic pseudo-random number generation for the engine.
//
// Games need reproducible randomness: particle seeds, level generation, and
// recorded tests must be stable across runs. mt19937 is a good default; a
// fast xorshift is provided for per-frame use where statistical quality is
// less important than throughput (particle noise).

#include <vengine/core/Types.hpp>

#include <random>

namespace vengine::math {

/// xorshift64*. Very fast, good enough for visual noise; not cryptographic.
class Rng {
public:
    explicit Rng(u64 seed = 0x9E3779B97F4A7C15ull) noexcept : state_(seed ? seed : 1) {}

    void seed(u64 s) noexcept { state_ = s ? s : 1; }

    u64 next_u64() noexcept {
        state_ ^= state_ >> 12;
        state_ ^= state_ << 25;
        state_ ^= state_ >> 27;
        return state_ * 0x2545F4914F6CDD1Dull;
    }
    u32 next_u32() noexcept { return static_cast<u32>(next_u64() >> 32); }

    /// Uniform float in [0, 1).
    float next_float() noexcept {
        // 24 bits of mantissa for a float.
        return static_cast<float>(next_u32() >> 8) * (1.0f / 16777216.0f);
    }

    /// Uniform float in [min, max).
    float range(float min, float max) noexcept {
        return min + (max - min) * next_float();
    }

    /// Uniform int in [min, max] inclusive.
    i32 range_int(i32 min, i32 max) noexcept {
        if (max <= min) return min;
        return min + static_cast<i32>(next_u32() % static_cast<u32>(max - min + 1));
    }

    /// Approx standard-normal via sum of uniforms (Irwin-Hall, n=4).
    float next_normal(float mean = 0.0f, float stddev = 1.0f) noexcept {
        float s = next_float() + next_float() + next_float() + next_float();
        return mean + (s - 2.0f) * stddev;
    }

private:
    u64 state_;
};

/// Standard-library backed generator for cases needing better distribution
/// quality (procedural generation). Slower but statistically robust.
class StdRng {
public:
    explicit StdRng(u64 seed = 0xC0FFEEull) : engine_(static_cast<std::mt19937_64::result_type>(seed)) {}

    void seed(u64 s) { engine_.seed(static_cast<std::mt19937_64::result_type>(s)); }

    float next_float() {
        return std::uniform_real_distribution<float>(0.0f, 1.0f)(engine_);
    }
    float range(float min, float max) {
        return std::uniform_real_distribution<float>(min, max)(engine_);
    }
    i32 range_int(i32 min, i32 max) {
        return std::uniform_int_distribution<i32>(min, max)(engine_);
    }

private:
    std::mt19937_64 engine_;
};

} // namespace vengine::math
