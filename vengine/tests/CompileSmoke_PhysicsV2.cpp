// Compile-only smoke test for the v2 physics modules (no gtest).
#include <vengine/physics/BVH.hpp>
#include <vengine/physics/CCD.hpp>
#include <vengine/physics/CollisionFilter.hpp>
#include <vengine/physics/IslandSolver.hpp>
#include <vengine/physics/Joints.hpp>
#include <vengine/physics/SpatialHash.hpp>

#include <vengine/math/Geometry.hpp>
#include <vengine/math/Quat.hpp>
#include <vengine/math/Spline.hpp>
#include <vengine/math/Transform.hpp>

int main() {
    using namespace vengine;

    // BVH
    physics::BVHTree bvh;
    auto leaf = bvh.insert({{0,0},{1,1}}, 0);
    (void)leaf;
    bvh.query({{0,0},{2,2}}, [](std::uint32_t){});

    // Spatial hash
    physics::SpatialHashGrid grid(10.0f);
    grid.insert(0, {{0,0},{5,5}});
    grid.query({{0,0},{3,3}}, [](std::uint32_t){});

    // CCD
    physics::sweep_aabb({{0,0},{1,1}}, {5,0}, {{8,0},{9,1}});
    physics::sweep_circle({0,0}, 0.5f, {10,0}, {5,0}, 0.5f);

    // Filter
    physics::CollisionFilter f;
    physics::CollisionFilter::should_collide(f, f);

    // Island
    physics::IslandGraph g;
    g.body_count = 3;
    g.add_edge(0,1); g.add_edge(1,2);
    auto islands = g.compute_islands();
    (void)islands;

    // Joints (needs bodies)
    std::vector<physics::PhysicsBody> bodies(2);
    physics::JointSolver js(bodies);
    physics::Joint j; j.a = 0; j.b = 1; j.type = physics::JointType::Distance;
    js.create(j);
    js.solve(1.0f/60.0f);

    // Geometry
    math::Circle c{{0,0}, 1.0f};
    (void)c.contains({0,0});
    math::Polygon poly = math::Polygon::regular({0,0}, 1.0f, 4);
    math::sat_overlap(poly, poly);

    // Quat
    math::Quat q = math::Quat::from_axis_angle({0,0,1}, 1.5707f);
    q.rotate({1,0,0});
    math::Quat::slerp(q, math::Quat::identity(), 0.5f);

    // Spline
    math::CatmullRom cr({{0,0},{1,0},{2,1},{3,0}});
    cr.sample(0.5f);
    cr.sample_uniform(0.5f);

    // Transform
    math::TransformHierarchy h;
    auto n = h.create({});
    h.set_local(n, math::Transform2D{math::Vec2f{1,0}, 0.0f, math::Vec2f{1,1}});
    h.update();

    return 0;
}
