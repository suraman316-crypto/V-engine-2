#pragma once

#include <vengine/Common.hpp>
#include <vengine/core/Error.hpp>
#include <vengine/core/Types.hpp>

#include <memory>
#include <string>
#include <string_view>

namespace vengine::audio {

/// Audio bus identifiers. The engine routes sounds to buses; each bus has its
/// own volume and mute, all summed into the master.
enum class Bus : std::uint8_t { Master, Sfx, Music, Ui, Count };

/// Playback parameters for a one-shot or looping sound.
struct PlayParams {
    Bus    bus{Bus::Sfx};
    float  volume{1.0f};
    float  pitch{1.0f};
    float  pan{0.0f};        ///< -1 (left) .. +1 (right)
    bool   loop{false};
    float  fade_in_seconds{0.0f};
    /// 2D spatialization: world position + listener; ignored if zero distance.
    bool   spatialized{false};
    float  world_x{0.0f};
    float  world_y{0.0f};
    float  listener_x{0.0f};
    float  listener_y{0.0f};
    float  max_distance{500.0f};
};

/// An opaque handle to a playing voice; used to stop or fade it.
struct VoiceHandle { std::uint32_t id{0}; bool valid() const noexcept { return id != 0; } };

/// Abstract audio engine. The Android backend wraps Oboe behind this.
class IAudioEngine {
public:
    virtual ~IAudioEngine() = default;

    virtual Result<void> initialize() = 0;
    virtual void         shutdown() = 0;

    /// Load a sound/music asset from a path into memory/decoding state.
    virtual Result<void> load(std::string_view name, std::string_view path, bool streaming) = 0;
    virtual void         unload(std::string_view name) = 0;

    virtual VoiceHandle  play(std::string_view name, const PlayParams& params = {}) = 0;
    virtual void         stop(VoiceHandle v, float fade_out_seconds = 0.0f) = 0;

    virtual void         set_bus_volume(Bus bus, float volume) = 0;
    virtual void         set_bus_muted(Bus bus, bool muted) = 0;
    virtual float        bus_volume(Bus bus) const noexcept = 0;
    virtual bool         bus_muted(Bus bus) const noexcept = 0;

    virtual void         pause_all() = 0;
    virtual void         resume_all() = 0;
};

} // namespace vengine::audio
