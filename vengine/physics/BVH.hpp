#pragma once

// V Engine 2.0 — Dynamic bounding volume hierarchy (BVH) broad-phase.
//
// A balanced AABB tree supporting incremental insert/remove/move and
// efficient range and ray queries. This replaces the O(n^2) pair check in
// the original solver for large scenes (thousands of bodies).

#include <vengine/math/AABB.hpp>

#include <algorithm>
#include <cstdint>
#include <stack>
#include <vector>

namespace vengine::physics {

using math::AABB;

class BVHTree {
public:
    static constexpr std::uint32_t kNull = 0xFFFFFFFFu;

    std::uint32_t insert(const AABB& aabb, std::uint32_t user_id) {
        const std::uint32_t leaf = alloc_node();
        nodes_[leaf].aabb = aabb;
        nodes_[leaf].user_id = user_id;
        nodes_[leaf].is_leaf = true;
        insert_leaf(leaf);
        return leaf;
    }

    void remove(std::uint32_t node_id) {
        if (node_id >= nodes_.size() || !nodes_[node_id].is_leaf) return;
        remove_leaf(node_id);
        free_node(node_id);
    }

    bool update(std::uint32_t node_id, const AABB& aabb) {
        if (node_id >= nodes_.size() || !nodes_[node_id].is_leaf) return false;
        AABB combined = nodes_[node_id].aabb;
        combined.min.x = std::min(combined.min.x, aabb.min.x);
        combined.min.y = std::min(combined.min.y, aabb.min.y);
        combined.max.x = std::max(combined.max.x, aabb.max.x);
        combined.max.y = std::max(combined.max.y, aabb.max.y);
        // early-out: if the AABB grew more than a fat-margin, reinsert.
        const AABB& cur = nodes_[node_id].aabb;
        const float growth = area(aabb) - area(cur);
        if (growth <= area(cur) * 0.1f && contains(cur, aabb)) return false;
        remove_leaf(node_id);
        nodes_[node_id].aabb = aabb;
        insert_leaf(node_id);
        return true;
    }

    /// Query all leaf user-ids whose AABBs overlap `query`.
    template <typename Visitor>
    void query(const AABB& query, Visitor&& visit) const {
        if (root_ == kNull) return;
        std::stack<std::uint32_t> stack;
        stack.push(root_);
        while (!stack.empty()) {
            const std::uint32_t idx = stack.top(); stack.pop();
            if (idx == kNull || idx >= nodes_.size()) continue;
            const Node& n = nodes_[idx];
            if (!n.aabb.intersects(query)) continue;
            if (n.is_leaf) visit(n.user_id);
            else { stack.push(n.left); stack.push(n.right); }
        }
    }

    std::size_t size() const noexcept { return leaf_count_; }

private:
    struct Node {
        AABB aabb{};
        std::uint32_t parent{kNull};
        std::uint32_t left{kNull};
        std::uint32_t right{kNull};
        std::uint32_t user_id{0};
        int height{0};
        bool is_leaf{false};
    };

    std::uint32_t alloc_node() {
        if (free_list_ != kNull) {
            const std::uint32_t idx = free_list_;
            free_list_ = nodes_[idx].parent;
            nodes_[idx] = Node{};
            return idx;
        }
        nodes_.push_back(Node{});
        return static_cast<std::uint32_t>(nodes_.size() - 1);
    }

    void free_node(std::uint32_t idx) {
        nodes_[idx].is_leaf = false;
        nodes_[idx].parent = free_list_;
        free_list_ = idx;
        --leaf_count_;
    }

    static float area(const AABB& a) {
        return a.width() * a.height();
    }
    static AABB union_of(const AABB& a, const AABB& b) {
        return AABB{
            {std::min(a.min.x, b.min.x), std::min(a.min.y, b.min.y)},
            {std::max(a.max.x, b.max.x), std::max(a.max.y, b.max.y)},
        };
    }
    static bool contains(const AABB& outer, const AABB& inner) {
        return outer.min.x <= inner.min.x && outer.min.y <= inner.min.y
            && outer.max.x >= inner.max.x && outer.max.y >= inner.max.y;
    }

