#pragma once

// V Engine 2.0 — Frame stats & performance counters: a rolling history of
// per-frame metrics (FPS, frame time, CPU/GPU time, draw calls, triangles,
// texture memory, physics time, audio time) for the editor profiler panel.

#include <vengine/Common.hpp>

#include <algorithm>
#include <cstdint>
#include <deque>
#include <string>
#include <unordered_map>

namespace vengine::perf {

struct FrameMetrics {
    double frame_time_ms{0.0};
    double cpu_time_ms{0.0};
    double gpu_time_ms{0.0};
    double physics_time_ms{0.0};
    double audio_time_ms{0.0};
    double render_time_ms{0.0};
    double script_time_ms{0.0};
    std::uint32_t draw_calls{0};
    std::uint32_t triangles{0};
    std::uint32_t vertices{0};
    std::uint32_t texture_binds{0};
    std::uint32_t buffer_binds{0};
    std::uint32_t entities{0};
    std::uint64_t texture_memory_kb{0};
    std::uint64_t total_memory_kb{0};
    std::uint64_t asset_memory_kb{0};
    float fps{0.0f};
};

/// Rolling ring buffer of frame metrics + aggregate stats.
class FrameStats {
public:
    explicit FrameStats(std::size_t history = 240) : max_history_(history) {}

    void begin_frame() { frame_start_ = now(); }
    void end_frame() {
        double ms = elapsed_ms(frame_start_);
        current_.frame_time_ms = ms;
        current_.fps = ms > 0.0 ? static_cast<float>(1000.0 / ms) : 0.0f;
        history_.push_back(current_);
        if (history_.size() > max_history_) history_.pop_front();
        current_ = {};
    }

    FrameMetrics& current() noexcept { return current_; }
    const FrameMetrics& last() const { return history_.empty() ? empty_ : history_.back(); }

    double avg_frame_ms(std::size_t n = 60) const {
        if (history_.empty()) return 0.0;
        double sum = 0.0; std::size_t c = 0;
        for (auto it = history_.rbegin(); it != history_.rend() && c < n; ++it, ++c) sum += it->frame_time_ms;
        return sum / std::max<std::size_t>(1, std::min(n, history_.size()));
    }
    float avg_fps(std::size_t n = 60) const {
        double ms = avg_frame_ms(n);
        return ms > 0.0 ? static_cast<float>(1000.0 / ms) : 0.0f;
    }
    double max_frame_ms(std::size_t n = 60) const {
        double m = 0.0; std::size_t c = 0;
        for (auto it = history_.rbegin(); it != history_.rend() && c < n; ++it, ++c) m = std::max(m, it->frame_time_ms);
        return m;
    }
    std::uint64_t total_draw_calls(std::size_t n = 60) const {
        std::uint64_t s = 0; std::size_t c = 0;
        for (auto it = history_.rbegin(); it != history_.rend() && c < n; ++it, ++c) s += it->draw_calls;
        return s;
    }
    std::size_t history_size() const noexcept { return history_.size(); }

    /// Named counter: arbitrary user counters (e.g. "particles", "enemies").
    void set_counter(const std::string& name, std::uint64_t v) { counters_[name] = v; }
    std::uint64_t counter(const std::string& name) const {
        auto it = counters_.find(name);
        return it == counters_.end() ? 0 : it->second;
    }

private:
    using clock = std::chrono::steady_clock;
    static clock::time_point now() { return clock::now(); }
    static double elapsed_ms(clock::time_point t) {
        return std::chrono::duration<double, std::milli>(clock::now() - t).count();
    }
    std::size_t max_history_;
    std::deque<FrameMetrics> history_;
    FrameMetrics current_;
    FrameMetrics empty_;
    clock::time_point frame_start_;
    std::unordered_map<std::string, std::uint64_t> counters_;
};

/// Quality presets: Low / Medium / High / Ultra + Custom, auto-recommended
/// from device capability score.
enum class Quality : u8 { Low, Medium, High, Ultra, Custom };

struct QualitySettings {
    Quality quality{Quality::High};
    int resolution_scale{100};     ///< percent of native
    int msaa{2};
    bool bloom{true};
    bool color_grading{true};
    bool vignette{true};
    bool shadows{true};
    int shadow_resolution{1024};
    int particle_budget{2000};
    int texture_filter{1};         ///< 0=nearest,1=bilinear,2=trilinear
    bool texture_compression{true};
    int target_fps{60};
    bool vsync{true};
    int physics_substeps{1};
    int audio_voices{32};
};

inline QualitySettings preset(Quality q) {
    switch (q) {
        case Quality::Low:    return {q, 75, 0, false, false, false, false, 0, 500, 0, true, 30, true, 1, 16};
        case Quality::Medium: return {q, 85, 2, true, true, false, true, 512, 1000, 1, true, 60, true, 1, 24};
        case Quality::High:   return {q, 100, 2, true, true, true, true, 1024, 2000, 1, true, 60, true, 1, 32};
        case Quality::Ultra:  return {q, 100, 4, true, true, true, true, 2048, 4000, 2, true, 60, true, 2, 48};
        default:              return {q, 100, 2, true, true, true, true, 1024, 2000, 1, true, 60, true, 1, 32};
    }
}

/// Recommend a quality preset from a device capability score [0,100].
inline Quality recommend_quality(float score) {
    if (score < 30.0f) return Quality::Low;
    if (score < 55.0f) return Quality::Medium;
    if (score < 80.0f) return Quality::High;
    return Quality::Ultra;
}

} // namespace vengine::perf
