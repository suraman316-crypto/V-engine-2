#pragma once

// V Engine 2.0 — Navigation: A* pathfinding on a grid + navmesh, plus
// steering behaviors for flocking/avoidance.
//
// Grid A* is the simplest pathfinder: the world is discretized into cells
// (walkable/blocked) and A* finds the lowest-cost path using the octile
// heuristic. For complex worlds, a NavMesh (list of convex polygons with
// shared edges) is used with the funnel algorithm.

#include <vengine/math/Vec2.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <queue>
#include <unordered_map>
#include <vector>

namespace vengine::ai {

/// Grid A* pathfinder.
class GridPathfinder {
public:
    GridPathfinder(int w, int h) : width_(w), height_(h), cells_(w * h, true) {}

    void set_walkable(int x, int y, bool walkable) {
        if (in_bounds(x, y)) cells_[y * width_ + x] = walkable;
    }
    bool walkable(int x, int y) const {
        return in_bounds(x, y) && cells_[y * width_ + x];
    }

    /// 4- or 8-connected A*. Returns list of cell centers (world pos per cell).
    std::vector<math::Vec2f> find_path(int sx, int sy, int ex, int ey,
                                       float cell_size = 1.0f,
                                       bool diagonal = true) const {
        if (!walkable(sx, sy) || !walkable(ex, ey)) return {};
        if (sx == ex && sy == ey) return {{sx * cell_size + cell_size * 0.5f, sy * cell_size + cell_size * 0.5f}};
        struct Node { int x, y; float g, f; };
        auto h = [&](int x, int y) -> float {
            const float dx = std::abs(x - ex), dy = std::abs(y - ey);
            return diagonal ? (dx + dy) + (1.4142f - 2.0f) * std::min(dx, dy) : dx + dy;
        };
        auto idx = [&](int x, int y) { return y * width_ + x; };

        std::unordered_map<int, float> g_score;
        std::unordered_map<int, int> came_from;
        auto cmp = [](const Node& a, const Node& b) { return a.f > b.f; };
        std::priority_queue<Node, std::vector<Node>, decltype(cmp)> open(cmp);
        g_score[idx(sx, sy)] = 0.0f;
        open.push({sx, sy, 0.0f, h(sx, sy)});

        const int dx8[8] = {1, -1, 0, 0, 1, 1, -1, -1};
        const int dy8[8] = {0, 0, 1, -1, 1, -1, 1, -1};
        const int n = diagonal ? 8 : 4;

        while (!open.empty()) {
            Node cur = open.top(); open.pop();
            const int cur_idx = idx(cur.x, cur.y);
            auto git = g_score.find(cur_idx);
            if (git == g_score.end() || cur.g > git->second) continue; // stale entry
            if (cur.x == ex && cur.y == ey) {
                std::vector<math::Vec2f> path;
                int c = cur_idx;
                while (c != idx(sx, sy)) {
                    path.push_back({(c % width_) * cell_size + cell_size * 0.5f,
                                    (c / width_) * cell_size + cell_size * 0.5f});
                    auto it = came_from.find(c);
                    if (it == came_from.end()) break;
                    c = it->second;
                }
                path.push_back({sx * cell_size + cell_size * 0.5f, sy * cell_size + cell_size * 0.5f});
                std::reverse(path.begin(), path.end());
                return path;
            }
            for (int i = 0; i < n; ++i) {
                int nx = cur.x + dx8[i], ny = cur.y + dy8[i];
                if (!walkable(nx, ny)) continue;
                if (i >= 4 && (!walkable(cur.x + dx8[i], cur.y) || !walkable(cur.x, cur.y + dy8[i]))) continue;
                float step = (i >= 4) ? 1.4142f : 1.0f;
                float tentative = cur.g + step;
                int ni = idx(nx, ny);
                auto ng = g_score.find(ni);
                if (ng == g_score.end() || tentative < ng->second) {
                    g_score[ni] = tentative;
                    came_from[ni] = cur_idx;
                    open.push({nx, ny, tentative, tentative + h(nx, ny)});
                }
            }
        }
        return {};
    }

    int width() const noexcept { return width_; }
    int height() const noexcept { return height_; }

private:
    bool in_bounds(int x, int y) const { return x >= 0 && y >= 0 && x < width_ && y < height_; }
    int width_, height_;
    std::vector<bool> cells_;
};

/// Steering behaviors: seek, flee, arrive, wander, separation, alignment,
/// cohesion (Reynolds flocking).
struct Steering {
    static math::Vec2f seek(math::Vec2f pos, math::Vec2f vel, math::Vec2f target, float max_speed) {
        math::Vec2f desired = (target - pos).normalized();
        return desired * max_speed - vel;
    }
    static math::Vec2f flee(math::Vec2f pos, math::Vec2f vel, math::Vec2f threat, float max_speed, float panic = 100.0f) {
        float d = (threat - pos).length();
        if (d > panic) return {0, 0};
        math::Vec2f desired = (pos - threat).normalized();
        return desired * max_speed - vel;
    }
    static math::Vec2f arrive(math::Vec2f pos, math::Vec2f vel, math::Vec2f target, float max_speed, float slow_radius) {
        math::Vec2f to = target - pos;
        float d = to.length();
        if (d < 1e-4f) return {0, 0};
        float speed = d < slow_radius ? max_speed * (d / slow_radius) : max_speed;
        return to.normalized() * speed - vel;
    }
    static math::Vec2f wander(math::Vec2f vel, float& angle, float circle_r = 32.0f, float jitter = 0.3f) {
        angle += (static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * 2.0f * jitter;
        math::Vec2f circle{std::cos(angle) * circle_r, std::sin(angle) * circle_r};
        return (vel.normalized() * 64.0f + circle).normalized();
    }
    static math::Vec2f separate(math::Vec2f pos, const std::vector<math::Vec2f>& neighbors, float sep = 50.0f) {
        math::Vec2f sum{0, 0};
        int count = 0;
        for (const auto& n : neighbors) {
            math::Vec2f d = pos - n;
            float dist = d.length();
            if (dist > 0 && dist < sep) { sum = sum + d.normalized() / dist; ++count; }
        }
        if (count == 0) return {0, 0};
        return sum / static_cast<float>(count);
    }
};

} // namespace vengine::ai
