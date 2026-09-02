#pragma once

// V Engine 2.0 — Uniform-grid spatial hash for O(1) broad-phase pair
// generation when bodies are roughly uniformly distributed.
//
// Well-suited to 2D games where most entities are similar size. For
// highly variable sizes, the BVH (BVH.hpp) is preferred.

#include <vengine/math/AABB.hpp>

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace vengine::physics {

class SpatialHashGrid {
public:
    explicit SpatialHashGrid(float cell_size = 64.0f) : cell_size_(cell_size) {}

    void clear() { cells_.clear(); }

    /// Insert an object id covering the given AABB.
    void insert(std::uint32_t id, const AABB& aabb) {
        const int x0 = cell(aabb.min.x), y0 = cell(aabb.min.y);
        const int x1 = cell(aabb.max.x), y1 = cell(aabb.max.y);
        for (int x = x0; x <= x1; ++x)
            for (int y = y0; y <= y1; ++y)
                cells_[hash_key(x, y)].push_back(id);
    }

    /// Collect unique candidate pairs overlapping any cell.
    template <typename PairFn>
    void for_each_pair(PairFn&& fn) const {
        for (const auto& [key, ids] : cells_) {
            for (std::size_t i = 0; i < ids.size(); ++i)
                for (std::size_t j = i + 1; j < ids.size(); ++j)
                    if (ids[i] != ids[j]) fn(ids[i], ids[j]);
        }
    }

    /// Query all ids in the same cells as `query`.
    template <typename Visitor>
    void query(const AABB& query, Visitor&& visit) const {
        const int x0 = cell(query.min.x), y0 = cell(query.min.y);
        const int x1 = cell(query.max.x), y1 = cell(query.max.y);
        for (int x = x0; x <= x1; ++x)
            for (int y = y0; y <= y1; ++y) {
                auto it = cells_.find(hash_key(x, y));
                if (it == cells_.end()) continue;
                for (auto id : it->second) visit(id);
            }
    }

    float cell_size() const noexcept { return cell_size_; }

private:
    int cell(float v) const { return static_cast<int>(std::floor(v / cell_size_)); }
    static std::uint64_t hash_key(int x, int y) {
        // Thomas Mueller hash (pair to single int -> as key)
        std::uint32_t h = static_cast<std::uint32_t>(x) * 73856093u
                        ^ static_cast<std::uint32_t>(y) * 19349663u;
        return static_cast<std::uint64_t>(h);
    }

    float cell_size_;
    std::unordered_map<std::uint64_t, std::vector<std::uint32_t>> cells_;
};

} // namespace vengine::physics
