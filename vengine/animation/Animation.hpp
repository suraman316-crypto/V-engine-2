#pragma once

#include <vengine/core/Types.hpp>
#include <vengine/math/Rect.hpp>
#include <vengine/math/Vec2.hpp>

#include <string>
#include <vector>

namespace vengine::animation {

/// One frame of a sprite-sheet animation: UV rect + duration in seconds.
struct Frame {
    math::Rectf uv{0.0f, 0.0f, 1.0f, 1.0f};
    float       duration{0.1f};
};

/// A named clip: ordered frames with looping policy.
struct Clip {
    std::string       name;
    std::vector<Frame> frames;
    bool              loop{true};
    float             speed{1.0f};
};

/// Runtime playback state for a clip on an entity.
struct AnimationState {
    const Clip* clip{nullptr};
    std::size_t frame_index{0};
    float       elapsed{0.0f};
    bool        playing{true};

    void update(float dt) {
        if (!clip || !playing || clip->frames.empty()) return;
        elapsed += dt * clip->speed;
        const float dur = clip->frames[frame_index].duration;
        if (elapsed >= dur) {
            elapsed -= dur;
            ++frame_index;
            if (frame_index >= clip->frames.size()) {
                frame_index = clip->loop ? 0 : clip->frames.size() - 1;
                if (!clip->loop) playing = false;
            }
        }
    }

    const Frame& current_frame() const {
        static const Frame fallback{};
        if (!clip || clip->frames.empty()) return fallback;
        return clip->frames[frame_index];
    }
};

} // namespace vengine::animation
