#pragma once

// V Engine 2.0 — Material system: GPU pipeline state + shader parameters.
//
// A Material bundles a shader program, blend mode, cull mode, depth state,
// and a set of named uniforms/samplers. Materials are pooled and referenced
// by MaterialHandle so thousands of sprites can share one material.

#include <vengine/math/Color.hpp>
#include <vengine/math/Vec2.hpp>
#include <vengine/math/Vec3.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace vengine::renderer {

using math::Vec4f;

enum class BlendMode : u8 {
    Opaque,       ///< No blending (src*1 + dst*0).
    Alpha,        ///< Classic alpha: src*src.a + dst*(1-src.a).
    Additive,     ///< src*1 + dst*1 (glow, particles, fire).
    Multiply,     ///< src*dst (darken, glass).
    Screen,       ///< 1 - (1-src)*(1-dst) (lighten).
    Premultiplied,///< src*1 + dst*(1-src.a).
};

enum class CullMode : u8 { None, Back, Front };
enum class DepthTest : u8 { None, Less, LessEqual, Greater, Never, Always };
enum class StencilOp : u8 { Keep, Replace, Increment, Decrement, Invert };

/// A uniform value: floats, ints, vectors, colors, matrices — one at a time.
using UniformValue = std::variant<
    float, int, std::uint32_t, bool,
    math::Vec2f, math::Vec3f, math::Vec4f,
    math::Color
>;

struct Material {
    std::uint32_t shader_id{0};
    BlendMode blend{BlendMode::Alpha};
    CullMode  cull{CullMode::None};
    DepthTest depth{DepthTest::None};
    bool      wireframe{false};
    bool      depth_write{false};
    float     alpha_cutoff{0.0f}; ///< 0 = no alpha-testing
    StencilOp stencil_pass{StencilOp::Keep};
    std::uint8_t stencil_ref{0};

    std::unordered_map<std::string, UniformValue> uniforms;
    std::unordered_map<std::string, std::uint32_t> samplers; ///< slot -> texture id

    void set_float(std::string name, float v)        { uniforms[std::move(name)] = v; }
    void set_int(std::string name, int v)            { uniforms[std::move(name)] = v; }
    void set_vec2(std::string name, math::Vec2f v)  { uniforms[std::move(name)] = v; }
    void set_vec3(std::string name, math::Vec3f v)  { uniforms[std::move(name)] = v; }
    void set_color(std::string name, math::Color v) { uniforms[std::move(name)] = v; }
    void bind_texture(std::string slot, std::uint32_t tex) { samplers[std::move(slot)] = tex; }

    /// Sort key for batching: pack blend(4) + shader(20) + texture(8) bits.
    std::uint64_t sort_key() const noexcept {
        return (static_cast<std::uint64_t>(blend) << 32)
             | (static_cast<std::uint64_t>(shader_id & 0xFFFFF) << 8)
             | (static_cast<std::uint64_t>(samplers.empty() ? 0 : samplers.begin()->second & 0xFF));
    }
};

/// Pool of materials keyed by handle id.
class MaterialPool {
public:
    std::uint32_t create(Material m = {}) {
        std::uint32_t id = 0;
        if (!free_list_.empty()) {
            id = free_list_.back();
            free_list_.pop_back();
            materials_[id] = std::move(m);
        } else {
            id = static_cast<std::uint32_t>(materials_.size());
            materials_.push_back(std::move(m));
        }
        return id + 1; // 0 = invalid
    }

    Material* get(std::uint32_t handle) {
        if (handle == 0 || handle > materials_.size()) return nullptr;
        return &materials_[handle - 1];
    }
    const Material* get(std::uint32_t handle) const {
        if (handle == 0 || handle > materials_.size()) return nullptr;
        return &materials_[handle - 1];
    }

    void destroy(std::uint32_t handle) {
        if (handle == 0 || handle > materials_.size()) return;
        free_list_.push_back(handle - 1);
    }

    std::size_t size() const noexcept { return materials_.size() - free_list_.size(); }

private:
    std::vector<Material> materials_;
    std::vector<std::uint32_t> free_list_;
};

} // namespace vengine::renderer
