#include <vengine/audio/AudioMixer.hpp>
#include <vengine/math/Random.hpp>
#include <vengine/particles/Particles.hpp>
#include <vengine/renderer/Camera2D.hpp>
#include <vengine/renderer/RenderCommand.hpp>
#include <vengine/ui/UI.hpp>

#include <gtest/gtest.h>

using namespace vengine;
using namespace vengine::math;
using namespace vengine::audio;
using namespace vengine::particles;
using namespace vengine::renderer;
using namespace vengine::ui;

// ---------------------------------------------------------------------------
// Renderer
// ---------------------------------------------------------------------------

TEST(Renderer, SortsByLayerThenTexture) {
    RenderCommandBuffer buf;
    SpriteCommand a; a.layer = 0; a.texture = 5; a.order_in_layer = 0;
    SpriteCommand b; b.layer = 0; b.texture = 2; b.order_in_layer = 0;
    SpriteCommand c; c.layer = 1; c.texture = 9; c.order_in_layer = 0;
    buf.submit(a); buf.submit(b); buf.submit(c);
    std::vector<SpriteBatch> batches;
    std::size_t n = buf.bake(batches);
    ASSERT_EQ(n, 3u);
    EXPECT_EQ(batches[0].layer, 0);
    EXPECT_EQ(batches[1].layer, 0);
    EXPECT_EQ(batches[2].layer, 1);
    // Within layer 0, texture 2 sorts before texture 5.
    EXPECT_EQ(batches[0].texture, 2u);
    EXPECT_EQ(batches[1].texture, 5u);
}

TEST(Renderer, BatchesSameTextureIntoOneDraw) {
    RenderCommandBuffer buf;
    SpriteCommand s; s.texture = 7; s.layer = 0;
    buf.submit(s); buf.submit(s); buf.submit(s);
    std::vector<SpriteBatch> batches;
    std::size_t n = buf.bake(batches);
    ASSERT_EQ(n, 1u);
    EXPECT_EQ(batches[0].quad_count, 3u);
    EXPECT_EQ(batches[0].vertices.size(), 3u * 4u * 8u); // 3 quads * 4 verts * 8 floats
}

TEST(Renderer, CullRemovesOffscreenSprites) {
    RenderCommandBuffer buf;
    SpriteCommand s; s.transform.position = Vec2f{1000, 1000};
    buf.submit(s);
    AABB view{Vec2f{-100,-100}, Vec2f{100,100}};
    EXPECT_EQ(buf.cull(view), 1u);
    EXPECT_EQ(buf.sprite_count(), 0u);
}

TEST(Renderer, CameraScreenToWorldRoundTrip) {
    Camera2D cam;
    cam.viewport = Vec2f{800, 600};
    Rng rng{1};
    Vec2f screen{400, 300}; // center
    Vec2f world = cam.screen_to_world(screen, rng);
    EXPECT_NEAR(world.x, 0.0f, 0.01f);
    EXPECT_NEAR(world.y, 0.0f, 0.01f);
}

TEST(Renderer, CameraShakeAddsOffset) {
    Camera2D cam;
    cam.position = Vec2f{0, 0};
    cam.add_shake(10.0f);
    Rng rng{42};
    cam.update(0.001f, rng);
    Vec2f p = cam.shaken_position(rng);
    // Should be within magnitude of origin.
    EXPECT_LE(p.length(), 10.0f);
}

// ---------------------------------------------------------------------------
// Audio mixer
// ---------------------------------------------------------------------------

TEST(Audio, PlayRoutesThroughBusVolume) {
    AudioMixer mix;
    mix.initialize();
    mix.load("sfx", "path", false);
    mix.set_bus_volume(Bus::Sfx, 0.5f);
    PlayParams p; p.bus = Bus::Sfx; p.volume = 1.0f;
    VoiceHandle h = mix.play("sfx", p);
    ASSERT_TRUE(h.valid());
    ASSERT_EQ(mix.active_voices(), 1u);
    const Voice* vp = mix.voice(h.id);
    ASSERT_TRUE(vp);
    // master(1) * sfx(0.5) * voice(1) * fade(1) = 0.5
    EXPECT_NEAR(mix.final_gain(*vp), 0.5f, 1e-3f);
}

TEST(Audio, MutedBusSilences) {
    AudioMixer mix;
    mix.initialize();
    mix.load("mus", "p", true);
    mix.set_bus_muted(Bus::Music, true);
    VoiceHandle h = mix.play("mus", PlayParams{Bus::Music, 1.0f, 1.0f, 0.0f, true});
    const Voice* vp = mix.voice(h.id); ASSERT_TRUE(vp);
    EXPECT_EQ(mix.final_gain(*vp), 0.0f);
}

