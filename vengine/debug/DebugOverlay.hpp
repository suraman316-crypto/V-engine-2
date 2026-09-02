#pragma once

// V Engine 2.0 — Debug overlay (Phase 9 extension): on-screen stats and debug
// draws (FPS, frame time, draw calls, entity count, profiling scopes) for the
// dashboard / dev builds. Toggled at runtime via show flag.
#include <cstdint>
#include <string>
#include <vector>

namespace vengine::debug {

struct OverlayStat {
    std::string label;
    std::string value;
};

class DebugOverlay {
public:
    bool show{true};
    bool show_fps{true};
    bool show_frame_time{true};
    bool show_draw_calls{true};
    bool show_entity_count{true};

    void set_fps(float fps) { fps_ = fps; }
    void set_frame_time_ms(float ms) { frame_ms_ = ms; }
    void set_draw_calls(std::uint32_t n) { draw_calls_ = n; }
    void set_entity_count(std::uint32_t n) { entity_count_ = n; }

    std::vector<OverlayStat> build() const {
        std::vector<OverlayStat> out;
        if (!show) return out;
        if (show_fps) out.push_back({"FPS", std::to_string(static_cast<int>(fps_))});
        if (show_frame_time) out.push_back({"Frame", std::to_string(frame_ms_) + " ms"});
        if (show_draw_calls) out.push_back({"Draws", std::to_string(draw_calls_)});
        if (show_entity_count) out.push_back({"Entities", std::to_string(entity_count_)});
        return out;
    }

    void push_scope(const std::string& name, float ms) {
        scopes_.push_back({name, std::to_string(ms) + " ms"});
        if (scopes_.size() > 32) scopes_.erase(scopes_.begin());
    }

private:
    float fps_{0};
    float frame_ms_{0};
    std::uint32_t draw_calls_{0};
    std::uint32_t entity_count_{0};
    std::vector<OverlayStat> scopes_;
};

} // namespace vengine::debug
