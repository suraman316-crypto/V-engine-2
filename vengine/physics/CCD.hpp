#pragma once

// V Engine 2.0 — Continuous collision detection (CCD) via ray-sweep.
//
// Prevents fast-moving ("tunneling") bodies from skipping through thin
// geometry. Uses a conservative advancement + ray-AABB test at substeps.

#include <vengine/math/AABB.hpp>
#include <vengine/math/Vec2.hpp>

#include <cmath>
#include <cstdint>
#include <vector>

namespace vengine::physics {

/// Sweep a moving AABB against a static AABB. Returns the first time-of-impact
/// in [0,1] along the swept path, or 1.0 if no impact.
/// Uses the slab method on the swept ray.
inline float sweep_aabb(const AABB& moving, const math::Vec2f& delta,
                        const AABB& target) {
    // Expand the target by the moving box's half-extents.
    math::Vec2f mh = (moving.max - moving.min) * 0.5f;
    AABB expanded{
        {target.min.x - mh.x, target.min.y - mh.y},
        {target.max.x + mh.x, target.max.y + mh.y},
    };
    math::Vec2f origin = moving.center();
    float tmin = 0.0f, tmax = 1.0f;
    for (int axis = 0; axis < 2; ++axis) {
        const float o = (axis == 0 ? origin.x : origin.y);
        const float d = (axis == 0 ? delta.x : delta.y);
        const float lo = (axis == 0 ? expanded.min.x : expanded.min.y);
        const float hi = (axis == 0 ? expanded.max.x : expanded.max.y);
        if (std::abs(d) < 1e-8f) {
            if (o < lo || o > hi) return 1.0f; // never overlaps on this axis
            continue;
        }
        float t1 = (lo - o) / d;
        float t2 = (hi - o) / d;
        if (t1 > t2) std::swap(t1, t2);
        tmin = std::max(tmin, t1);
        tmax = std::min(tmax, t2);
        if (tmin > tmax) return 1.0f;
    }
    return tmin >= 0.0f ? tmin : 1.0f;
}

/// Conservative advancement for circle-vs-circle: returns time-of-impact.
inline float sweep_circle(math::Vec2f p0, float r0, math::Vec2f delta,
                          math::Vec2f p1, float r1) {
    const float total_r = r0 + r1;
    const math::Vec2f rel = p1 - p0;
    const float dist = rel.length();
    if (dist <= total_r) return 0.0f; // already overlapping
    const math::Vec2f n = rel / dist;
    const float projected = delta.dot(n);
    if (projected <= 0.0f) return 1.0f; // moving away
    const float travel = (dist - total_r) / projected;
    return travel <= 1.0f ? travel : 1.0f;
}

} // namespace vengine::physics
