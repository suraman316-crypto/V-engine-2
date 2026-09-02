#pragma once

// V Engine 2.0 — Island/Constraint solver: groups connected bodies (via
// contacts or joints) into "islands" and solves each independently.
//
// This is the standard approach (used by Box2D) for handling constraint
// graphs of arbitrary size. Bodies in different islands don't interact,
// so small-island solving is cache-friendly and parallelizable.

#include <algorithm>
#include <cstdint>
#include <stack>
#include <unordered_map>
#include <vector>

namespace vengine::physics {

struct IslandGraph {
    /// Edge: body `a` connects to body `b` (undirected).
    struct Edge { std::uint32_t a, b; };

    std::vector<Edge> edges;
    std::uint32_t body_count{0};

    void clear() { edges.clear(); body_count = 0; }
    void add_edge(std::uint32_t a, std::uint32_t b) { edges.push_back({a, b}); }

    /// Union-find build + collect islands (each island = list of body indices).
    std::vector<std::vector<std::uint32_t>> compute_islands() const {
        std::vector<std::uint32_t> parent(body_count);
        for (std::uint32_t i = 0; i < body_count; ++i) parent[i] = i;
        auto find = [&](std::uint32_t x) -> std::uint32_t {
            while (parent[x] != x) { parent[x] = parent[parent[x]]; x = parent[x]; }
            return x;
        };
        auto unite = [&](std::uint32_t a, std::uint32_t b) {
            std::uint32_t ra = find(a), rb = find(b);
            if (ra != rb) parent[ra] = rb;
        };
        for (const auto& e : edges) {
            if (e.a < body_count && e.b < body_count) unite(e.a, e.b);
        }
        std::unordered_map<std::uint32_t, std::vector<std::uint32_t>> buckets;
        for (std::uint32_t i = 0; i < body_count; ++i)
            buckets[find(i)].push_back(i);
        std::vector<std::vector<std::uint32_t>> result;
        result.reserve(buckets.size());
        for (auto& [root, members] : buckets)
            result.push_back(std::move(members));
        return result;
    }
};

} // namespace vengine::physics
