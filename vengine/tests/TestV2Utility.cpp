// V Engine 2.0 — Tests for v2 utility modules: AudioBus, BinaryStream,
// DebugOverlay, Tilemap.
#include <vengine/audio/AudioBus.hpp>
#include <vengine/serialization/BinaryStream.hpp>
#include <vengine/debug/DebugOverlay.hpp>
#include <vengine/renderer/Tilemap.hpp>

#include <gtest/gtest.h>

using namespace vengine;

TEST(AudioBus, VolumeAndMute) {
    audio::BusGraph g;
    auto sfx = g.add_bus("sfx", 0);
    g.bus(sfx).volume = 0.5f;
    EXPECT_FLOAT_EQ(g.resolved_volume(sfx), 0.5f);
    g.bus(sfx).muted = true;
    EXPECT_FLOAT_EQ(g.resolved_volume(sfx), 0.0f);
}

TEST(AudioBus, SoloRouting) {
    audio::BusGraph g;
    auto sfx = g.add_bus("sfx", 0);
    auto music = g.add_bus("music", 0);
    g.bus(sfx).soloed = true;
    EXPECT_GT(g.resolved_volume(sfx), 0.0f);
    EXPECT_FLOAT_EQ(g.resolved_volume(music), 0.0f);
}

TEST(AudioBus, Duck) {
    audio::BusGraph g;
    auto music = g.add_bus("music", 0);
    g.set_duck(music, 0.8f);
    EXPECT_NEAR(g.resolved_volume(music), 0.2f, 1e-5f);
}

TEST(BinaryStream, RoundTrip) {
    serialization::BinaryWriter w;
    w.write_u32(123456);
    w.write_i32(-42);
    w.write_f32(3.14f);
    w.write_string("engine");
    serialization::BinaryReader r(w.data());
    EXPECT_EQ(r.read_u32(), 123456u);
    EXPECT_EQ(r.read_i32(), -42);
    EXPECT_FLOAT_EQ(r.read_f32(), 3.14f);
    EXPECT_EQ(r.read_string(), "engine");
    EXPECT_TRUE(r.eof());
}

TEST(DebugOverlay, BuildStats) {
    debug::DebugOverlay ov;
    ov.set_fps(59.5f);
    ov.set_frame_time_ms(16.8f);
    ov.set_draw_calls(120);
    ov.set_entity_count(4096);
    auto stats = ov.build();
    EXPECT_FALSE(stats.empty());
    ov.show = false;
    EXPECT_TRUE(ov.build().empty());
}

TEST(Tilemap, LayersAndCulling) {
    tiles::Tilemap tm;
    tm.resize(100, 50);
    auto bg = tm.add_layer("background", "tileset_0.png");
    auto& l = tm.layer(bg);
    l.tiles[0].id = 5;
    EXPECT_EQ(tm.tile_at(bg, 0, 0).id, 5u);
    EXPECT_EQ(tm.tile_at(bg, 99, 49).id, 0u);
    auto vis = tm.visible_tiles(math::Rectf{0, 0, 320, 320}, 32);
    EXPECT_FALSE(vis.empty());
}
