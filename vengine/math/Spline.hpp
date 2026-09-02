#pragma once

// V Engine 2.0 — Splines: Catmull-Rom, Bezier, B-spline paths.
//
// Used by cameras (follow paths), particles (emitter trajectories), AI
// patrol routes, and editor gizmos.

#include <vengine/math/Vec2.hpp>

#include <vector>

namespace vengine::math {

/// Catmull-Rom spline through control points. Pass through all points.
class CatmullRom {
public:
    explicit CatmullRom(std::vector<Vec2f> pts = {}) : points_(std::move(pts)) {}

    void add(Vec2f p) { points_.push_back(p); }
    std::size_t size() const noexcept { return points_.size(); }
    const std::vector<Vec2f>& points() const noexcept { return points_; }

    /// Sample the spline at t in [0,1] across the whole path.
    Vec2f sample(float t) const {
        if (points_.empty()) return {};
        if (points_.size() == 1) return points_[0];
        t = std::clamp(t, 0.0f, 1.0f);
        const float f = t * static_cast<float>(points_.size() - 1);
        const std::size_t i = std::min<std::size_t>(
            static_cast<std::size_t>(f), points_.size() - 2);
        const float u = f - static_cast<float>(i);
        return segment(i, u);
    }

    /// Sample segment i at local parameter u in [0,1].
    Vec2f segment(std::size_t i, float u) const {
        if (i + 1 >= points_.size()) return points_.empty() ? Vec2f{} : points_.back();
        const Vec2f& p0 = points_[i == 0 ? 0 : i - 1];
        const Vec2f& p1 = points_[i];
        const Vec2f& p2 = points_[i + 1];
        const Vec2f& p3 = points_[i + 2 < points_.size() ? i + 2 : points_.size() - 1];
        const float u2 = u * u, u3 = u2 * u;
        return Vec2f{
            0.5f * ((2.0f * p1.x) + (-p0.x + p2.x) * u
                    + (2.0f*p0.x - 5.0f*p1.x + 4.0f*p2.x - p3.x) * u2
                    + (-p0.x + 3.0f*p1.x - 3.0f*p2.x + p3.x) * u3),
            0.5f * ((2.0f * p1.y) + (-p0.y + p2.y) * u
                    + (2.0f*p0.y - 5.0f*p1.y + 4.0f*p2.y - p3.y) * u2
                    + (-p0.y + 3.0f*p1.y - 3.0f*p2.y + p3.y) * u3),
        };
    }

    /// Arc-length reparameterization: t is a fraction of total length.
    Vec2f sample_uniform(float t) const {
        if (points_.size() < 2) return points_.empty() ? Vec2f{} : points_[0];
        build_length_table();
        const float target = std::clamp(t, 0.0f, 1.0f) * total_length_;
        // binary search the cumulative length table
        std::size_t lo = 0, hi = arc_lengths_.size() - 1;
        while (hi - lo > 1) {
            const std::size_t mid = (lo + hi) / 2;
            if (arc_lengths_[mid] < target) lo = mid;
            else hi = mid;
        }
        const float seg = arc_lengths_[hi] - arc_lengths_[lo];
        const float u = seg > 1e-9f ? (target - arc_lengths_[lo]) / seg : 0.0f;
        return segment(lo / kSamplesPerSegment, u);
    }

    float total_length() const {
        if (points_.size() < 2) return 0.0f;
        build_length_table();
        return total_length_;
    }

private:
    static constexpr std::size_t kSamplesPerSegment = 16;

    void build_length_table() const {
        if (arc_lengths_.size() == points_.size() * kSamplesPerSegment) return;
        arc_lengths_.clear();
        arc_lengths_.push_back(0.0f);
        total_length_ = 0.0f;
        Vec2f prev = points_[0];
        for (std::size_t i = 0; i + 1 < points_.size(); ++i) {
            for (std::size_t j = 1; j <= kSamplesPerSegment; ++j) {
                const float u = j / static_cast<float>(kSamplesPerSegment);
                const Vec2f p = segment(i, u);
                total_length_ += (p - prev).length();
                arc_lengths_.push_back(total_length_);
                prev = p;
            }
        }
    }

    std::vector<Vec2f> points_;
    mutable std::vector<float> arc_lengths_;
    mutable float total_length_{0.0f};
};

/// Cubic Bezier with two control points.
struct Bezier3 {
    Vec2f p0{}, c0{}, c1{}, p1{};

    Vec2f sample(float t) const {
        const float u = 1.0f - t;
        return p0 * (u*u*u) + c0 * (3.0f*u*u*t) + c1 * (3.0f*u*t*t) + p1 * (t*t*t);
    }
};

} // namespace vengine::math
