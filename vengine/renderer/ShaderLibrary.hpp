#pragma once

// V Engine 2.0 — Shader system: GLSL source management + a built-in
// shader library (sprite, text, shape, post-process, lit-sprite).
//
// The engine doesn't ship a GLSL compiler; instead it stores named
// vertex+fragment source pairs and a uniform-reflection table. A concrete
// backend (GLES) compiles+links them and caches the program id.

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace vengine::renderer {

struct ShaderStage {
    std::string source;
    bool is_vertex{true};
};

struct ShaderProgram {
    std::string name;
    std::vector<ShaderStage> stages;
    std::vector<std::string> uniforms;   ///< names the backend should reflect
    std::vector<std::string> attributes;
};

/// Built-in shaders used by the engine's default render paths.
class ShaderLibrary {
public:
    /// Sprite shader: textured quad with vertex color & UV.
    static ShaderProgram sprite() {
        return {
            "vengine/sprite",
            {
                {R"GLSL(#version 300 es
precision highp float;
uniform mat4 u_mvp;
layout(location=0) in vec2 a_pos;
layout(location=1) in vec2 a_uv;
layout(location=2) in vec4 a_color;
out vec2 v_uv;
out vec4 v_color;
void main() {
    v_uv = a_uv;
    v_color = a_color;
    gl_Position = u_mvp * vec4(a_pos, 0.0, 1.0);
})GLSL", true},
                {R"GLSL(#version 300 es
precision highp float;
uniform sampler2D u_tex;
in vec2 v_uv;
in vec4 v_color;
out vec4 frag;
void main() {
    frag = texture(u_tex, v_uv) * v_color;
})GLSL", false},
            },
            {"u_mvp", "u_tex"},
            {"a_pos", "a_uv", "a_color"},
        };
    }

    /// Shape shader: solid-color lines/circles/polygons for debug draw.
    static ShaderProgram shape() {
        return {
            "vengine/shape",
            {
                {R"GLSL(#version 300 es
precision highp float;
uniform mat4 u_mvp;
layout(location=0) in vec2 a_pos;
layout(location=1) in vec4 a_color;
out vec4 v_color;
void main() {
    v_color = a_color;
    gl_Position = u_mvp * vec4(a_pos, 0.0, 1.0);
})GLSL", true},
                {R"GLSL(#version 300 es
precision highp float;
in vec4 v_color;
out vec4 frag;
void main() { frag = v_color; })GLSL", false},
            },
            {"u_mvp"},
            {"a_pos", "a_color"},
        };
    }

    /// Text shader: SDF-distance-field text rendering.
    static ShaderProgram text_sdf() {
        return {
            "vengine/text_sdf",
            {
                {R"GLSL(#version 300 es
precision highp float;
uniform mat4 u_mvp;
layout(location=0) in vec2 a_pos;
layout(location=1) in vec2 a_uv;
layout(location=2) in vec4 a_color;
out vec2 v_uv;
out vec4 v_color;
void main() {
    v_uv = a_uv;
    v_color = a_color;
    gl_Position = u_mvp * vec4(a_pos, 0.0, 1.0);
})GLSL", true},
                {R"GLSL(#version 300 es
precision highp float;
uniform sampler2D u_tex;
uniform vec4 u_color;
in vec2 v_uv;
in vec4 v_color;
out vec4 frag;
void main() {
    float dist = texture(u_tex, v_uv).a;
    float alpha = smoothstep(0.5 - 0.1, 0.5 + 0.1, dist);
    frag = v_color * vec4(u_color.rgb, alpha);
})GLSL", false},
            },
            {"u_mvp", "u_tex", "u_color"},
            {"a_pos", "a_uv", "a_color"},
        };
    }

    /// Lit sprite: directional light + ambient term for 2D normal-mapped sprites.
    static ShaderProgram lit_sprite() {
        return {
            "vengine/lit_sprite",
            {
                {R"GLSL(#version 300 es
precision highp float;
uniform mat4 u_mvp;
layout(location=0) in vec2 a_pos;
layout(location=1) in vec2 a_uv;
layout(location=2) in vec4 a_color;
layout(location=3) in vec3 a_normal;
out vec2 v_uv;
out vec4 v_color;
out vec3 v_normal;
void main() {
    v_uv = a_uv;
    v_color = a_color;
    v_normal = a_normal;
    gl_Position = u_mvp * vec4(a_pos, 0.0, 1.0);
})GLSL", true},
                {R"GLSL(#version 300 es
precision highp float;
uniform sampler2D u_tex;
uniform sampler2D u_normal;
uniform vec3 u_light_dir;
uniform vec3 u_light_color;
uniform float u_ambient;
in vec2 v_uv;
in vec4 v_color;
in vec3 v_normal;
out vec4 frag;
void main() {
    vec4 albedo = texture(u_tex, v_uv) * v_color;
    vec3 n = normalize(texture(u_normal, v_uv).xyz * 2.0 - 1.0);
    float d = max(dot(n, normalize(u_light_dir)), 0.0);
    vec3 lit = albedo.rgb * (u_ambient + u_light_color * d);
    frag = vec4(lit, albedo.a);
})GLSL", false},
            },
            {"u_mvp", "u_tex", "u_normal", "u_light_dir", "u_light_color", "u_ambient"},
            {"a_pos", "a_uv", "a_color", "a_normal"},
        };
    }

    /// Post-process: vignette + color grading + scanline (CRT effect).
    static ShaderProgram post_crt() {
        return {
            "vengine/post_crt",
            {
                {R"GLSL(#version 300 es
precision highp float;
layout(location=0) in vec2 a_pos;
layout(location=1) in vec2 a_uv;
out vec2 v_uv;
void main() {
    v_uv = a_uv;
    gl_Position = vec4(a_pos, 0.0, 1.0);
})GLSL", true},
                {R"GLSL(#version 300 es
precision highp float;
uniform sampler2D u_tex;
uniform float u_time;
uniform float u_vignette;
uniform vec3 u_tint;
in vec2 v_uv;
out vec4 frag;
void main() {
    vec4 col = texture(u_tex, v_uv);
    // scanline
    float scan = 0.9 + 0.1 * sin(v_uv.y * 800.0 + u_time * 30.0);
    col.rgb *= scan;
    // vignette
    vec2 d = v_uv - 0.5;
    float v = 1.0 - smoothstep(0.4, 0.7, length(d)) * u_vignette;
    col.rgb *= v;
    col.rgb *= u_tint;
    frag = col;
})GLSL", false},
            },
            {"u_tex", "u_time", "u_vignette", "u_tint"},
            {"a_pos", "a_uv"},
        };
    }

    /// Blur (gaussian, separable) for bloom/UI soft shadows.
    static ShaderProgram blur() {
        return {
            "vengine/blur",
            {
                {R"GLSL(#version 300 es
precision highp float;
layout(location=0) in vec2 a_pos;
layout(location=1) in vec2 a_uv;
out vec2 v_uv;
void main() {
    v_uv = a_uv;
    gl_Position = vec4(a_pos, 0.0, 1.0);
})GLSL", true},
                {R"GLSL(#version 300 es
precision highp float;
uniform sampler2D u_tex;
uniform vec2 u_texel;
uniform float u_intensity;
in vec2 v_uv;
out vec4 frag;
void main() {
    vec4 sum = vec4(0.0);
    sum += texture(u_tex, v_uv + vec2(-3.0, 0.0) * u_texel) * 0.06136;
    sum += texture(u_tex, v_uv + vec2(-2.0, 0.0) * u_texel) * 0.24477;
    sum += texture(u_tex, v_uv + vec2(-1.0, 0.0) * u_texel) * 0.38774;
    sum += texture(u_tex, v_uv) * 0.06136;
    sum += texture(u_tex, v_uv + vec2( 1.0, 0.0) * u_texel) * 0.38774;
    sum += texture(u_tex, v_uv + vec2( 2.0, 0.0) * u_texel) * 0.24477;
    sum += texture(u_tex, v_uv + vec2( 3.0, 0.0) * u_texel) * 0.06136;
    frag = sum * u_intensity;
})GLSL", false},
            },
            {"u_tex", "u_texel", "u_intensity"},
            {"a_pos", "a_uv"},
        };
    }

    /// Particle shader: point sprites with rotation + soft edge.
    static ShaderProgram particle() {
        return {
            "vengine/particle",
            {
                {R"GLSL(#version 300 es
precision highp float;
uniform mat4 u_mvp;
layout(location=0) in vec2 a_pos;
layout(location=1) in vec2 a_uv;
layout(location=2) in vec4 a_color;
out vec2 v_uv;
out vec4 v_color;
void main() {
    v_uv = a_uv;
    v_color = a_color;
    gl_Position = u_mvp * vec4(a_pos, 0.0, 1.0);
})GLSL", true},
                {R"GLSL(#version 300 es
precision highp float;
uniform sampler2D u_tex;
in vec2 v_uv;
in vec4 v_color;
out vec4 frag;
void main() {
    vec4 t = texture(u_tex, v_uv);
    float soft = smoothstep(0.0, 0.5, 1.0 - length(v_uv - 0.5) * 2.0);
    frag = t * v_color * soft;
})GLSL", false},
            },
            {"u_mvp", "u_tex"},
            {"a_pos", "a_uv", "a_color"},
        };
    }
};

} // namespace vengine::renderer
