#pragma once

// V Engine 2.0 — Device capability detection: scores a device on a [0,100]
// scale from reported specs (CPU cores, RAM, GL version, screen DPI) so the
// engine can auto-recommend a quality preset (Phase 19).

#include <vengine/Common.hpp>

#include <algorithm>

namespace vengine::device {

struct DeviceInfo {
    int cpu_cores{4};
    std::uint64_t ram_mb{2048};
    int gl_version{3};          ///< major GLES version
    int screen_dpi{320};
    int screen_width{1920};
    int screen_height{1080};
    bool has_gles3{true};
    bool low_power{false};
};

/// Compute a capability score [0,100]. Weighted: RAM 40%, CPU 25%, GPU 25%, DPI 10%.
inline float capability_score(const DeviceInfo& d) {
    float ram_score = std::min(1.0f, static_cast<float>(d.ram_mb) / 8192.0f);
    float cpu_score = std::min(1.0f, static_cast<float>(d.cpu_cores) / 8.0f);
    float gpu_score = d.has_gles3 ? (d.gl_version >= 3 ? 1.0f : 0.5f) : 0.2f;
    float dpi_score = std::min(1.0f, static_cast<float>(d.screen_dpi) / 640.0f);
    if (d.low_power) gpu_score *= 0.6f;
    return (ram_score * 0.40f + cpu_score * 0.25f + gpu_score * 0.25f + dpi_score * 0.10f) * 100.0f;
}

/// Categorize a device into a tier.
enum class Tier { LowEnd, MidRange, HighEnd, Flagship };

inline Tier tier(const DeviceInfo& d) {
    float s = capability_score(d);
    if (s < 30.0f) return Tier::LowEnd;
    if (s < 55.0f) return Tier::MidRange;
    if (s < 80.0f) return Tier::HighEnd;
    return Tier::Flagship;
}

inline const char* tier_name(Tier t) {
    switch (t) {
        case Tier::LowEnd:   return "Low-end";
        case Tier::MidRange:  return "Mid-range";
        case Tier::HighEnd:   return "High-end";
        case Tier::Flagship:  return "Flagship";
    }
    return "Unknown";
}

} // namespace vengine::device
