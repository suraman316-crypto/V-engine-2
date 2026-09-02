#pragma once

// V Engine 2.0 — Mesh resource: vertex/index buffers for 2D and 3D geometry.
//
// A Mesh holds typed vertex data (pos, uv, color, normal) and an index list.
// The backend uploads it to a VBO/IBO; this header is the CPU-side source
// of truth and the generator for common 2D primitive shapes.

#include <vengine/math/Color.hpp>
#include <vengine/math/Vec2.hpp>
#include <vengine/math/Vec3.hpp>

#include <cstdint>
#include <vector>

namespace vengine::renderer {

struct Vertex {
    math::Vec3f position{};
    math::Vec2f uv{};
    math::Color color{1.0f, 1.0f, 1.0f, 1.0f};
    math::Vec3f normal{0.0f, 0.0f, 1.0f};
};

class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;

    static Mesh quad(float w = 1.0f, float h = 1.0f) {
        const float hw = w * 0.5f, hh = h * 0.5f;
        Mesh m;
        m.vertices = {
            {{-hw, -hh, 0}, {0, 0}}, {{ hw, -hh, 0}, {1, 0}},
            {{ hw,  hh, 0}, {1, 1}}, {{-hw,  hh, 0}, {0, 1}},
        };
        m.indices = {0, 1, 2, 0, 2, 3};
        return m;
    }

    /// 9-slice mesh for scalable UI panels and sliced sprites.
    static Mesh nine_slice(float w, float h, float border) {
        Mesh m;
        const float hw = w * 0.5f, hh = h * 0.5f, b = border;
        const float lx = -hw, rx = hw, ty = -hh, by = hh;
        // 8x8 vertices forming 9 quads, but simplified: full quad is enough
        // for the CPU representation; the shader does the slicing via UVs.
        m.vertices = {
            {{lx, ty, 0}, {0, 0}}, {{-b, ty, 0}, {0.25f, 0}}, {{ b, ty, 0}, {0.75f, 0}}, {{rx, ty, 0}, {1, 0}},
            {{lx, -b, 0}, {0, 0.25f}}, {{-b, -b, 0}, {0.25f, 0.25f}}, {{ b, -b, 0}, {0.75f, 0.25f}}, {{rx, -b, 0}, {1, 0.25f}},
            {{lx, b, 0}, {0, 0.75f}}, {{-b, b, 0}, {0.25f, 0.75f}}, {{ b, b, 0}, {0.75f, 0.75f}}, {{rx, b, 0}, {1, 0.75f}},
            {{lx, by, 0}, {0, 1}}, {{-b, by, 0}, {0.25f, 1}}, {{ b, by, 0}, {0.75f, 1}}, {{rx, by, 0}, {1, 1}},
        };
        // 9 quads = 18 triangles
        auto quad_idx = [&](int a, int b2, int c, int d) {
            indices_push(m, a, b2, c, a, c, d);
        };
        // row-major 3x3 grid of quads
        int grid[4][4];
        int idx = 0;
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c)
                grid[r][c] = idx++;
        for (int r = 0; r < 3; ++r)
            for (int c = 0; c < 3; ++c)
                quad_idx(grid[r][c], grid[r][c+1], grid[r+1][c+1], grid[r+1][c]);
        return m;
    }

    static Mesh circle(float radius = 1.0f, std::size_t segments = 32) {
        Mesh m;
        m.vertices.push_back({{}, {0.5f, 0.5f}}); // center
        for (std::size_t i = 0; i <= segments; ++i) {
            const float a = (i / static_cast<float>(segments)) * 6.2831853f;
            m.vertices.push_back({
                {std::cos(a) * radius, std::sin(a) * radius, 0.0f},
                {std::cos(a) * 0.5f + 0.5f, std::sin(a) * 0.5f + 0.5f},
            });
        }
        for (std::size_t i = 1; i <= segments; ++i) {
            m.indices.push_back(0);
            m.indices.push_back(static_cast<std::uint32_t>(i));
            m.indices.push_back(static_cast<std::uint32_t>(i + 1));
        }
        return m;
    }

    /// Regular convex polygon (triangle fan).
    static Mesh polygon(float radius, std::size_t sides) {
        Mesh m;
        for (std::size_t i = 0; i < sides; ++i) {
            const float a = (i / static_cast<float>(sides)) * 6.2831853f;
            m.vertices.push_back({{std::cos(a) * radius, std::sin(a) * radius, 0.0f}, {0, 0}});
        }
        for (std::size_t i = 1; i + 1 < sides; ++i) {
            m.indices.push_back(0);
            m.indices.push_back(static_cast<std::uint32_t>(i));
            m.indices.push_back(static_cast<std::uint32_t>(i + 1));
        }
        return m;
    }

    /// Subdivide a quad mesh into a grid of NxN quads (for water/shaders).
    static Mesh grid(float w, float h, std::size_t segments_x, std::size_t segments_y) {
        Mesh m;
        const float hw = w * 0.5f, hh = h * 0.5f;
        for (std::size_t y = 0; y <= segments_y; ++y) {
            for (std::size_t x = 0; x <= segments_x; ++x) {
                const float u = x / static_cast<float>(segments_x);
                const float v = y / static_cast<float>(segments_y);
                m.vertices.push_back({
                    {(-hw + u * w), (-hh + v * h), 0.0f},
                    {u, v},
                });
            }
        }
        const std::uint32_t stride = static_cast<std::uint32_t>(segments_x + 1);
        for (std::size_t y = 0; y < segments_y; ++y) {
            for (std::size_t x = 0; x < segments_x; ++x) {
                const std::uint32_t i0 = static_cast<std::uint32_t>(y * stride + x);
                m.indices.push_back(i0);
                m.indices.push_back(i0 + stride);
                m.indices.push_back(i0 + stride + 1);
                m.indices.push_back(i0);
                m.indices.push_back(i0 + stride + 1);
                m.indices.push_back(i0 + 1);
            }
        }
        return m;
    }

private:
    static void indices_push(Mesh& m, std::uint32_t a, std::uint32_t b, std::uint32_t c, std::uint32_t d, std::uint32_t e, std::uint32_t f) {
        m.indices.push_back(a); m.indices.push_back(b); m.indices.push_back(c);
        m.indices.push_back(d); m.indices.push_back(e); m.indices.push_back(f);
    }
};

} // namespace vengine::renderer
