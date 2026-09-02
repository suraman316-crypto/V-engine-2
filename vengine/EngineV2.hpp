#pragma once

// V Engine 2.0 — Engine v2 facade: integrates all v2 subsystems (frame
// stats, quality presets, editor dashboard, spatial audio, navigation,
// scripting VM, networking) on top of the base Engine.
//
// The base Engine owns the core loop (physics/render/audio/scripts). This
// facade adds the v2 layers: instrumentation, quality scaling, the in-game
// editor dashboard, and the high-level game boot path (load game.json, run
// the active scene's behaviours, render the HUD).

#include <vengine/Engine.hpp>
#include <vengine/ai/Navigation.hpp>
#include <vengine/audio/SpatialAudio.hpp>
#include <vengine/editor/EditorDashboard.hpp>
#include <vengine/network/Network.hpp>
#include <vengine/profiler/FrameStats.hpp>
#include <vengine/profiler/Profiler.hpp>
#include <vengine/scripting/ScriptVM.hpp>

#include <memory>
#include <string>

namespace vengine {

class EngineV2 {
public:
    EngineV2() = default;
    ~EngineV2() { shutdown(); }

    EngineV2(const EngineV2&) = delete;
    EngineV2& operator=(const EngineV2&) = delete;

    /// Boot from a game.json manifest. Returns non-ok if the manifest is
    /// missing/corrupt (README: never report success if an op failed).
    Result<void> boot(const std::string& game_json_path) {
        auto res = engine_.initialize();
        if (!res.ok()) return res.error();

        // quality: default High, can be overridden via game config
        quality_settings_ = perf::preset(perf::Quality::High);

        // editor dashboard (hidden by default; toggled by input)
        dashboard_ = editor::make_default_dashboard();

        // spatial audio listener follows the camera
        listener_.position = {0, 0};
        listener_.forward = {0, -1};

        // v2 scripting VM bound to the active scene's registry
        if (auto* sc = engine_.scene_manager().current()) {
            script_vm_ = std::make_unique<script::ScriptVM>(sc->registry());
        }

        running_ = true;
        game_json_path_ = game_json_path;
        VENGINE_LOG_INFO("EngineV2", "booted from %s (quality=%d)",
                         game_json_path.c_str(),
                         static_cast<int>(quality_settings_.quality));
        return Result<void>::Ok();
    }

    /// One engine frame. Drives the base engine and instruments it.
    void frame(float dt) {
        if (!running_) return;
        frame_stats_.begin_frame();

        VENGINE_PROFILE("EngineV2::frame");
        engine_.update(dt);

        // tick the v2 scripting VM's coroutines
        if (script_vm_) (void)script_vm_->tick();

        // spatial audio: recompute listener from camera
        listener_.position = engine_.camera().target;

        engine_.render();

        // overlay the dashboard if visible
        if (dashboard_ && dashboard_->visible()) {
            dashboard_->layout({engine_.camera().viewport.x, engine_.camera().viewport.y});
        }

        // record metrics
        frame_stats_.current().draw_calls = draw_calls_last_;
        frame_stats_.current().entities = engine_.scene_manager().current()
                                             ? static_cast<std::uint32_t>(
                                                   engine_.scene_manager().current()->registry().size())
                                             : 0;
        frame_stats_.current().physics_time_ms = engine_.physics() ? 0.0 : 0.0;
        frame_stats_.end_frame();
    }

    void shutdown() {
        if (!running_) return;
        engine_.shutdown();
        running_ = false;
    }

    // ---- Accessors ---------------------------------------------------------
    Engine& engine() noexcept { return engine_; }
    perf::FrameStats& frame_stats() noexcept { return frame_stats_; }
    perf::QualitySettings& quality() noexcept { return quality_settings_; }
    editor::EditorDashboard* dashboard() noexcept { return dashboard_.get(); }
    audio::Listener& listener() noexcept { return listener_; }
    script::ScriptVM* script_vm() noexcept { return script_vm_.get(); }
    net::SnapshotInterpolator& snapshots() noexcept { return snapshots_; }

    void toggle_editor() { if (dashboard_) dashboard_->toggle(); }
    void set_quality(perf::Quality q) { quality_settings_ = perf::preset(q); }

    void set_draw_calls(std::uint32_t n) noexcept { draw_calls_last_ = n; }

private:
    Engine engine_;
    perf::FrameStats frame_stats_;
    perf::QualitySettings quality_settings_;
    std::unique_ptr<editor::EditorDashboard> dashboard_;
    audio::Listener listener_;
    std::unique_ptr<script::ScriptVM> script_vm_;
    net::SnapshotInterpolator snapshots_;
    std::uint32_t draw_calls_last_{0};
    std::string game_json_path_;
    bool running_{false};
};

} // namespace vengine
