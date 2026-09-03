#pragma once

// V Engine 2.0 — Physics joints/constraints: distance, revolute (pin),
// prismatic (slider), weld, and mouse-spring.
//
// Each joint constrains two bodies. Joints are solved after the contact
// solver in the same velocity-iteration loop, using the standard sequential
// impulse formulation (soft constraints via Baumgarte bias).

#include <vengine/math/Vec2.hpp>
#include <vengine/physics/PhysicsWorld2D.hpp>

#include <cmath>
#include <vector>

namespace vengine::physics {

using math::Vec2f;

enum class JointType : u8 {
    Distance,   ///< Keep two anchor points a fixed distance apart.
    Revolute,   ///< Pin two bodies together at a shared point.
    Prismatic,  ///< Allow sliding along one axis only.
    Weld,       ///< Rigidly glue two bodies (position + rotation).
    Spring,     ///< Soft distance constraint (mouse dragging).
};

struct Joint {
    JointType type{JointType::Distance};
    std::uint32_t a{0}, b{0};      ///< body indices
    math::Vec2f anchor_a{}, anchor_b{}; ///< local-space anchors
    float length{1.0f};            ///< Distance/Weld target length
    float frequency{8.0f};         ///< Spring softness (Hz)
    float damping{0.5f};           ///< Spring damping ratio
    float angle_ref{0.0f};        ///< Weld/Revolute reference angle
    float axis_angle{0.0f};       ///< Prismatic slide axis (world radians)
    float lower_limit{0.0f};      ///< Prismatic translation limits
    float upper_limit{0.0f};
    bool limit_enabled{false};
    bool collide_connected{false};
    bool broken{false};
    float break_force{0.0f};      ///< 0 = unbreakable

    // cached solver state
    math::Vec2f r_a{}, r_b{};
    math::Vec2f normal{};
    float mass{0.0f};
    float bias{0.0f};
    float accumulated_impulse{0.0f};
};

class JointSolver {
public:
    explicit JointSolver(std::vector<PhysicsBody>& bodies) : bodies_(bodies) {}

    Joint& create(Joint j) {
        joints_.push_back(std::move(j));
        return joints_.back();
    }

    /// Solve all joints for one velocity substep (after contacts).
    void solve(float dt) {
        for (auto& j : joints_) {
            if (j.broken) continue;
            if (j.a >= bodies_.size() || j.b >= bodies_.size()) continue;
            switch (j.type) {
            case JointType::Distance: solve_distance(j, dt); break;
            case JointType::Revolute: solve_revolute(j, dt); break;
            case JointType::Prismatic: solve_prismatic(j, dt); break;
            case JointType::Weld:     solve_weld(j, dt); break;
            case JointType::Spring:   solve_spring(j, dt); break;
            }
        }
    }

    std::vector<Joint>& joints() noexcept { return joints_; }
    void clear() noexcept { joints_.clear(); }
    std::size_t size() const noexcept { return joints_.size(); }

private:
    std::vector<PhysicsBody>& bodies_;
    std::vector<Joint> joints_;

    float inv_mass(std::uint32_t i) const {
        const auto& b = bodies_[i];
        return (b.type == components::BodyType::Dynamic && b.awake)
            ? (b.mass > 0.0f ? 1.0f / b.mass : 0.0f) : 0.0f;
    }
    float inv_inertia(std::uint32_t i) const {
        const auto& b = bodies_[i];
        return (b.type == components::BodyType::Dynamic && !b.fixed_rotation)
            ? (b.inertia > 0.0f ? 1.0f / b.inertia : 0.0f) : 0.0f;
    }

