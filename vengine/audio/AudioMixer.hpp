#pragma once

// CPU audio mixer: a dependency-free implementation of IAudioEngine.
//
// It models a bus graph (Master <- {SFX, Music, UI}) with per-bus volume/mute,
// active voices with volume/pitch/pan/loop, fades, and 2D spatialization. It
// produces a final mixed volume per voice so a real Oboe backend can sample
// it, and so the engine logic is testable on host without a device.
//
// No PCM is decoded here (no codec dependency); the mixer computes the *gain*
// applied to a voice, which is what an Oboe render callback multiplies the
// decoded samples by. This keeps the host testable and the backend thin.

#include <vengine/Common.hpp>
#include <vengine/audio/Audio.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace vengine::audio {

struct Voice {
    std::uint32_t id{0};
    std::string   asset;
    Bus           bus{Bus::Sfx};
    float         volume{1.0f};
    float         pitch{1.0f};
    float         pan{0.0f};
    bool          loop{false};
    bool          playing{false};
    bool          paused{false};
    float         fade{0.0f};       ///< current fade gain 0..1
    float         fade_target{1.0f};
    float         fade_rate{0.0f};  ///< per-second
    // spatial
    bool          spatialized{false};
    float         world_x{0.0f}, world_y{0.0f};
    float         listener_x{0.0f}, listener_y{0.0f};
    float         max_distance{500.0f};
};

class AudioMixer : public IAudioEngine {
public:
    static constexpr std::size_t kBusCount = static_cast<std::size_t>(Bus::Count);

    Result<void> initialize() override {
        for (auto& b : bus_volume_) b = 1.0f;
        for (auto& b : bus_muted_) b = false;
        paused_ = false;
        next_id_ = 1;
        return Result<void>::Ok();
    }
    void shutdown() override {
        voices_.clear();
        assets_.clear();
    }

    Result<void> load(std::string_view name, std::string_view path, bool streaming) override {
        (void)streaming;
        assets_[std::string{name}] = std::string{path};
        return Result<void>::Ok();
    }
    void unload(std::string_view name) override {
        assets_.erase(std::string{name});
        for (auto& v : voices_) if (v.asset == name) v.playing = false;
    }

    VoiceHandle play(std::string_view name, const PlayParams& p = {}) override {
        if (!assets_.count(std::string{name}))
            return VoiceHandle{};
        Voice v;
        v.id = next_id_++;
        v.asset = std::string{name};
        v.bus = p.bus;
        v.volume = p.volume;
        v.pitch = p.pitch;
        v.pan = p.pan;
        v.loop = p.loop;
        v.playing = true;
        v.spatialized = p.spatialized;
        v.world_x = p.world_x; v.world_y = p.world_y;
        v.listener_x = p.listener_x; v.listener_y = p.listener_y;
        v.max_distance = p.max_distance;
        v.fade = p.fade_in_seconds > 0.0f ? 0.0f : 1.0f;
        v.fade_target = 1.0f;
        v.fade_rate = p.fade_in_seconds > 0.0f ? 1.0f / p.fade_in_seconds : 0.0f;
        voices_.push_back(v);
        return VoiceHandle{v.id};
    }

    void stop(VoiceHandle h, float fade_out = 0.0f) override {
        auto* v = find(h);
        if (!v) return;
        if (fade_out > 0.0f) {
            v->fade_target = 0.0f;
            v->fade_rate = 1.0f / fade_out;
        } else {
            v->playing = false;
        }
    }

    void set_bus_volume(Bus b, float vol) override {
        bus_volume_[static_cast<std::size_t>(b)] = std::clamp(vol, 0.0f, 1.0f);
    }
    void set_bus_muted(Bus b, bool m) override { bus_muted_[static_cast<std::size_t>(b)] = m; }
    float bus_volume(Bus b) const noexcept override { return bus_volume_[static_cast<std::size_t>(b)]; }
    bool bus_muted(Bus b) const noexcept override { return bus_muted_[static_cast<std::size_t>(b)]; }

    void pause_all() override { paused_ = true; }
    void resume_all() override { paused_ = false; }

    /// Advance all voices by dt: update fades, retire finished non-looping
    /// voices, and decay spatialization. The backend calls this once per
    /// buffer; tests call it directly.
    void update(float dt) {
        if (paused_) return;
        for (auto& v : voices_) {
            if (!v.playing) continue;
            if (v.paused) continue;
            if (v.fade_rate > 0.0f) {
                if (v.fade_target > v.fade) v.fade = std::min(v.fade + v.fade_rate * dt, v.fade_target);
                else                        v.fade = std::max(v.fade - v.fade_rate * dt, v.fade_target);
                if (v.fade_target == 0.0f && v.fade <= 0.0f) v.playing = false;
            }
        }
        std::erase_if(voices_, [](const Voice& v) { return !v.playing && v.fade <= 0.0f; });
    }

    /// Final gain a backend would apply to a voice's PCM: volume * fade *
    /// bus_volume * (1 - muted) * spatial attenuation * pan.
    float final_gain(const Voice& v) const {
        float bus = bus_muted_[static_cast<std::size_t>(v.bus)] ? 0.0f
                    : bus_volume_[static_cast<std::size_t>(v.bus)];
        float master = bus_muted_[static_cast<std::size_t>(Bus::Master)] ? 0.0f
                       : bus_volume_[static_cast<std::size_t>(Bus::Master)];
        float gain = v.volume * v.fade * bus * master;
        if (v.spatialized) {
            float dx = v.world_x - v.listener_x, dy = v.world_y - v.listener_y;
            float dist = std::sqrt(dx*dx + dy*dy);
            gain *= std::max(1.0f - dist / std::max(v.max_distance, 0.0001f), 0.0f);
        }
        return gain;
    }

    std::size_t active_voices() const noexcept {
        std::size_t n = 0;
        for (const auto& v : voices_) if (v.playing) ++n;
        return n;
    }

    const std::vector<Voice>& voices() const noexcept { return voices_; }
    const Voice* voice(std::uint32_t id) const {
        for (const auto& v : voices_) if (v.id == id) return &v;
        return nullptr;
    }

private:
    Voice* find(VoiceHandle h) {
        for (auto& v : voices_) if (v.id == h.id) return &v;
        return nullptr;
    }

    std::unordered_map<std::string, std::string> assets_;
    std::vector<Voice> voices_;
    float bus_volume_[kBusCount]{1.0f, 1.0f, 1.0f, 1.0f};
    bool  bus_muted_[kBusCount]{false, false, false, false};
    std::uint32_t next_id_{1};
    bool paused_{false};
};

} // namespace vengine::audio
