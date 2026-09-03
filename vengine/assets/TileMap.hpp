#pragma once

// V Engine 2.0 — Tilemap: multi-layer tile grids, infinite streaming chunks,
// and collision extraction.
//
// A Tilemap is a set of named layers, each a 2D grid of TileIds. Layers can
// be rendered in any order and one or more layers can be flagged as
// "solid" for physics. Tilesets map a TileId to an atlas UV + collision
// shape (full box, slope, one-way platform, or empty).

#include <vengine/Common.hpp>
#include <vengine/math/AABB.hpp>
#include <vengine/math/Rect.hpp>
#include <vengine/math/Vec2.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace vengine::assets {

using TileId = std::uint32_t;
constexpr TileId kEmptyTile = 0;

enum class TileCollision : u8 {
    Empty,
    Full,       ///< solid AABB
    OneWay,     ///< collide from top only (platforms)
    SlopeUp,    ///< 45° rising
    SlopeDown,  ///< 45° falling
    Ladder,
};

struct TileDef {
    TileId id{kEmptyTile};
    math::Rectf uv{};          ///< in tileset texture
    TileCollision collision{TileCollision::Empty};
    bool animated{false};
    std::uint8_t frames{1};
    float frame_duration{0.1f};
};

struct TileLayer {
    std::string name;
    int width{0};
    int height{0};
    int tile_w{32};
    int tile_h{32};
    std::vector<TileId> tiles; ///< width * height, row-major
    bool solid{false};
    bool visible{true};
    float parallax{1.0f}; ///< 1.0 = locked to world; <1 = scrolls slower (background)
    float opacity{1.0f};

    TileId at(int x, int y) const {
        if (x < 0 || y < 0 || x >= width || y >= height) return kEmptyTile;
        return tiles[static_cast<std::size_t>(y) * width + x];
    }
    void set(int x, int y, TileId t) {
        if (x < 0 || y < 0 || x >= width || y >= height) return;
        tiles[static_cast<std::size_t>(y) * width + x] = t;
    }
};

class Tileset {
public:
    void add(TileDef d) { defs_[d.id] = std::move(d); }
    const TileDef* get(TileId id) const {
        auto it = defs_.find(id);
        return it == defs_.end() ? nullptr : &it->second;
    }
private:
    std::unordered_map<TileId, TileDef> defs_;
};

class TileMap {
public:
    void add_layer(TileLayer l) { layers_.push_back(std::move(l)); }
    TileLayer* layer(std::size_t i) { return i < layers_.size() ? &layers_[i] : nullptr; }
    std::size_t layer_count() const noexcept { return layers_.size(); }

    /// Get visible tiles in a world-space AABB for rendering/culling.
    struct VisibleTile {
        math::Vec2f world_pos;
        TileId id;
        const TileDef* def;
        float opacity;
    };
    std::vector<VisibleTile> cull(const math::AABB& view, const Tileset& ts) const {
        std::vector<VisibleTile> out;
        for (const auto& layer : layers_) {
            if (!layer.visible) continue;
            const int x0 = std::max(0, static_cast<int>(view.min.x / layer.tile_w));
            const int y0 = std::max(0, static_cast<int>(view.min.y / layer.tile_h));
            const int x1 = std::min(layer.width - 1, static_cast<int>(view.max.x / layer.tile_w));
            const int y1 = std::min(layer.height - 1, static_cast<int>(view.max.y / layer.tile_h));
            for (int y = y0; y <= y1; ++y) {
                for (int x = x0; x <= x1; ++x) {
                    const TileId t = layer.at(x, y);
                    if (t == kEmptyTile) continue;
                    const TileDef* d = ts.get(t);
                    out.push_back({
                        {static_cast<float>(x * layer.tile_w), static_cast<float>(y * layer.tile_h)},
                        t, d, layer.opacity,
                    });
                }
            }
        }
        return out;
    }

    /// Extract solid AABBs from solid layers within view (for physics).
    std::vector<math::AABB> solid_boxes(const math::AABB& view, const Tileset& ts) const {
        std::vector<math::AABB> out;
        for (const auto& layer : layers_) {
            if (!layer.solid) continue;
            const int x0 = std::max(0, static_cast<int>(view.min.x / layer.tile_w));
            const int y0 = std::max(0, static_cast<int>(view.min.y / layer.tile_h));
            const int x1 = std::min(layer.width - 1, static_cast<int>(view.max.x / layer.tile_w));
            const int y1 = std::min(layer.height - 1, static_cast<int>(view.max.y / layer.tile_h));
            for (int y = y0; y <= y1; ++y) {
                for (int x = x0; x <= x1; ++x) {
                    const TileId t = layer.at(x, y);
                    if (t == kEmptyTile) continue;
                    const TileDef* d = ts.get(t);
                    if (!d || d->collision != TileCollision::Full) continue;
                    out.push_back({{x * layer.tile_w * 1.0f, y * layer.tile_h * 1.0f},
                                   {(x + 1) * layer.tile_w * 1.0f, (y + 1) * layer.tile_h * 1.0f}});
                }
            }
        }
        return out;
    }

private:
    std::vector<TileLayer> layers_;
};

} // namespace vengine::assets
