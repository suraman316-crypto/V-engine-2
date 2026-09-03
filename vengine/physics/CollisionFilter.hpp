#pragma once

// V Engine 2.0 — Collision filtering: layers, groups, masks.
//
// Bitwise layer system: each body belongs to a 16-bit group of layers, and
// has a 16-bit mask of layers it collides with. Two bodies collide iff
// (a.group & b.mask) && (b.group & a.group). Sensors/triggers report overlap
// but no impulse resolution.

#include <cstdint>

namespace vengine::physics {

struct CollisionFilter {
    std::uint16_t group{0xFFFF}; ///< which layers this body is on
    std::uint16_t mask{0xFFFF};  ///< which layers this body collides with
    std::int16_t  category{-1};  ///< negative = no group-based override
    bool          is_trigger{false};

    /// True if two bodies should interact (trigger or collide).
    static bool should_collide(const CollisionFilter& a, const CollisionFilter& b) {
        // group override: same non-negative category = always collide;
        // opposite categories = never collide.
        if (a.category >= 0 && b.category >= 0) {
            if (a.category == b.category) return true;
            if (a.category == -b.category) return false;
        }
        return (a.group & b.mask) != 0 && (b.group & a.mask) != 0;
    }

    static bool is_trigger_pair(const CollisionFilter& a, const CollisionFilter& b) {
        return a.is_trigger || b.is_trigger;
    }
};

/// Pre-defined layer constants for common game categories.
enum class CollisionLayer : std::uint16_t {
    Default     = 1u << 0,
    Player      = 1u << 1,
    Enemy       = 1u << 2,
    Environment = 1u << 3,
    Projectile  = 1u << 4,
    Trigger     = 1u << 5,
    Pickup      = 1u << 6,
    All         = 0xFFFFu,
};

} // namespace vengine::physics
