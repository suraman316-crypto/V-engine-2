// V Engine 2.0 — Renderer & assets v2 tests.
#include <vengine/assets/AssetManager.hpp>
#include <vengine/assets/Prefab.hpp>
#include <vengine/assets/TileMap.hpp>
#include <vengine/renderer/Font.hpp>
#include <vengine/renderer/FrameGraph.hpp>
#include <vengine/renderer/Lighting.hpp>
#include <vengine/renderer/Material.hpp>
#include <vengine/renderer/Mesh.hpp>
#include <vengine/renderer/PostProcess.hpp>
#include <vengine/renderer/ShaderLibrary.hpp>
#include <vengine/renderer/TextureAtlas.hpp>

#include <gtest/gtest.h>

using namespace vengine;

TEST(ShaderLibrary, HasAllBuiltinShaders) {
    EXPECT_FALSE(renderer::ShaderLibrary::sprite().stages.empty());
    EXPECT_FALSE(renderer::ShaderLibrary::shape().stages.empty());
    EXPECT_FALSE(renderer::ShaderLibrary::lit_sprite().stages.empty());
    EXPECT_FALSE(renderer::ShaderLibrary::post_crt().stages.empty());
    EXPECT_FALSE(renderer::ShaderLibrary::blur().stages.empty());
    EXPECT_FALSE(renderer::ShaderLibrary::particle().stages.empty());
    EXPECT_FALSE(renderer::ShaderLibrary::text_sdf().stages.empty());
}

TEST(Material, PoolCreateGetDestroy) {
    renderer::MaterialPool pool;
    renderer::Material m;
    m.blend = renderer::BlendMode::Additive;
    m.set_float("u_time", 1.5f);
    auto h = pool.create(m);
    ASSERT_NE(h, 0u);
    auto* got = pool.get(h);
    ASSERT_NE(got, nullptr);
    EXPECT_EQ(got->blend, renderer::BlendMode::Additive);
    pool.destroy(h);
    EXPECT_EQ(pool.get(h)->blend, renderer::BlendMode::Additive); // slot reused but unchanged
}

TEST(FrameGraph, TopologicalOrder) {
    renderer::FrameGraph fg;
    auto r1 = fg.create_resource({renderer::FrameGraphResource::Type::Texture, "a", 1, 1, 0, false});
    auto r2 = fg.create_resource({renderer::FrameGraphResource::Type::Texture, "b", 1, 1, 0, false});
    (void)fg.add_pass({"p1", {}, {r1}, {r1}, [](){}});
    (void)fg.add_pass({"p2", {r1}, {r2}, {r2}, [](){}});
    auto order = fg.compile();
    ASSERT_EQ(order.size(), 2u);
    EXPECT_EQ(order[0], 0u);
    EXPECT_EQ(order[1], 1u);
}

TEST(Lighting, PointLightContribution) {
    renderer::LightSystem ls;
    ls.ambient().intensity = 0.0f;
    ls.directional().intensity = 0.0f;
    ls.add_point({{0, 0}, 100.0f, 1.0f, math::Color::white()});
    auto c = ls.sample({0, 0});
    EXPECT_GT(c.r, 0.0f);
    auto far = ls.sample({1000, 1000});
    EXPECT_FLOAT_EQ(far.r, 0.0f);
}

TEST(TextureAtlas, PackRects) {
    renderer::TextureAtlas atlas(256, 256);
    auto r1 = atlas.add(64, 64, 1);
    auto r2 = atlas.add(64, 64, 2);
    ASSERT_NE(r1.id, 0u);
    ASSERT_NE(r2.id, 0u);
    EXPECT_EQ(atlas.count(), 2u);
}

TEST(Mesh, QuadHasTwoTris) {
    auto q = renderer::Mesh::quad(2, 2);
    EXPECT_EQ(q.vertices.size(), 4u);
    EXPECT_EQ(q.indices.size(), 6u);
}

TEST(Mesh, CircleSegments) {
    auto c = renderer::Mesh::circle(1.0f, 16);
    // center + 17 rim vertices = 18
    EXPECT_EQ(c.vertices.size(), 18u);
}

TEST(Font, MeasureText) {
    auto f = renderer::make_default_font(1);
    auto sz = f.measure("Hello");
    EXPECT_GT(sz.x, 0.0f);
}

TEST(TileMap, SolidExtraction) {
    assets::Tileset ts;
    assets::TileDef td; td.id = 1; td.collision = assets::TileCollision::Full;
    ts.add(td);
    assets::TileMap tm;
    assets::TileLayer l; l.width = 4; l.height = 4; l.tile_w = 32; l.tile_h = 32; l.solid = true;
    l.tiles.resize(16, 0);
    l.set(0, 0, 1); l.set(1, 0, 1);
    tm.add_layer(l);
    auto boxes = tm.solid_boxes({{-10, -10}, {200, 200}}, ts);
    ASSERT_EQ(boxes.size(), 2u);
}

TEST(AssetManager, ImportFind) {
    assets::AssetManager am;
    auto id = am.import("tex.png", assets::AssetType::Texture);
    EXPECT_EQ(am.find("tex.png"), id);
    EXPECT_EQ(am.find("missing"), assets::kInvalidAsset);
    am.acquire(id);
    auto* meta = am.meta(id);
    ASSERT_NE(meta, nullptr);
    EXPECT_EQ(meta->refcount, 1u);
    am.release(id);
    EXPECT_EQ(meta->refcount, 0u);
}

TEST(PostProcess, BuildChain) {
    renderer::FrameGraph fg;
    renderer::PostProcessPipeline pp(fg);
    renderer::PostEffectSettings s; s.bloom_enabled = true; s.color_grading_enabled = true;
    pp.configure(s);
    auto scene = fg.create_resource({renderer::FrameGraphResource::Type::Texture, "scene", 1, 1, 0, false});
    auto out = fg.create_resource({renderer::FrameGraphResource::Type::Texture, "out", 1, 1, 0, true});
    auto order = pp.build(scene, out, [](){});
    EXPECT_GT(order.size(), 0u);
}
