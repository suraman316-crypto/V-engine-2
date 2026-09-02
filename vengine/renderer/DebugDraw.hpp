#pragma once

// Debug draw: immediate-mode overlay lines/circles/boxes/text for the editor,
// physics visualization, and gameplay gizmos.
//
// The debug renderer accumulates primitives each frame and hands them to the
// renderer's RenderCommandBuffer as ShapeCommands. It is intentionally
// allocation-light: a fixed-capacity ring is reused across frames. Toggling
// debug_draw() on/off gates whether primitives are emitted at all.

#include <vengine/math/Color.hpp>
#include <vengine/math/Vec2.hpp>
#include <vengine/renderer/RenderCommand.hpp>

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace vengine::renderer {

class DebugDraw {
public:
    void begin_frame() { lines_.clear(); circles_.clear(); boxes_.clear(); texts_.clear(); }

    void enabled(bool e) noexcept { enabled_ = e; }
    bool enabled() const noexcept { return enabled_; }

    void line(math::Vec2f a, math::Vec2f b, math::Color c = {1,1,1,1}, int layer = 1000) {
        if (!enabled_) return;
        lines_.push_back({a, b, c, layer});
    }
    void ray(math::Vec2f origin, math::Vec2f dir, float len,
             math::Color c = {1,1,1,1}, int layer = 1000) {
        line(origin, origin + dir * len, c, layer);
    }
    void circle(math::Vec2f center, float radius, math::Color c = {1,1,1,1}, int layer = 1000) {
        if (!enabled_) return;
        circles_.push_back({center, radius, c, layer});
    }
    void box(math::Vec2f center, math::Vec2f half, math::Color c = {1,1,1,1}, int layer = 1000) {
        if (!enabled_) return;
        boxes_.push_back({center, half, c, layer});
    }
    void text(math::Vec2f pos, std::string_view s, math::Color c = {1,1,1,1}, int layer = 1000) {
        if (!enabled_) return;
        texts_.push_back({pos, std::string{s}, c, layer});
    }

    /// Bake accumulated primitives into the command buffer's shape list.
    void submit(RenderCommandBuffer& buf) const {
        if (!enabled_) return;
        for (const auto& l : lines_) {
            ShapeCommand s; s.kind = ShapeCommand::Kind::Line;
            s.a = l.a; s.b = l.b; s.color = l.c; s.layer = l.layer;
            buf.submit(s);
        }
        for (const auto& c : circles_) {
            ShapeCommand s; s.kind = ShapeCommand::Kind::Circle;
            s.a = c.center; s.radius = c.radius; s.color = c.c; s.layer = c.layer;
            // Approximate circle as 16-gon polygon for the backend's polygon path.
            for (int i = 0; i < 16; ++i) {
                float a0 = (i / 16.0f) * 6.2831853f;
                s.points.push_back(c.center + math::Vec2f{std::cos(a0), std::sin(a0)} * c.radius);
            }
            buf.submit(s);
        }
        for (const auto& b : boxes_) {
            ShapeCommand s; s.kind = ShapeCommand::Kind::Rect;
            s.a = b.center - b.half; s.b = b.center + b.half; s.color = b.c; s.layer = b.layer;
            s.filled = false;
            buf.submit(s);
        }
    }

    std::size_t primitive_count() const noexcept {
        return lines_.size() + circles_.size() + boxes_.size();
    }

private:
    struct L { math::Vec2f a, b; math::Color c; int layer; };
    struct C { math::Vec2f center; float radius; math::Color c; int layer; };
    struct B { math::Vec2f center, half; math::Color c; int layer; };
    struct T { math::Vec2f pos; std::string text; math::Color c; int layer; };

    std::vector<L> lines_;
    std::vector<C> circles_;
    std::vector<B> boxes_;
    std::vector<T> texts_;
    bool enabled_{true};
};

} // namespace vengine::renderer