    void solve_distance(Joint& j, float dt) {
        auto& A = bodies_[j.a];
        auto& B = bodies_[j.b];
        // world anchors
        const float ca = std::cos(A.rotation), sa = std::sin(A.rotation);
        const float cb = std::cos(B.rotation), sb = std::sin(B.rotation);
        j.r_a = Vec2f{ j.anchor_a.x * ca - j.anchor_a.y * sa,
                       j.anchor_a.x * sa + j.anchor_a.y * ca };
        j.r_b = Vec2f{ j.anchor_b.x * cb - j.anchor_b.y * sb,
                       j.anchor_b.x * sb + j.anchor_b.y * cb };
        const Vec2f p_a = A.position + j.r_a;
        const Vec2f p_b = B.position + j.r_b;
        Vec2f d = p_b - p_a;
        const float dist = d.length();
        if (dist < 1e-6f) { j.normal = Vec2f{1, 0}; }
        else j.normal = d * (1.0f / dist);

        // effective mass along the constraint axis
        const float im_a = inv_mass(j.a), im_b = inv_mass(j.b);
        const float ii_a = inv_inertia(j.a), ii_b = inv_inertia(j.b);
        const float rn_a = j.r_a.cross(j.normal);
        const float rn_b = j.r_b.cross(j.normal);
        const float k = im_a + im_b + ii_a * rn_a * rn_a + ii_b * rn_b * rn_b;
        j.mass = k > 1e-9f ? 1.0f / k : 0.0f;

        // Baumgarte positional bias
        const float C = dist - j.length;
        const float beta = 0.2f;
        j.bias = beta / dt * C;

        const Vec2f v_a = A.velocity + Vec2f{-A.angular_velocity * j.r_a.y, A.angular_velocity * j.r_a.x};
        const Vec2f v_b = B.velocity + Vec2f{-B.angular_velocity * j.r_b.y, B.angular_velocity * j.r_b.x};
        const float Cdot = (v_b - v_a).dot(j.normal);

        float impulse = -j.mass * (Cdot + j.bias);
        // accumulate & clamp for unbreakable joints
        const float old = j.accumulated_impulse;
        j.accumulated_impulse = std::max(0.0f, old + impulse);
        impulse = j.accumulated_impulse - old;

        if (j.break_force > 0.0f && std::abs(impulse) > j.break_force) {
            j.broken = true;
            return;
        }

        A.velocity -= j.normal * impulse * im_a;
        B.velocity += j.normal * impulse * im_b;
        A.angular_velocity -= ii_a * j.r_a.cross(j.normal * impulse);
        B.angular_velocity += ii_b * j.r_b.cross(j.normal * impulse);
    }

    void solve_revolute(Joint& j, float /*dt*/) {
        // Point-to-point constraint: keep world anchors coincident.
        auto& A = bodies_[j.a];
        auto& B = bodies_[j.b];
        const float ca = std::cos(A.rotation), sa = std::sin(A.rotation);
        const float cb = std::cos(B.rotation), sb = std::sin(B.rotation);
        j.r_a = Vec2f{ j.anchor_a.x * ca - j.anchor_a.y * sa,
                       j.anchor_a.x * sa + j.anchor_a.y * ca };
        j.r_b = Vec2f{ j.anchor_b.x * cb - j.anchor_b.y * sb,
                       j.anchor_b.x * sb + j.anchor_b.y * cb };
        const Vec2f p_a = A.position + j.r_a;
        const Vec2f p_b = B.position + j.r_b;
        const Vec2f C = p_b - p_a; // positional violation
        const Vec2f v_a = A.velocity + Vec2f{-A.angular_velocity * j.r_a.y, A.angular_velocity * j.r_a.x};
        const Vec2f v_b = B.velocity + Vec2f{-B.angular_velocity * j.r_b.y, B.angular_velocity * j.r_b.x};
        const Vec2f Cdot = v_b - v_a;

        // 2x2 effective mass matrix (approximate: diagonal)
        const float im_a = inv_mass(j.a), im_b = inv_mass(j.b);
        const float ii_a = inv_inertia(j.a), ii_b = inv_inertia(j.b);
        for (int axis = 0; axis < 2; ++axis) {
            Vec2f n = axis == 0 ? Vec2f{1,0} : Vec2f{0,1};
            const float rn_a = j.r_a.cross(n);
            const float rn_b = j.r_b.cross(n);
            const float k = im_a + im_b + ii_a * rn_a * rn_a + ii_b * rn_b * rn_b;
            if (k < 1e-9f) continue;
            const float bias = (axis == 0 ? C.x : C.y) * 0.2f * 60.0f; // Baumgarte
            float impulse = -(Cdot.dot(n) + bias) / k;
            A.velocity -= n * impulse * im_a;
            B.velocity += n * impulse * im_b;
            A.angular_velocity -= ii_a * j.r_a.cross(n * impulse);
            B.angular_velocity += ii_b * j.r_b.cross(n * impulse);
        }
    }

