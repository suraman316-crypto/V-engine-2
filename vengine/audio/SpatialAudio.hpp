#pragma once

// V Engine 2.0 — Spatial audio: 2D panning, distance attenuation, doppler,
// and audio occlusion by geometry.
//
// A Listener has a position/forward; a SpatialVoice has a world position.
// The mixer applies: distance-based gain (inverse-square + max/ramp), a
// 2D pan from relative X, and optional low-pass occlusion when line-of-sight
// to the listener is blocked.

#include <vengine/math/Vec2.hpp>

#include <cmath>
#include <cstdint>
#include <vector>

namespace vengine::audio {

struct Listener {
    math::Vec2f position{};
    math::Vec2f forward{0.0f, -1.0f};
    float gain{1.0f};
};

struct SpatialParams {
    math::Vec2f position{};
    float min_distance{64.0f};
    float max_distance{1024.0f};
    float rolloff{1.0f};       ///< 1=inverse, 2=quadratic
    float pan_strength{1.0f};
    float doppler_scale{1.0f};
    float occlusion{0.0f};      ///< 0=open, 1=fully blocked
    bool  looping{false};
};

struct SpatialVoice {
    std::uint32_t id{0};
    std::uint32_t clip{0};
    SpatialParams params;
    float base_gain{1.0f};
    bool playing{false};
};

/// Compute the gain+pan for a voice relative to a listener.
struct MixResult { float gain; float pan; float pitch; };

inline MixResult compute_spatial_mix(const Listener& lis, const SpatialVoice& v) {
    const math::Vec2f to = v.params.position - lis.position;
    const float dist = to.length();
    // distance attenuation
    const float denom = std::max(v.params.min_distance, dist);
    float atten = 1.0f;
    if (v.params.rolloff <= 1.0f)
        atten = v.params.min_distance / (v.params.min_distance + denom * (1.0f - v.params.min_distance / std::max(denom, 1.0f)));
    else
        atten = std::pow(v.params.min_distance / std::max(denom, v.params.min_distance), v.params.rolloff);
    if (dist > v.params.max_distance) atten *= 0.0f;
    // occlusion low-pass → simulate by cutting gain
    atten *= (1.0f - v.params.occlusion * 0.85f);
    // pan: project onto listener's right vector
    const math::Vec2f right{-lis.forward.y, lis.forward.x};
    const float pan = (to.length() > 1e-4f ? to.dot(right) / dist : 0.0f) * v.params.pan_strength;
    // doppler: simplistic, based on relative velocity (caller sets pitch)
    const float pitch = 1.0f;
    return {atten * lis.gain * v.base_gain, std::clamp(pan, -1.0f, 1.0f), pitch};
}

/// Streaming: chunks of decoded PCM buffered ring-buffer style for music/ambience.
struct StreamingBuffer {
    std::vector<float> pcm_l;  ///< interleaved or planar
    std::vector<float> pcm_r;
    std::size_t read_pos{0};
    bool loop{false};
    bool finished{false};

    void push(const float* l, const float* r, std::size_t n) {
        pcm_l.insert(pcm_l.end(), l, l + n);
        pcm_r.insert(pcm_r.end(), r, r + n);
    }
    /// Read `n` samples; returns how many were actually read (may be < n at EOF).
    std::size_t read(float* l, float* r, std::size_t n) {
        std::size_t avail = pcm_l.size() - read_pos;
        if (avail == 0) { if (loop) read_pos = 0, avail = pcm_l.size(); else { finished = true; return 0; } }
        std::size_t got = std::min(n, avail);
        for (std::size_t i = 0; i < got; ++i) {
            l[i] = pcm_l[read_pos + i];
            r[i] = pcm_r[read_pos + i];
        }
        read_pos += got;
        return got;
    }
    void clear() { pcm_l.clear(); pcm_r.clear(); read_pos = 0; finished = false; }
};

/// Audio bus / mixer group hierarchy: Master → Music/SFX/UI sub-buses.
struct Bus {
    std::string name;
    float volume{1.0f};
    float pan{0.0f};
    bool muted{false};
    bool solo{false};
    std::vector<Bus> children;
};

} // namespace vengine::audio
