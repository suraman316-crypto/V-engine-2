// V Engine 2.0 — Engine v2 facade + integration tests.
#include <vengine/EngineV2.hpp>
#include <vengine/memory/ObjectPool.hpp>

#include <gtest/gtest.h>

using namespace vengine;

TEST(EngineV2, BootHeadless) {
    EngineV2 e;
    auto res = e.boot("sample_game/game.json");
    // headless (no platform installed) — boot still returns ok since the
    // base engine tolerates a missing window.
    EXPECT_TRUE(res.ok());
    e.set_quality(perf::Quality::Medium);
    EXPECT_EQ(e.quality().target_fps, 60);
    e.frame(0.016f);
    EXPECT_GT(e.frame_stats().history_size(), 0u);
    e.toggle_editor();
    EXPECT_TRUE(e.dashboard()->visible());
    e.shutdown();
}

TEST(EngineV2, QualityPresets) {
    EXPECT_EQ(perf::preset(perf::Quality::Low).msaa, 0);
    EXPECT_EQ(perf::preset(perf::Quality::Ultra).msaa, 4);
    EXPECT_EQ(perf::recommend_quality(20.0f), perf::Quality::Low);
    EXPECT_EQ(perf::recommend_quality(90.0f), perf::Quality::Ultra);
}

TEST(EngineV2, FrameStatsRolling) {
    perf::FrameStats fs(10);
    for (int i = 0; i < 15; ++i) {
        fs.begin_frame();
        fs.current().draw_calls = 100 + i;
        fs.end_frame();
    }
    EXPECT_EQ(fs.history_size(), 10u);
    EXPECT_EQ(fs.last().draw_calls, 114u);
    EXPECT_GT(fs.avg_fps(), 0.0f);
    fs.set_counter("enemies", 42);
    EXPECT_EQ(fs.counter("enemies"), 42u);
}

TEST(HandlePool, CreateGetDestroy) {
    memory::HandlePool<int> hp(8);
    auto h = hp.create(123);
    auto* p = hp.get(h);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(*p, 123);
    hp.destroy(h);
    EXPECT_EQ(hp.get(h), nullptr);
    // recycled slot reuses the index but bumps generation, so old handle invalid
    auto h2 = hp.create(456);
    ASSERT_NE(hp.get(h2), nullptr);
    EXPECT_EQ(*hp.get(h2), 456);
    EXPECT_EQ(hp.get(h), nullptr); // stale handle still rejected
}

TEST(HandlePool, StaleGenerationRejected) {
    memory::HandlePool<int> hp(4);
    auto h = hp.create(1);
    hp.destroy(h);
    // bump generation by reusing same slot
    auto h2 = hp.create(2);
    // h's generation is now stale
    EXPECT_EQ(hp.get(h), nullptr);
    (void)h2;
}
