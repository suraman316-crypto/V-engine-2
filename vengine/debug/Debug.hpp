#pragma once

#include <vengine/core/Types.hpp>
#include <vengine/math/Color.hpp>
#include <vengine/math/Vec2.hpp>

#include <string>
#include <vector>

namespace vengine::debug {

/// Debug draw overlay commands. The editor uses these to visualize colliders,
/// origins, raycasts, etc. They are collected per frame and rendered on top.
enum class DrawKind { Line, Rect, Circle, Text };

struct DebugDraw {
    DrawKind     kind{DrawKind::Line};
    math::Vec2f  a{};
    math::Vec2f  b{};
    float        radius{0.0f};
    math::Color  color{1.0f, 1.0f, 1.0f, 1.0f};
    std::string  text;
};

/// Accumulator for debug draws; cleared each frame.
class DebugDrawList {
public:
    void clear() noexcept { items_.clear(); }
    void line(math::Vec2f a, math::Vec2f b, math::Color c) {
        items_.push_back({DrawKind::Line, a, b, 0.0f, c, {}});
    }
    void rect(math::Vec2f a, math::Vec2f b, math::Color c) {
        items_.push_back({DrawKind::Rect, a, b, 0.0f, c, {}});
    }
    void circle(math::Vec2f center, float r, math::Color c) {
        items_.push_back({DrawKind::Circle, center, {}, r, c, {}});
    }
    void text(math::Vec2f pos, std::string t, math::Color c) {
        items_.push_back({DrawKind::Text, pos, {}, 0.0f, c, std::move(t)});
    }

    const std::vector<DebugDraw>& items() const noexcept { return items_; }

private:
    std::vector<DebugDraw> items_;
};

} // namespace vengine::debug
