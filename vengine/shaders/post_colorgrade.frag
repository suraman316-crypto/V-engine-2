// V Engine 2.0 — Post-process: color grading LUT + tone mapping (ACES).
#version 300 es
precision highp float;

uniform sampler2D u_scene;
uniform sampler2D u_lut;
uniform float u_exposure;

in vec2 v_texcoord;
out vec4 frag_color;

vec3 aces_tonemap(vec3 c) {
    const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
    return clamp((c * (a * c + b)) / (d * (a * c + b) + e), 0.0, 1.0);
}

void main() {
    vec3 col = texture(u_scene, v_texcoord).rgb * u_exposure;
    col = aces_tonemap(col);
    // 1D LUT lookup for color grading
    float r = texture(u_lut, vec2(col.r, 0.5)).r;
    float g = texture(u_lut, vec2(col.g, 0.5)).g;
    float b = texture(u_lut, vec2(col.b, 0.5)).b;
    frag_color = vec4(vec3(r, g, b), 1.0);
}
