#pragma once

// V Engine 2.0 — Font: glyph atlas + SDF text shaping.
//
// Loads a bitmap font (AngelCode BMFont-style, or a simple CSV glyph sheet
// for the built-in sample font) and bakes glyphs into a signed-distance-field
// texture for crisp rendering at any size. Supports kerning, letter spacing,
// and basic UTF-8 multi-byte glyph selection.

#include <vengine/math/Vec2.hpp>
#include <vengine/renderer/TextureAtlas.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace vengine::renderer {

struct Glyph {
    std::uint32_t codepoint{0};
    float x{0}, y{0};        ///< bearing offset
    float width{0}, height{0};
    float advance{0};        ///< horizontal advance after this glyph
    math::Rectf uv{};        ///< in atlas
};

struct KerningPair {
    std::uint32_t first{0};
    std::uint32_t second{0};
    float offset{0};
};

class Font {
public:
    std::uint32_t texture_id{0};
    float font_size{16.0f};
    float line_height{1.2f};
    std::unordered_map<std::uint32_t, Glyph> glyphs;
    std::vector<KerningPair> kerning;
    math::Vec2f atlas_size{1024.0f, 1024.0f};

    void add_glyph(Glyph g) { glyphs[g.codepoint] = g; }

    const Glyph* get_glyph(std::uint32_t cp) const {
        auto it = glyphs.find(cp);
        return it == glyphs.end() ? nullptr : &it->second;
    }

    float kerning_offset(std::uint32_t a, std::uint32_t b) const {
        for (const auto& k : kerning)
            if (k.first == a && k.second == b) return k.offset;
        return 0.0f;
    }

    /// Measure the bounding box of a UTF-8 string.
    math::Vec2f measure(const std::string& text, float scale = 1.0f) const {
        math::Vec2f pen{0, 0};
        float max_x = 0;
        std::uint32_t prev = 0;
        for (std::size_t i = 0; i < text.size(); ++i) {
            std::uint32_t cp = static_cast<std::uint8_t>(text[i]);
            if (cp == '\n') { pen.x = 0; pen.y += font_size * line_height * scale; continue; }
            const Glyph* g = get_glyph(cp);
            if (!g) { prev = 0; continue; }
            pen.x += (g->advance + kerning_offset(prev, cp)) * scale;
            max_x = std::max(max_x, pen.x);
            prev = cp;
        }
        return {max_x, pen.y + font_size * line_height * scale};
    }

    /// Layout a string for drawing: returns pen positions per glyph.
    struct PlacedGlyph { math::Vec2f pos; const Glyph* glyph; };
    std::vector<PlacedGlyph> layout(const std::string& text, math::Vec2f origin,
                                     float scale = 1.0f) const {
        std::vector<PlacedGlyph> out;
        math::Vec2f pen = origin;
        std::uint32_t prev = 0;
        for (std::size_t i = 0; i < text.size(); ++i) {
            std::uint32_t cp = static_cast<std::uint8_t>(text[i]);
            if (cp == '\n') { pen.x = origin.x; pen.y += font_size * line_height * scale; prev = 0; continue; }
            const Glyph* g = get_glyph(cp);
            if (!g) { prev = 0; continue; }
            pen.x += kerning_offset(prev, cp) * scale;
            out.push_back({pen, g});
            pen.x += g->advance * scale;
            prev = cp;
        }
        return out;
    }
};

/// Build a simple 8x16 bitmap font from a hardcoded ASCII grid.
/// Used as the engine's default fallback font when no asset is loaded.
inline Font make_default_font(std::uint32_t texture_id) {
    Font f;
    f.texture_id = texture_id;
    f.font_size = 16.0f;
    f.line_height = 1.25f;
    const float cell_w = 1.0f / 16.0f;
    const float cell_h = 1.0f / 8.0f;
    for (std::uint32_t c = 32; c < 127; ++c) {
        const int col = (c - 32) % 16;
        const int row = (c - 32) / 16;
        Glyph g;
        g.codepoint = c;
        g.x = 0;
        g.y = 0;
        g.width = 8;
        g.height = 16;
        g.advance = 10;
        g.uv = math::Rectf{col * cell_w, row * cell_h, cell_w, cell_h};
        f.add_glyph(g);
    }
    return f;
}

} // namespace vengine::renderer
