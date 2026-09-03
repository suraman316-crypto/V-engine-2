#pragma once

// V Engine 2.0 — Texture atlas: pack many small sprite rectangles into one
// big texture to reduce bind-switches at draw time.
//
// Uses a guillotine (shelf + skyline) packer with Best-Area-Fit heuristic.
// Returns the UV rectangle for each added rect; the pixel data is uploaded
// by the backend.

#include <vengine/math/Rect.hpp>

#include <algorithm>
#include <cstdint>
#include <vector>

namespace vengine::renderer {

struct PackedRect {
    std::uint32_t id{0};
    math::Rectf uv{};        ///< 0..1 in atlas space
    int x{0}, y{0};          ///< pixel position
    int w{0}, h{0};          ///< pixel size
};

/// Skyline bottom-left bin packer.
class TextureAtlas {
public:
    TextureAtlas(int width = 1024, int height = 1024)
        : width_(width), height_(height), skyline_(1, {0, 0, width_}) {}

    /// Returns a packed rect, or {0} on failure.
    PackedRect add(int w, int h, std::uint32_t id) {
        if (w > width_ || h > height_) return {};
        int best_x = 0, best_y = height_ + 1, best_index = -1;
        // best-area fit across all skylines.
        for (std::size_t i = 0; i < skyline_.size(); ++i) {
            int y = skyline_[i].y;
            // find the lowest y at which `w` fits horizontally starting at skyline_[i].x
            int x = skyline_[i].x;
            int needed = w;
            std::size_t j = i;
            int cur_top = y;
            while (needed > 0 && j < skyline_.size()) {
                if (skyline_[j].y > cur_top) cur_top = skyline_[j].y;
                needed -= skyline_[j].width;
                ++j;
            }
            if (needed > 0) continue; // ran off the edge of the atlas row
            if (cur_top + h > height_) continue;
            if (cur_top < best_y || (cur_top == best_y && i < static_cast<std::size_t>(best_index))) {
                best_y = cur_top;
                best_x = skyline_[i].x;
                best_index = static_cast<int>(i);
            }
        }
        if (best_index < 0) return {};
        PackedRect r;
        r.id = id;
        r.x = best_x; r.y = best_y;
        r.w = w; r.h = h;
        r.uv = to_uv(best_x, best_y, w, h);
        add_skyline(best_index, best_x, best_y + h, w);
        rects_.push_back(r);
        return r;
    }

    int width() const noexcept { return width_; }
    int height() const noexcept { return height_; }
    std::size_t count() const noexcept { return rects_.size(); }

private:
    struct Skyline { int x, y, width; };

    void add_skyline(int index, int x, int y, int width) {
        Skyline new_line{x, y, width};
        skyline_.insert(skyline_.begin() + index, new_line);
        // merge with next if overlapping
        std::size_t i = static_cast<std::size_t>(index);
        while (i + 1 < skyline_.size() && skyline_[i].x + skyline_[i].width >= skyline_[i+1].x) {
            int new_w = std::max(skyline_[i].x + skyline_[i].width,
                                 skyline_[i+1].x + skyline_[i+1].width) - skyline_[i].x;
            skyline_[i].width = new_w;
            skyline_.erase(skyline_.begin() + i + 1);
        }
    }

    math::Rectf to_uv(int x, int y, int w, int h) const {
        return math::Rectf{
            static_cast<float>(x) / width_,
            static_cast<float>(y) / height_,
            static_cast<float>(w) / width_,
            static_cast<float>(h) / height_,
        };
    }

    int width_, height_;
    std::vector<Skyline> skyline_;
    std::vector<PackedRect> rects_;
};

} // namespace vengine::renderer
