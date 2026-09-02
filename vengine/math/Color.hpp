#pragma once

#include <vengine/Common.hpp>
#include <vengine/core/Types.hpp>
#include <cstdint>

namespace vengine::math {

/// RGBA color. Stored as normalized floats for rendering and as packed u32
/// for serialization. Conversion helpers are inline and branch-free.
struct Color {
    float r{1.0f}, g{1.0f}, b{1.0f}, a{1.0f};

    constexpr Color() = default;
    constexpr Color(float r_, float g_, float b_, float a_ = 1.0f) noexcept
        : r(r_), g(g_), b(b_), a(a_) {}

    constexpr static Color rgb(u8 r_, u8 g_, u8 b_) noexcept {
        return Color{r_ / 255.0f, g_ / 255.0f, b_ / 255.0f, 1.0f};
    }
    constexpr static Color rgba(u8 r_, u8 g_, u8 b_, u8 a_) noexcept {
        return Color{r_ / 255.0f, g_ / 255.0f, b_ / 255.0f, a_ / 255.0f};
    }

    constexpr static Color white() noexcept { return Color{1, 1, 1, 1}; }
    constexpr static Color black() noexcept { return Color{0, 0, 0, 1}; }
    constexpr static Color transparent() noexcept { return Color{0, 0, 0, 0}; }
    constexpr static Color red()   noexcept { return Color{1, 0, 0, 1}; }
    constexpr static Color green() noexcept { return Color{0, 1, 0, 1}; }
    constexpr static Color blue()  noexcept { return Color{0, 0, 1, 1}; }

    constexpr bool operator==(const Color&) const noexcept = default;

    /// Pack into 0xAABBGGRR (little-endian RGBA8888).
    constexpr std::uint32_t pack() const noexcept {
        auto cl = [](float v) -> u8 {
            if (v <= 0.0f) return 0;
            if (v >= 1.0f) return 255;
            return static_cast<u8>(v * 255.0f + 0.5f);
        };
        return (static_cast<std::uint32_t>(cl(a)) << 24) |
               (static_cast<std::uint32_t>(cl(b)) << 16) |
               (static_cast<std::uint32_t>(cl(g)) <<  8) |
               (static_cast<std::uint32_t>(cl(r)));
    }

    constexpr static Color unpack(std::uint32_t p) noexcept {
        return Color{
            static_cast<float>((p      ) & 0xFF) / 255.0f,
            static_cast<float>((p >>  8) & 0xFF) / 255.0f,
            static_cast<float>((p >> 16) & 0xFF) / 255.0f,
            static_cast<float>((p >> 24) & 0xFF) / 255.0f,
        };
    }
};

} // namespace vengine::math
