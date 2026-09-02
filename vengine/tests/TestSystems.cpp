#include <vengine/input/Input.hpp>
#include <vengine/memory/ObjectPool.hpp>
#include <vengine/serialization/Serialization.hpp>
#include <vengine/particles/Particles.hpp>
#include <vengine/assets/AssetDatabase.hpp>

#include <gtest/gtest.h>

#include <cmath>
#include <cstdio>
#include <filesystem>

using namespace vengine;

TEST(Input, ActionMapping) {
    input::Input in;
    in.map_action("Jump", {input::Key::Space, input::Key::GamepadA});
    in.on_key_down(input::Key::Space);
    EXPECT_TRUE(in.is_action_pressed("Jump"));
    EXPECT_TRUE(in.is_action_just_pressed("Jump"));
    in.end_frame();
    EXPECT_TRUE(in.is_action_pressed("Jump"));
    EXPECT_FALSE(in.is_action_just_pressed("Jump")); // edge cleared
    in.on_key_up(input::Key::Space);
    EXPECT_TRUE(in.is_action_just_released("Jump"));
    in.end_frame();
    EXPECT_FALSE(in.is_action_pressed("Jump"));
}

TEST(Input, Axis) {
    input::Input in;
    in.map_axis("Horizontal", input::Key::Left, input::Key::Right);
    in.on_key_down(input::Key::Right);
    EXPECT_FLOAT_EQ(in.axis_value("Horizontal"), 1.0f);
    in.on_key_down(input::Key::Left);
    EXPECT_FLOAT_EQ(in.axis_value("Horizontal"), 0.0f); // both held cancel
    in.on_key_up(input::Key::Right);
    EXPECT_FLOAT_EQ(in.axis_value("Horizontal"), -1.0f);
}

TEST(Input, MultitouchAndPinch) {
    input::Input in;
    in.on_pointer_down(1, {100, 100}, {0.1f, 0.1f});
    in.on_pointer_down(2, {200, 200}, {0.2f, 0.2f});
    EXPECT_EQ(in.pointer_count(), 2u);
    EXPECT_FLOAT_EQ(in.pinch_distance(), std::sqrt(20000.0f));
    in.on_pointer_up(1);
    EXPECT_EQ(in.pointer_count(), 1u); // id2 still down
    in.on_pointer_up(2);
    in.end_frame();
    EXPECT_EQ(in.pointer_count(), 0u);
}

TEST(ObjectPool, AcquireReleaseReuses) {
    memory::ObjectPool<int> pool{4};
    EXPECT_EQ(pool.available(), 4u);
    int* a = pool.acquire(7);
    ASSERT_NE(a, nullptr);
    EXPECT_EQ(*a, 7);
    EXPECT_EQ(pool.available(), 3u);
    pool.release(a);
    EXPECT_EQ(pool.available(), 4u);
    // Exhaustion returns nullptr, never throws.
    int* all[4];
    for (int i = 0; i < 4; ++i) all[i] = pool.acquire(i);
    EXPECT_EQ(pool.available(), 0u);
    EXPECT_EQ(pool.acquire(99), nullptr);
    for (int i = 0; i < 4; ++i) pool.release(all[i]);
}

TEST(Json, ParseAndDumpRoundTrip) {
    std::string src = R"({"name":"test","count":3,"items":[1,2,3],"flag":true,"nil":null,"nested":{"x":1.5}})";
    auto parsed = serialization::parse_json(src);
    ASSERT_TRUE(parsed.ok()) << parsed.error().format();
    const auto& root = parsed.value();
    EXPECT_EQ(root.find("name")->as_string(), "test");
    EXPECT_EQ(root.find("count")->as_int(), 3);
    EXPECT_TRUE(root.find("flag")->as_bool());
    EXPECT_TRUE(root.find("nil")->is_null());
    EXPECT_EQ(root.find("items")->array().size(), 3u);
    EXPECT_NEAR(root.find("nested")->find("x")->as_float(), 1.5, 1e-9);

    // Re-dump and re-parse for stability.
    std::string again = serialization::dump_json(root);
    auto reparsed = serialization::parse_json(again);
    ASSERT_TRUE(reparsed.ok());
}

TEST(Json, RejectsMalformed) {
    auto r1 = serialization::parse_json("{not json}");
    ASSERT_FALSE(r1.ok());
    auto r2 = serialization::parse_json("{\"a\":1} trailing");
    ASSERT_FALSE(r2.ok());
    auto r3 = serialization::parse_json("[1,2,]");
    ASSERT_FALSE(r3.ok());
}

TEST(Particles, EmitAndAge) {
    particles::EmitterConfig cfg;
    cfg.max_particles = 16;
    cfg.lifetime = 0.5f;
    cfg.rate = 0.0f; // manual emit only
    particles::ParticleSystem ps{cfg};
    ps.emit({0, 0}, 5);
    EXPECT_EQ(ps.alive_count(), 5u);
    ps.update(0.6f); // all expire
    EXPECT_EQ(ps.alive_count(), 0u);
}

TEST(AssetDatabase, RoundTripAndMissing) {
    assets::AssetDatabase db;
    assets::AssetId id = db.next_id();
    assets::AssetRecord rec;
    rec.id = id;
    rec.type = assets::AssetType::Texture;
    rec.source_path = "assets/player.png";
    rec.imported_path = "imported/player.tex";
    rec.name = "player";
    db.upsert(rec);
    ASSERT_NE(db.find(id), nullptr);
    EXPECT_EQ(db.find_by_name("player")->source_path, "assets/player.png");

    auto path = (std::filesystem::temp_directory_path() / "vengine_assets.json").string();
    auto res = db.save(path);
    ASSERT_TRUE(res.ok()) << res.error().format();
    assets::AssetDatabase loaded;
    auto lres = loaded.load(path);
    ASSERT_TRUE(lres.ok()) << lres.error().format();
    ASSERT_NE(loaded.find(id), nullptr);
    EXPECT_EQ(loaded.find(id)->type, assets::AssetType::Texture);

    auto missing = loaded.find_missing([](std::string_view p) {
        return std::filesystem::exists(std::string(p));
    });
    EXPECT_EQ(missing.size(), 1u);
    std::remove(path.c_str());
}
