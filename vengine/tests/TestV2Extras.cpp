// V Engine 2.0 — Tests for v2 extra modules: Vec4, Mat3, physics2 joints/
// filters/grid, net transport, editor panels.
#include <vengine/math/Vec3.hpp>
#include <vengine/math/Mat3.hpp>
#include <vengine/physics2/Joints.hpp>
#include <vengine/network/Transport.hpp>
#include <vengine/editor/DashboardPanels.hpp>
#include <vengine/input/InputExtensions.hpp>
#include <vengine/resources/Importers.hpp>

#include <gtest/gtest.h>

using namespace vengine;

TEST(Vec4, ArithmeticAndOps) {
    math::Vec4f a(1, 2, 3, 4), b(2, 3, 4, 5);
    auto sum = a + b;
    EXPECT_FLOAT_EQ(sum.x, 3.0f);
    EXPECT_FLOAT_EQ(sum.w, 9.0f);
    EXPECT_FLOAT_EQ(a.dot(b), 40.0f);
    auto scaled = a * 2.0f;
    EXPECT_FLOAT_EQ(scaled.z, 6.0f);
    EXPECT_FLOAT_EQ((b - a).w, 1.0f);
}

TEST(Mat3, TransformAndInverse) {
    auto t = math::Mat3::translation({5, -3});
    auto p = t.transform_point({1, 1});
    EXPECT_FLOAT_EQ(p.x, 6.0f);
    EXPECT_FLOAT_EQ(p.y, -2.0f);
    auto r = math::Mat3::rotation(1.5707963f);
    auto v = r.transform_vector({1, 0});
    EXPECT_NEAR(v.x, 0.0f, 1e-4f);
    EXPECT_NEAR(v.y, 1.0f, 1e-4f);
    auto inv = t.inverse();
    auto back = inv.transform_point(p);
    EXPECT_NEAR(back.x, 1.0f, 1e-4f);
    EXPECT_NEAR(back.y, 1.0f, 1e-4f);
    auto s = math::Mat3::scaling({2, 3});
    EXPECT_FLOAT_EQ(s.transform_vector({1, 1}).x, 2.0f);
}

TEST(Physics2, CollisionFilter) {
    physics2::CollisionFilter a, b;
    a.category_bits = 0x01; a.mask_bits = 0x02;
    b.category_bits = 0x02; b.mask_bits = 0x01;
    EXPECT_TRUE(a.can_collide(b));
    physics2::CollisionFilter c; c.category_bits = 0x04;
    EXPECT_FALSE(a.can_collide(c));
    a.group_index = 5; b.group_index = 5;
    EXPECT_TRUE(a.can_collide(b)); // same positive group
}

TEST(Physics2, GridBroadPhase) {
    physics2::GridBroadPhase grid(64.0f);
    grid.insert(1, physics2::AABB{0.0f, 0.0f, 32.0f, 32.0f});
    grid.insert(2, physics2::AABB{10.0f, 10.0f, 30.0f, 30.0f});
    grid.insert(3, physics2::AABB{500.0f, 500.0f, 20.0f, 20.0f});
    auto pairs = grid.pairs();
    ASSERT_FALSE(pairs.empty());
    bool found12 = false;
    for (auto& pr : pairs) if ((pr.first == 1 && pr.second == 2) || (pr.first == 2 && pr.second == 1)) found12 = true;
    EXPECT_TRUE(found12);
}

TEST(Net, ByteWriterReader) {
    net::ByteWriter w;
    w.u8(0xAB);
    w.u16(0x1234);
    w.u32(0xDEADBEEF);
    w.str("hello");
    net::ByteReader r(w.data());
    EXPECT_EQ(r.u8(), 0xAB);
    EXPECT_EQ(r.u16(), 0x1234);
    EXPECT_EQ(r.u32(), 0xDEADBEEFu);
    EXPECT_EQ(r.str(), "hello");
    EXPECT_TRUE(r.eof());
}

TEST(Net, ConnectionReliability) {
    net::Connection c("10.0.0.1:5000");
    net::Packet p{net::ChannelType::Reliable, 5, {}, false};
    c.send(p);
    c.on_ack(5);
    EXPECT_EQ(c.sent_total(), 1u);
    EXPECT_FLOAT_EQ(c.packet_loss(), 0.0f);
}

TEST(Editor, PanelsBuildWithoutThrowing) {
    editor::HierarchyPanel h;
    editor::InspectorPanel ins;
    editor::SceneViewPanel sv;
    editor::ConsolePanel con;
    editor::AssetBrowserPanel ab;
    editor::ToolbarPanel tb;
    con.log("test");
    ab.set_assets({"a.png", "b.wav"});
    std::vector<editor::PanelRow> rows;
    h.build(rows); ins.build(rows); sv.build(rows);
    con.build(rows); ab.build(rows); tb.build(rows);
    EXPECT_FALSE(rows.empty());
}

TEST(Importers, FailOnEmptySource) {
    resources::TextureImporter ti;
    auto r = ti.import_png("", "/tmp", false, {});
    EXPECT_FALSE(r.ok);
    EXPECT_FALSE(r.error.empty());
}

TEST(Importers, ImportPngSuccess) {
    resources::TextureImporter ti;
    auto r = ti.import_png("src.png", "/tmp/out", true, {});
    EXPECT_TRUE(r.ok);
    EXPECT_TRUE(r.compressed);
}