TEST(Audio, FadeOutStopsVoice) {
    AudioMixer mix;
    mix.initialize();
    mix.load("s", "p", false);
    VoiceHandle h = mix.play("s", {});
    mix.stop(h, 0.1f);
    mix.update(0.2f);
    EXPECT_EQ(mix.active_voices(), 0u);
}

TEST(Audio, SpatialAttenuationDistant) {
    AudioMixer mix;
    mix.initialize();
    mix.load("s", "p", false);
    PlayParams p; p.spatialized = true; p.world_x = 400; p.max_distance = 500;
    VoiceHandle h = mix.play("s", p);
    const Voice* vp = mix.voice(h.id); ASSERT_TRUE(vp);
    // distance 400 / max 500 -> gain *= 0.2
    EXPECT_NEAR(mix.final_gain(*vp), 0.2f, 0.01f);
}

// ---------------------------------------------------------------------------
// Particles
// ---------------------------------------------------------------------------

TEST(Particles, EmitsAndDiesOverLifetime) {
    EmitterConfig cfg;
    cfg.rate = 0; cfg.lifetime = 0.5f; cfg.max_particles = 64; cfg.gravity = Vec2f{0,0};
    ParticleSystem ps(cfg);
    ps.emit(Vec2f{0,0}, 10);
    EXPECT_EQ(ps.alive_count(), 10u);
    for (int i = 0; i < 100; ++i) ps.update(0.01f, Vec2f{0,0});
    EXPECT_EQ(ps.alive_count(), 0u);
}

TEST(Particles, RespectsMaxParticles) {
    EmitterConfig cfg;
    cfg.rate = 0; cfg.lifetime = 10.0f; cfg.max_particles = 8;
    ParticleSystem ps(cfg);
    ps.emit(Vec2f{0,0}, 100);
    EXPECT_LE(ps.alive_count(), 8u);
}

// ---------------------------------------------------------------------------
// UI
// ---------------------------------------------------------------------------

TEST(UI, ButtonClickFiresOnTouchUp) {
    Canvas c;
    c.root().rect = Rectf{0, 0, 800, 600};
    int clicks = 0;
    auto btn = make_button("start", "Start", Rectf{100, 100, 200, 80},
                           [&](Widget&) { ++clicks; });
    c.add(c.root(), btn);
    c.layout_pass();
    Vec2f center{200, 140};
    c.touch_down(center);
    c.touch_up(center);
    EXPECT_EQ(clicks, 1);
}

TEST(UI, HitTestReturnsDeepest) {
    Canvas c;
    c.root().rect = Rectf{0, 0, 800, 600};
    auto panel = make_panel("p", Rectf{10, 10, 400, 400});
    auto& p = c.add(c.root(), panel);
    auto label = make_label("l", "hi", Rectf{5, 5, 100, 30});
    c.add(p, label);
    c.layout_pass();
    // point at (20,20) is inside panel(10-410). Label is non-touchable, so the
    // panel (the deepest *touchable* widget) wins.
    Widget* hit = c.hit_test(Vec2f{20, 20});
    ASSERT_TRUE(hit);
    EXPECT_EQ(hit->id, "p");
}

TEST(UI, SliderDragUpdatesValue) {
    Canvas c;
    c.root().rect = Rectf{0, 0, 800, 600};
    auto slider = make_slider("vol", Rectf{100, 100, 200, 20}, 0.0f, 100.0f, 0.0f);
    Widget& s = c.add(c.root(), slider);
    c.layout_pass();
    // drag to x=200 (center of slider 100..300) -> ~50
    c.touch_down(Vec2f{200, 110});
    EXPECT_NEAR(s.value, 50.0f, 1.0f);
}

TEST(UI, ToggleFlipsOnTap) {
    Canvas c;
    c.root().rect = Rectf{0, 0, 800, 600};
    auto tog = make_toggle("mute", "Mute", Rectf{100, 100, 80, 80}, false);
    Widget& t = c.add(c.root(), tog);
    c.layout_pass();
    Vec2f ctr{140, 140};
    c.touch_down(ctr);
    c.touch_up(ctr);
    EXPECT_TRUE(t.toggled);
    c.touch_down(ctr);
    c.touch_up(ctr);
    EXPECT_FALSE(t.toggled);
}
