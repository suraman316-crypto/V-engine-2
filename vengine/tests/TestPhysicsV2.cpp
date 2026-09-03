// V Engine 2.0 — Physics v2 tests (joints, CCD, BVH, islands, filters, geometry).
#include <vengine/math/Geometry.hpp>
#include <vengine/math/Quat.hpp>
#include <vengine/math/Spline.hpp>
#include <vengine/math/Transform.hpp>
#include <vengine/physics/BVH.hpp>
#include <vengine/physics/CCD.hpp>
#include <vengine/physics/CollisionFilter.hpp>
#include <vengine/physics/IslandSolver.hpp>
#include <vengine/physics/Joints.hpp>
#include <vengine/physics/SpatialHash.hpp>

#include <gtest/gtest.h>

using namespace vengine;

TEST(BVH, InsertAndQuery) {
    physics::BVHTree bvh;
    auto a = bvh.insert({{0, 0}, {1, 1}}, 10);
    auto b = bvh.insert({{5, 5}, {6, 6}}, 20);
    (void)a; (void)b;
    std::vector<std::uint32_t> hits;
    bvh.query({{0, 0}, {2, 2}}, [&](std::uint32_t id) { hits.push_back(id); });
    ASSERT_EQ(hits.size(), 1u);
    EXPECT_EQ(hits[0], 10u);
}

TEST(SpatialHash, InsertAndQuery) {
    physics::SpatialHashGrid grid(10.0f);
    grid.insert(0, {{0, 0}, {5, 5}});
    grid.insert(1, {{20, 20}, {25, 25}});
    std::vector<std::uint32_t> hits;
    grid.query({{0, 0}, {6, 6}}, [&](std::uint32_t id) { hits.push_back(id); });
    ASSERT_EQ(hits.size(), 1u);
    EXPECT_EQ(hits[0], 0u);
}

TEST(CCD, SweepAabbHits) {
    // box moving from x=0 to x=10, target at x=8 -> hits at t=0.7
    float t = physics::sweep_aabb({{0, 0}, {1, 1}}, {10, 0}, {{8, 0}, {9, 1}});
    EXPECT_GT(t, 0.0f);
    EXPECT_LT(t, 1.0f);
}

TEST(CCD, SweepAabbMiss) {
    // box moving up, target far to the side -> no hit, returns 1
    float t = physics::sweep_aabb({{0, 0}, {1, 1}}, {0, 10}, {{100, 0}, {101, 1}});
    EXPECT_FLOAT_EQ(t, 1.0f);
}

TEST(CCD, SweepCircleOverlap) {
    // circles already overlapping (distance 1 < sum 2) -> TOI 0
    EXPECT_FLOAT_EQ(physics::sweep_circle({0, 0}, 1.0f, {10, 0}, {1, 0}, 1.0f), 0.0f);
}

TEST(CCD, SweepCircleTravel) {
    // start 5 apart, moving 10 units -> touches at distance 2, t = 3/10
    float t = physics::sweep_circle({0, 0}, 1.0f, {10, 0}, {5, 0}, 1.0f);
    EXPECT_NEAR(t, 0.3f, 0.001f);
}

TEST(CollisionFilter, LayerMask) {
    physics::CollisionFilter player{0x0002, 0xFFFFu};
    physics::CollisionFilter enemy{0x0004, 0xFFFFu};
    EXPECT_TRUE(physics::CollisionFilter::should_collide(player, enemy));
    physics::CollisionFilter none{0x0000, 0xFFFFu};
    EXPECT_FALSE(physics::CollisionFilter::should_collide(none, enemy));
}

TEST(IslandSolver, OneIsland) {
    physics::IslandGraph g;
    g.body_count = 4;
    g.add_edge(0, 1); g.add_edge(1, 2); g.add_edge(2, 3);
    auto islands = g.compute_islands();
    ASSERT_EQ(islands.size(), 1u);
    EXPECT_EQ(islands[0].size(), 4u);
}

TEST(IslandSolver, TwoIslands) {
    physics::IslandGraph g;
    g.body_count = 4;
    g.add_edge(0, 1); g.add_edge(2, 3);
    auto islands = g.compute_islands();
    EXPECT_EQ(islands.size(), 2u);
}

TEST(Geometry, SatOverlap) {
    auto a = math::Polygon::regular({0, 0}, 1.0f, 4);
    auto b = math::Polygon::regular({0.5f, 0}, 1.0f, 4);
    EXPECT_TRUE(math::sat_overlap(a, b));
    auto c = math::Polygon::regular({10, 10}, 1.0f, 4);
    EXPECT_FALSE(math::sat_overlap(a, c));
}

TEST(Quat, SlerpEndpoints) {
    auto q1 = math::Quat::from_axis_angle({0, 0, 1}, 0.0f);
    auto q2 = math::Quat::from_axis_angle({0, 0, 1}, 3.14159f);
    auto mid = math::Quat::slerp(q1, q2, 0.5f);
    EXPECT_NEAR(mid.x, 0.0f, 0.01f);
    EXPECT_NEAR(mid.y, 0.0f, 0.01f);
}

TEST(Spline, CatmullRomSampling) {
    math::CatmullRom cr({{0, 0}, {1, 0}, {2, 0}, {3, 0}});
    auto p = cr.sample(0.5f);
    EXPECT_FLOAT_EQ(p.y, 0.0f);
}

TEST(Transform, HierarchyUpdate) {
    math::TransformHierarchy h;
    auto parent = h.create(math::Transform2D{math::Vec2f{10, 0}, 0.0f, math::Vec2f{1, 1}});
    auto child = h.create(math::Transform2D{math::Vec2f{1, 0}, 0.0f, math::Vec2f{1, 1}}, parent);
    h.update();
    // child world should be at parent+child offset
    EXPECT_FLOAT_EQ(h.world(child).m[12], 11.0f);
}