    void solve_prismatic(Joint& j, float /*dt*/) {
        // Constrain relative velocity perpendicular to the slide axis.
        auto& A = bodies_[j.a];
        auto& B = bodies_[j.b];
        const Vec2f axis = Vec2f{std::cos(j.axis_angle), std::sin(j.axis_angle)};
        const Vec2f perp = Vec2f{-axis.y, axis.x};
        const Vec2f rel = B.position - A.position;
        const Vec2f vrel = B.velocity - A.velocity;

        const float im_a = inv_mass(j.a), im_b = inv_mass(j.b);
        const float k = im_a + im_b;
        if (k < 1e-9f) return;

        const float C_perp = rel.dot(perp);
        const float bias = C_perp * 0.2f * 60.0f;
        const float Cdot_perp = vrel.dot(perp);
        float impulse = -(Cdot_perp + bias) / k;
        A.velocity -= perp * impulse * im_a;
        B.velocity += perp * impulse * im_b;

        // limits: clamp translation along axis
        if (j.limit_enabled) {
            const float translation = rel.dot(axis);
            if (translation < j.lower_limit) {
                const float Cdot = vrel.dot(axis);
                float imp = -(Cdot + (translation - j.lower_limit) * 0.2f * 60.0f) / k;
                A.velocity -= axis * imp * im_a;
                B.velocity += axis * imp * im_b;
            } else if (translation > j.upper_limit) {
                const float Cdot = vrel.dot(axis);
                float imp = -(Cdot + (translation - j.upper_limit) * 0.2f * 60.0f) / k;
                A.velocity -= axis * imp * im_a;
                B.velocity += axis * imp * im_b;
            }
        }

        // match rotations
        const float ang_err = B.rotation - A.rotation - j.angle_ref;
        const float ii = inv_inertia(j.a) + inv_inertia(j.b);
        if (ii > 1e-9f) {
            const float w_err = B.angular_velocity - A.angular_velocity;
            const float ang_impulse = -(w_err + ang_err * 0.2f * 60.0f) / ii;
            A.angular_velocity -= ang_impulse * inv_inertia(j.a);
            B.angular_velocity += ang_impulse * inv_inertia(j.b);
        }
    }

    void solve_weld(Joint& j, float dt) {
        solve_revolute(j, dt); // position
        // + lock rotation
        auto& A = bodies_[j.a];
        auto& B = bodies_[j.b];
        const float ii = inv_inertia(j.a) + inv_inertia(j.b);
        if (ii < 1e-9f) return;
        const float ang_err = B.rotation - A.rotation - j.angle_ref;
        const float w_err = B.angular_velocity - A.angular_velocity;
        const float impulse = -(w_err + ang_err * 0.2f * 60.0f) / ii;
        A.angular_velocity -= impulse * inv_inertia(j.a);
        B.angular_velocity += impulse * inv_inertia(j.b);
        (void)dt;
    }

    void solve_spring(Joint& j, float dt) {
        // Soft spring: frequency & damping-ratio formulation (Box2D-style).
        auto& A = bodies_[j.a];
        auto& B = bodies_[j.b];
        const float ca = std::cos(A.rotation), sa = std::sin(A.rotation);
        const float cb = std::cos(B.rotation), sb = std::sin(B.rotation);
        j.r_a = Vec2f{ j.anchor_a.x * ca - j.anchor_a.y * sa,
                       j.anchor_a.x * sa + j.anchor_a.y * ca };
        j.r_b = Vec2f{ j.anchor_b.x * cb - j.anchor_b.y * sb,
                       j.anchor_b.x * sb + j.anchor_b.y * cb };
        const Vec2f p_a = A.position + j.r_a;
        const Vec2f p_b = B.position + j.r_b;
        const Vec2f d = p_b - p_a;
        const float dist = d.length();
        if (dist < 1e-6f) return;
        const Vec2f n = d * (1.0f / dist);

        const float im_a = inv_mass(j.a), im_b = inv_mass(j.b);
        const float ii_a = inv_inertia(j.a), ii_b = inv_inertia(j.b);
        const float rn_a = j.r_a.cross(n);
        const float rn_b = j.r_b.cross(n);
        const float k = im_a + im_b + ii_a * rn_a * rn_a + ii_b * rn_b * rn_b;
        if (k < 1e-9f) return;
        const float mass = 1.0f / k;

        // soft-constraint coefficients
        const float omega = 2.0f * 3.14159265358979f * j.frequency;
        const float m = mass;
        const float c = 2.0f * mass * j.damping * omega;
        const float beta_dt = omega * dt;
        const float kk = m * beta_dt * beta_dt;
        const float bias = 0.0f;
        float gamma = m * 2.0f * j.damping * omega;
        gamma = gamma > 0.0f ? 1.0f / gamma : 0.0f;

        const Vec2f v_a = A.velocity + Vec2f{-A.angular_velocity * j.r_a.y, A.angular_velocity * j.r_a.x};
        const Vec2f v_b = B.velocity + Vec2f{-B.angular_velocity * j.r_b.y, B.angular_velocity * j.r_b.x};
        const float Cdot = (v_b - v_a).dot(n);
        const float C = dist - j.length;
        const float rhs = -Cdot - beta_dt * kk * C / (1.0f + dt * c * gamma + kk) - bias;

        float impulse = mass * rhs / (1.0f + gamma * c);
        A.velocity -= n * impulse * im_a;
        B.velocity += n * impulse * im_b;
        A.angular_velocity -= ii_a * j.r_a.cross(n * impulse);
        B.angular_velocity += ii_b * j.r_b.cross(n * impulse);
    }
};

} // namespace vengine::physics
