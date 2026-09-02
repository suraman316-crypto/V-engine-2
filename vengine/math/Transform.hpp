#pragma once

// V Engine 2.0 — Transform hierarchy.
//
// Builds on the value-type Transform2D (position/rotation/scale) with a
// parent-child composition graph, dirty-flag propagation, and cached world
// matrices. This is the backbone of every scene-graph-driven system
// (rendering, physics, UI, particles).

#include <vengine/math/Mat4.hpp>
#include <vengine/math/Transform2D.hpp>
#include <vengine/math/Vec2.hpp>
#include <vengine/math/Vec3.hpp>

#include <cstdint>
#include <vector>

namespace vengine::math {

/// Build a local->parent Mat4 from a Transform2D (T * R * S).
inline Mat4 to_matrix(const Transform2D& t) {
    const float c = std::cos(t.rotation), s = std::sin(t.rotation);
    Mat4 m{};
    m.m[0]  = c * t.scale.x;  m.m[1]  = s * t.scale.x;
    m.m[4]  = -s * t.scale.y; m.m[5]  = c * t.scale.y;
    m.m[10] = 1.0f;           m.m[15] = 1.0f;
    m.m[12] = t.position.x;   m.m[13] = t.position.y;
    return m;
}

/// Transform node with dirty tracking and world-matrix cache.
struct TransformNode {
    Transform2D local;
    Mat4 world{Mat4::identity()};
    bool dirty{true};
    std::uint32_t parent{kInvalidIndex};
    std::vector<std::uint32_t> children;

    static constexpr std::uint32_t kInvalidIndex = 0xFFFFFFFFu;
};

/// Flat transform hierarchy: SOA store of nodes with parent indices.
class TransformHierarchy {
public:
    std::uint32_t create(Transform2D local = {}, std::uint32_t parent = TransformNode::kInvalidIndex) {
        nodes_.push_back(TransformNode{std::move(local), {}, true, parent, {}});
        const std::uint32_t idx = static_cast<std::uint32_t>(nodes_.size() - 1);
        if (parent != TransformNode::kInvalidIndex) nodes_[parent].children.push_back(idx);
        return idx;
    }

    void set_local(std::uint32_t i, const Transform2D& t) {
        if (i >= nodes_.size()) return;
        nodes_[i].local = t;
        mark_dirty(i);
    }

    const Transform2D& local(std::uint32_t i) const { return nodes_[i].local; }
    const Mat4& world(std::uint32_t i) const { return nodes_[i].world; }

    void mark_dirty(std::uint32_t i) {
        if (i >= nodes_.size()) return;
        nodes_[i].dirty = true;
        for (std::uint32_t c : nodes_[i].children) mark_dirty(c);
    }

    void update_world(std::uint32_t i, const Mat4& parent_world) {
        TransformNode& n = nodes_[i];
        n.world = parent_world * to_matrix(n.local);
        n.dirty = false;
        for (std::uint32_t c : n.children) update_world(c, n.world);
    }

    void update() {
        for (std::uint32_t i = 0; i < nodes_.size(); ++i) {
            if (nodes_[i].parent == TransformNode::kInvalidIndex) update_world(i, Mat4::identity());
        }
    }

    std::size_t size() const noexcept { return nodes_.size(); }

    std::vector<TransformNode> nodes_;
};

} // namespace vengine::math
