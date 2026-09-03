// V Engine 2.0 — Phase 19 modules: device capability, battery/thermal,
// build pipeline, asset streaming, frame stats quality presets.
#include <vengine/assets/AssetStreaming.hpp>
#include <vengine/platform/BatteryThermal.hpp>
#include <vengine/platform/BuildPipeline.hpp>
#include <vengine/platform/DeviceCapability.hpp>
#include <vengine/profiler/FrameStats.hpp>

#include <gtest/gtest.h>

using namespace vengine;

TEST(DeviceCapability, ScoringAndTier) {
    device::DeviceInfo low{2, 1024, 2, 240, 1280, 720, false, true};
    device::DeviceInfo high{8, 8192, 3, 640, 2560, 1440, true, false};
    EXPECT_LT(device::capability_score(low), 40.0f);
    EXPECT_GT(device::capability_score(high), 80.0f);
    EXPECT_EQ(device::tier(low), device::Tier::LowEnd);
    EXPECT_EQ(device::tier(high), device::Tier::Flagship);
    EXPECT_STREQ(device::tier_name(device::Tier::HighEnd), "High-end");
}

TEST(BatteryThermal, ReducesFpsUnderPressure) {
    power::BatteryThermalManager m;
    power::PowerState critical{10, false, 0.9f, 0.0f};
    m.update(critical, 5.0f);
    EXPECT_EQ(m.target_fps(60), 30);   // battery < 15 → half FPS
    EXPECT_TRUE(m.should_reduce_quality()); // thermal > 0.7
    EXPECT_FALSE(m.should_pause());    // thermal 0.9 < 0.95 pause threshold
    power::PowerState normal{100, true, 0.0f, 0.3f};
    m.update(normal, 5.0f);
    EXPECT_EQ(m.target_fps(60), 60);
}

TEST(FramePacer, SleepTime) {
    power::FramePacer p(60);
    EXPECT_NEAR(p.target_frame_ms(), 16.666f, 0.1f);
    EXPECT_GT(p.sleep_time(10.0f), 0.0f);
    EXPECT_EQ(p.sleep_time(20.0f), 0.0f);
}

TEST(BuildPipeline, ValidatesConfig) {
    build::BuildConfig bad;
    bad.application_id = "";
    auto issues = build::BuildPipeline::validate(bad);
    EXPECT_FALSE(issues.empty());
}

TEST(BuildPipeline, RunsBuildWithProgress) {
    build::BuildConfig cfg;
    cfg.signing.configured = true;
    int calls = 0;
    build::BuildPipeline p;
    auto res = p.run(cfg, [&](const build::BuildProgress&){ ++calls; });
    EXPECT_TRUE(res.ok);
    EXPECT_FALSE(res.output_path.empty());
    EXPECT_GT(calls, 0);
}

TEST(BuildPipeline, FailsOnInvalidConfig) {
    build::BuildConfig bad;
    bad.application_id = "";
    build::BuildPipeline p;
    auto res = p.run(bad, {});
    EXPECT_FALSE(res.ok);
    EXPECT_FALSE(res.errors.empty());
}

TEST(AssetCache, LruEviction) {
    stream::AssetCache cache(100);
    cache.acquire("a", 40);
    cache.acquire("b", 40);
    EXPECT_EQ(cache.cached_count(), 2u);
    cache.acquire("c", 40);  // should evict "a"
    EXPECT_EQ(cache.cached_count(), 2u);
    EXPECT_LE(cache.used(), 100u);
}

TEST(AsyncLoader, EnqueueAndStop) {
    std::atomic<int> loaded{0};
    stream::AsyncLoader loader([&](const std::string&){ loaded++; return true; });
    loader.enqueue("x", stream::Priority::High);
    loader.enqueue("y", stream::Priority::Low);
    // give the worker a moment
    for (int i = 0; i < 100 && loaded < 2; ++i) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    EXPECT_GE(loaded, 1);
}
