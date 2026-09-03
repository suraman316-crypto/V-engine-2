#pragma once

// V Engine 2.0 — Physics v2 extras (Phase 3 continuation): joints, collision
// manifolds, collision filters, broad-phase grid.
#include <vengine/math/Vec2.hpp>
#include <vengine/math/AABB.hpp>

#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

namespace vengine::physics2 {

using vengine::math::Vec2f;
using vengine::math::AABB;

enum class JointType { Distance, Revolute, Prismatic, Weld, Spring };

struct Joint {
    JointType type{JointType::Distance};
    std::uint32_t body_a{0};
    std::uint32_t body_b{0};
    Vec2f local_anchor_a{0, 0};
    Vec2f local_anchor_b{0, 0};
    float rest_length{1.0f};
    float stiffness{100.0f};
    float damping{5.0f};
    float min_limit{0.0f};
    float max_limit{1.0f};
    bool limit_enabled{false};
    bool motor_enabled{false};
    float motor_speed{0.0f};
    float max_motor_force{1000.0f};
};

/// A contact point in a collision manifold.
struct ContactPoint {
    Vec2f position{0, 0};
    Vec2f normal{0, 1};
    float penetration{0.0f};
    float normal_impulse{0.0f};
    float tangent_impulse{0.0f};
    float restitution{0.3f};
    float friction{0.5f};
};

struct Manifold {
    std::uint32_t body_a{0};
    std::uint32_t body_b{0};
    std::vector<ContactPoint> contacts;
};

/// 16-bit collision filter: category bits + mask bits.
struct CollisionFilter {
    std::uint16_t category_bits{0xFFFF};
    std::uint16_t mask_bits{0xFFFF};
    std::int16_t group_index{0};
    bool can_collide(const CollisionFilter& o) const {
        if (group_index != 0 && group_index == o.group_index)
            return group_index > 0;
        return (category_bits & o.mask_bits) != 0 && (o.category_bits & mask_bits) != 0;
    }
};

/// Uniform-grid broad phase: buckets AABBs into cells for pair generation.
class GridBroadPhase {
public:
    GridBroadPhase(float cell_size = 64.0f) : cell_size_(cell_size) {}

    void insert(std::uint32_t id, const AABB& box) {
        int x0 = cell(box.min.x), y0 = cell(box.min.y);
        int x1 = cell(box.max.x), y1 = cell(box.max.y);
        for (int y = y0; y <= y1; ++y)
            for (int x = x0; x <= x1; ++x)
                grid_[key(x, y)].push_back(id);
    }

    /// Candidate pairs sharing a cell (caller does narrow-phase).
    std::vector<std::pair<std::uint32_t, std::uint32_t>> pairs() const {
        std::vector<std::pair<std::uint32_t, std::uint32_t>> out;
        for (auto& [k, ids] : grid_)
            for (std::size_t i = 0; i < ids.size(); ++i)
                for (std::size_t j = i + 1; j < ids.size(); ++j)
                    out.push_back({ids[i], ids[j]});
        return out;
    }
    void clear() { grid_.clear(); }

private:
    int cell(float v) const { return static_cast<int>(v / cell_size_); }
    std::uint64_t key(int x, int y) const {
        return (static_cast<std::uint64_t>(x + 32768) << 32) | static_cast<std::uint32_t>(y + 32768);
    }
    float cell_size_;
    std::unordered_map<std::uint64_t, std::vector<std::uint32_t>> grid_;
};

} // namespace vengine::physics2