    void insert_leaf(std::uint32_t leaf) {
        ++leaf_count_;
        if (root_ == kNull) { root_ = leaf; nodes_[leaf].parent = kNull; return; }
        const AABB leaf_box = nodes_[leaf].aabb;
        // walk down to best sibling (minimize perimeter heuristic)
        std::uint32_t idx = root_;
        while (!nodes_[idx].is_leaf) {
            const std::uint32_t left = nodes_[idx].left;
            const std::uint32_t right = nodes_[idx].right;
            const float cur_area = area(nodes_[idx].aabb);
            const AABB combined = union_of(nodes_[idx].aabb, leaf_box);
            const float combined_area = area(combined);
            const float cost = 2.0f * combined_area;
            const float inheritance = 2.0f * (combined_area - cur_area);
            float left_cost = nodes_[left].is_leaf ? area(union_of(nodes_[left].aabb, leaf_box)) + inheritance : 2.0f * area(union_of(nodes_[left].aabb, leaf_box)) + inheritance - area(nodes_[left].aabb);
            float right_cost = nodes_[right].is_leaf ? area(union_of(nodes_[right].aabb, leaf_box)) + inheritance : 2.0f * area(union_of(nodes_[right].aabb, leaf_box)) + inheritance - area(nodes_[right].aabb);
            if (cost < left_cost && cost < right_cost) break;
            idx = left_cost < right_cost ? left : right;
        }
        const std::uint32_t sibling = idx;
        const std::uint32_t old_parent = nodes_[sibling].parent;
        const std::uint32_t new_internal = alloc_node();
        nodes_[new_internal].parent = old_parent;
        nodes_[new_internal].aabb = union_of(union_of(nodes_[sibling].aabb, leaf_box), leaf_box);
        nodes_[new_internal].is_leaf = false;
        nodes_[new_internal].height = nodes_[sibling].height + 1;
        if (old_parent != kNull) {
            if (nodes_[old_parent].left == sibling) nodes_[old_parent].left = new_internal;
            else nodes_[old_parent].right = new_internal;
        } else root_ = new_internal;
        nodes_[new_internal].left = sibling;
        nodes_[new_internal].right = leaf;
        nodes_[sibling].parent = new_internal;
        nodes_[leaf].parent = new_internal;
        // walk up: refit AABBs and balance
        std::uint32_t p = nodes_[new_internal].parent;
        while (p != kNull) {
            p = balance(p);
            nodes_[p].aabb = union_of(nodes_[nodes_[p].left].aabb, nodes_[nodes_[p].right].aabb);
            nodes_[p].height = 1 + std::max(nodes_[nodes_[p].left].height, nodes_[nodes_[p].right].height);
            p = nodes_[p].parent;
        }
    }

    void remove_leaf(std::uint32_t leaf) {
        --leaf_count_;
        if (leaf == root_) { root_ = kNull; return; }
        const std::uint32_t parent = nodes_[leaf].parent;
        const std::uint32_t grandparent = nodes_[parent].parent;
        const std::uint32_t sibling = nodes_[parent].left == leaf ? nodes_[parent].right : nodes_[parent].left;
        if (grandparent != kNull) {
            if (nodes_[grandparent].left == parent) nodes_[grandparent].left = sibling;
            else nodes_[grandparent].right = sibling;
            nodes_[sibling].parent = grandparent;
            std::uint32_t p = grandparent;
            while (p != kNull) {
                nodes_[p].aabb = union_of(nodes_[nodes_[p].left].aabb, nodes_[nodes_[p].right].aabb);
                nodes_[p].height = 1 + std::max(nodes_[nodes_[p].left].height, nodes_[nodes_[p].right].height);
                p = nodes_[p].parent;
            }
        } else {
            root_ = sibling;
            nodes_[sibling].parent = kNull;
        }
        free_node(parent);
    }

    std::uint32_t balance(std::uint32_t a) {
        if (nodes_[a].is_leaf || nodes_[a].height < 2) return a;
        const std::uint32_t b = nodes_[a].left;
        const std::uint32_t c = nodes_[a].right;
        const int balance = nodes_[c].height - nodes_[b].height;
        if (balance > 1) rotate(a, b, c);
        else if (balance < -1) rotate(a, c, b);
        return a;
    }

    void rotate(std::uint32_t a, std::uint32_t b, std::uint32_t c) {
        const std::uint32_t f = nodes_[c].left;
        const std::uint32_t g = nodes_[c].right;
        nodes_[c].left = a;
        nodes_[c].parent = nodes_[a].parent;
        nodes_[a].parent = c;
        if (nodes_[c].parent != kNull) {
            if (nodes_[nodes_[c].parent].left == a) nodes_[nodes_[c].parent].left = c;
            else nodes_[nodes_[c].parent].right = c;
        } else root_ = c;
        if (area(nodes_[f].aabb) > area(nodes_[g].aabb)) {
            nodes_[c].right = f;
            nodes_[a].right = g; nodes_[g].parent = a;
            nodes_[a].aabb = union_of(nodes_[b].aabb, nodes_[g].aabb);
            nodes_[c].aabb = union_of(nodes_[a].aabb, nodes_[f].aabb);
        } else {
            nodes_[c].right = g;
            nodes_[a].right = f; nodes_[f].parent = a;
            nodes_[a].aabb = union_of(nodes_[b].aabb, nodes_[f].aabb);
            nodes_[c].aabb = union_of(nodes_[a].aabb, nodes_[g].aabb);
        }
        nodes_[a].height = 1 + std::max(nodes_[nodes_[a].left].height, nodes_[nodes_[a].right].height);
        nodes_[c].height = 1 + std::max(nodes_[nodes_[c].left].height, nodes_[nodes_[c].right].height);
    }

    std::vector<Node> nodes_;
    std::uint32_t root_{kNull};
    std::uint32_t free_list_{kNull};
    std::size_t leaf_count_{0};
};

} // namespace vengine::physics
