#pragma once

// V Engine 2.0 — Geometry: shapes, raycasting primitives, spatial queries.
//
// Everything a 2D engine needs: circles, capsules, polygons, rays, and
// closest-point/containment/intersection tests used by physics, UI, and the
// editor's gizmo picking.

#include <vengine/math/AABB.hpp>
#include <vengine/math/Vec2.hpp>

#include <array>
#include <cmath>
#include <vector>

namespace vengine::math {

/// Distance from point p to segment ab, plus the closest point.
inline float point_segment_distance(Vec2f p, Vec2f a, Vec2f b, Vec2f* closest = nullptr) {
    const Vec2f ab = b - a;
    const float len_sq = ab.dot(ab);
    if (len_sq < 1e-12f) {
        if (closest) *closest = a;
        return (p - a).length();
    }
    float t = (p - a).dot(ab) / len_sq;
    t = std::clamp(t, 0.0f, 1.0f);
    const Vec2f c = a + ab * t;
    if (closest) *closest = c;
    return (p - c).length();
}

struct Circle {
    Vec2f center{};
    float radius{1.0f};

    bool contains(const Vec2f& p) const { return (p - center).length_squared() <= radius * radius; }
    AABB aabb() const { return AABB{center - Vec2f{radius, radius}, center + Vec2f{radius, radius}}; }
};

struct Capsule {
    Vec2f a{}, b{};
    float radius{1.0f};

    float distance_to(const Vec2f& p) const {
        return point_segment_distance(p, a, b) - radius;
    }
    bool contains(const Vec2f& p) const { return distance_to(p) <= 0.0f; }
    AABB aabb() const {
        const Vec2f mn = Vec2f{std::min(a.x, b.x), std::min(a.y, b.y)} - Vec2f{radius, radius};
        const Vec2f mx = Vec2f{std::max(a.x, b.x), std::max(a.y, b.y)} + Vec2f{radius, radius};
        return AABB{mn, mx};
    }
};

/// Convex polygon (CCW winding expected by SAT tests).
struct Polygon {
    std::vector<Vec2f> points;

    AABB aabb() const {
        if (points.empty()) return AABB{};
        Vec2f mn = points[0], mx = points[0];
        for (const auto& p : points) {
            mn = Vec2f{std::min(mn.x, p.x), std::min(mn.y, p.y)};
            mx = Vec2f{std::max(mx.x, p.x), std::max(mx.y, p.y)};
        }
        return AABB{mn, mx};
    }

    Vec2f centroid() const {
        if (points.empty()) return {};
        Vec2f c{0,0};
        for (const auto& p : points) c = c + p;
        return c * (1.0f / static_cast<float>(points.size()));
    }

    /// Generate a regular convex polygon.
    static Polygon regular(Vec2f center, float radius, std::size_t sides, float rotation = 0.0f) {
        Polygon poly;
        poly.points.reserve(sides);
        for (std::size_t i = 0; i < sides; ++i) {
            const float a = rotation + (i / static_cast<float>(sides)) * 6.28318530718f;
            poly.points.push_back(center + Vec2f{std::cos(a), std::sin(a)} * radius);
        }
        return poly;
    }
};

/// Separating-axis theorem test for two convex polygons.
inline bool sat_overlap(const Polygon& a, const Polygon& b) {
    auto has_separating_axis = [](const Polygon& p, const Polygon& q) {
        const std::size_t n = p.points.size();
        if (n < 2) return false;
        for (std::size_t i = 0; i < n; ++i) {
            const Vec2f edge = p.points[(i + 1) % n] - p.points[i];
            const Vec2f axis = Vec2f{-edge.y, edge.x};
            float min_p = 1e30f, max_p = -1e30f, min_q = 1e30f, max_q = -1e30f;
            for (const auto& v : p.points) {
                const float d = axis.dot(v);
                min_p = std::min(min_p, d); max_p = std::max(max_p, d);
            }
            for (const auto& v : q.points) {
                const float d = axis.dot(v);
                min_q = std::min(min_q, d); max_q = std::max(max_q, d);
            }
            if (max_p < min_q || max_q < min_p) return true;
        }
        return false;
    };
    return !has_separating_axis(a, b) && !has_separating_axis(b, a);
}

} // namespace vengine::math
