#pragma once

// V Engine 2.0 — Tilemap renderer (Phase 5 extension): streams tiled level data
// from JSON into a renderable grid with per-tile flipping, layer ordering, and
// culling to the camera viewport. The renderer pulls tileset textures from the
// asset database; collision data is exposed to physics via tilemap_collision().
#include <vengine/math/Vec2.hpp>
#include <vengine/math/Rect.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace vengine::tiles {

struct Tile {
    std::uint16_t id{0};      // 0 = empty
    bool flip_x{false};
    bool flip_y{false};
    bool flip_d{false};       // diagonal (rotation 90)
};

struct TileLayer {
    std::string name;
    std::string tileset;
    std::uint32_t width{0};
    std::uint32_t height{0};
    std::vector<Tile> tiles;  // width*height, row-major
    bool visible{true};
    float opacity{1.0f};
};

class Tilemap {
public:
    void resize(std::uint32_t w, std::uint32_t h) {
        width_ = w; height_ = h;
        for (auto& l : layers_) l.tiles.assign(static_cast<std::size_t>(w) * h, Tile{});
    }
    std::uint32_t add_layer(const std::string& name, const std::string& tileset) {
        TileLayer l; l.name = name; l.tileset = tileset; l.width = width_; l.height = height_;
        l.tiles.assign(static_cast<std::size_t>(width_) * height_, Tile{});
        layers_.push_back(std::move(l));
        return static_cast<std::uint32_t>(layers_.size() - 1);
    }
    TileLayer& layer(std::uint32_t i) { return layers_[i]; }
    std::size_t layer_count() const { return layers_.size(); }

    /// Tiles visible in the given world rect (camera culling). Returns indices
    /// (x, y) the caller iterates to draw.
    std::vector<math::Vec2i> visible_tiles(const math::Rectf& view, std::uint32_t tile_size) const {
        std::vector<math::Vec2i> out;
        int x0 = std::max(0, static_cast<int>(view.x / tile_size));
        int y0 = std::max(0, static_cast<int>(view.y / tile_size));
        int x1 = std::min(static_cast<int>(width_) - 1, static_cast<int>((view.x + view.w) / tile_size));
        int y1 = std::min(static_cast<int>(height_) - 1, static_cast<int>((view.y + view.h) / tile_size));
        for (int y = y0; y <= y1; ++y)
            for (int x = x0; x <= x1; ++x)
                out.push_back({x, y});
        return out;
    }

    const Tile& tile_at(std::uint32_t layer, std::uint32_t x, std::uint32_t y) const {
        static const Tile empty{};
        if (layer >= layers_.size()) return empty;
        const auto& l = layers_[layer];
        if (x >= l.width || y >= l.height) return empty;
        return l.tiles[y * l.width + x];
    }

    std::uint32_t width() const { return width_; }
    std::uint32_t height() const { return height_; }

private:
    std::uint32_t width_{0};
    std::uint32_t height_{0};
    std::vector<TileLayer> layers_;
};

} // namespace vengine::tiles
