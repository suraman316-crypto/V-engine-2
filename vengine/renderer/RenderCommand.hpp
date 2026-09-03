#pragma once

// Render command buffer + sprite batcher.
//
// The renderer collects draw requests into a command buffer each frame, sorts
// them (layer, then order, then texture to minimize binds), and batches quads
// that share a texture into single draw calls. This module is the
// platform-independent scheduling layer; it does NOT touch OpenGL/Vulkan. A
// concrete backend (GLES) consumes the baked batches and uploads them.
//
// Why a command buffer:
//   - Lets gameplay/systems run on any thread while the GPU thread replays.
//   - Makes sorting and batching deterministic and testable without a GPU.
//   - Keeps the backend thin: it only knows how to draw a Batch.

#include <vengine/Common.hpp>
#include <vengine/math/AABB.hpp>
#include <vengine/math/Color.hpp>
#include <vengine/math/Mat4.hpp>
#include <vengine/math/Rect.hpp>
#include <vengine/math/Transform2D.hpp>
#include <vengine/math/Vec2.hpp>

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace vengine::renderer {

/// Opaque texture id. 0 == invalid/no texture (used for solid-color shapes).
using TextureId = std::uint32_t;

/// A single quad sprite request submitted to the command buffer.
struct SpriteCommand {
    math::Transform2D transform;          ///< world transform
    TextureId         texture{0};          ///< 0 = untextured quad
    math::Rectf       uv{0.0f, 0.0f, 1.0f, 1.0f};
    math::Color       color{1.0f, 1.0f, 1.0f, 1.0f};
    int               layer{0};            ///< sorting layer
    int               order_in_layer{0};   ///< tie-breaker
    bool              flip_x{false};
    bool              flip_y{false};
};

/// Shape primitive draws (lines, circles, rectangles, polygons) used by the
/// debug renderer and shape-based UI.
struct ShapeCommand {
    enum class Kind : u8 { Line, Circle, Rect, Polygon };
    Kind         kind{Kind::Rect};
    math::Vec2f  a{};
    math::Vec2f  b{};
    float        radius{0.0f};
    std::vector<math::Vec2f> points; ///< polygon / line strip
    math::Color  color{1.0f, 1.0f, 1.0f, 1.0f};
    float        line_width{1.0f};
    bool         filled{true};
    int          layer{0};
};

/// A baked batch of quads that share a texture and can be drawn in one call.
struct SpriteBatch {
    TextureId texture{0};
    /// Packed vertex data (x,y,u,v,r,g,b,a) per quad, 8 floats each.
    std::vector<float> vertices;
    std::size_t        quad_count{0};
    int                layer{0};
};

/// Sort key: layer packs into the high bits so sorting by this single u64
/// yields layer -> order -> texture ordering.
inline std::uint64_t sort_key(const SpriteCommand& s) noexcept {
    // layer signed -> bias to unsigned 24 bits.
    std::uint64_t l = static_cast<std::uint64_t>(static_cast<std::int64_t>(s.layer) + 0x800000);
    std::uint64_t o = static_cast<std::uint64_t>(static_cast<std::uint32_t>(s.order_in_layer));
    std::uint64_t t = s.texture;
    return (l << 40) | ((o & 0xFFFFF) << 20) | (t & 0xFFFFF);
}

/// Command buffer: collects sprite + shape commands for one frame, then bakes
/// them into sorted batches the backend consumes. No GPU calls.
class RenderCommandBuffer {
public:
    void clear() noexcept {
        sprites_.clear();
        shapes_.clear();
    }

    void submit(const SpriteCommand& s) { sprites_.push_back(s); }
    void submit(SpriteCommand&& s) { sprites_.push_back(std::move(s)); }
    void submit(const ShapeCommand& s) { shapes_.push_back(s); }

    std::size_t sprite_count() const noexcept { return sprites_.size(); }
    std::size_t shape_count() const noexcept  { return shapes_.size(); }

    const std::vector<SpriteCommand>& sprites() const noexcept { return sprites_; }
    const std::vector<ShapeCommand>&  shapes() const noexcept  { return shapes_; }

    /// Cull sprites whose world AABB does not intersect the camera view AABB.
    /// Returns the number removed. Used before baking to reduce work.
    std::size_t cull(const math::AABB& view) {
        std::size_t before = sprites_.size();
        std::erase_if(sprites_, [&](const SpriteCommand& s) {
            math::Vec2f half = s.transform.scale * 0.5f;
            math::AABB box{s.transform.position - half, s.transform.position + half};
            return !box.intersects(view);
        });
        return before - sprites_.size();
    }

    /// Sort sprites by (layer, order, texture) and bake adjacent same-texture
    /// quads into SpriteBatches. Output is appended to `out`. Returns the
    /// total number of batches.
    std::size_t bake(std::vector<SpriteBatch>& out) const {
        std::vector<const SpriteCommand*> sorted;
        sorted.reserve(sprites_.size());
        for (const auto& s : sprites_) sorted.push_back(&s);
        std::sort(sorted.begin(), sorted.end(),
            [](const SpriteCommand* x, const SpriteCommand* y) {
                return sort_key(*x) < sort_key(*y);
            });

        out.clear();
        SpriteBatch* current = nullptr;
        for (const auto* s : sorted) {
            if (!current || current->texture != s->texture || current->layer != s->layer) {
                out.push_back(SpriteBatch{s->texture, {}, 0, s->layer});
                current = &out.back();
            }
            append_quad(*current, *s);
        }
        return out.size();
    }

    /// Rebuild a single sprite's vertex data (8 floats: x,y,u,v,r,g,b,a) at the
    /// quad's local corner. Centered quad in [-0.5, 0.5] scaled by transform.
    static void append_quad(SpriteBatch& batch, const SpriteCommand& s) {
        const float hw = 0.5f, hh = 0.5f; // unit quad; backend scales by transform
        float u0 = s.uv.x, u1 = s.uv.x + s.uv.width();
        float v0 = s.uv.y, v1 = s.uv.y + s.uv.height();
        if (s.flip_x) std::swap(u0, u1);
        if (s.flip_y) std::swap(v0, v1);
        const float r = s.color.r, g = s.color.g, b = s.color.b, a = s.color.a;
        // 4 corners, 8 floats each.
        float corners[4][8] = {
            {-hw, -hh, u0, v0, r, g, b, a},
            { hw, -hh, u1, v0, r, g, b, a},
            { hw,  hh, u1, v1, r, g, b, a},
            {-hw,  hh, u0, v1, r, g, b, a},
        };
        for (auto& c : corners) {
            for (float f : c) batch.vertices.push_back(f);
        }
        batch.quad_count++;
    }

private:
    std::vector<SpriteCommand> sprites_;
    std::vector<ShapeCommand>  shapes_;
};

} // namespace vengine::renderer
