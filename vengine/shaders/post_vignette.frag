// V Engine 2.0 — Post-process: vignette + chromatic aberration.
#version 300 es
precision highp float;

uniform sampler2D u_scene;
uniform vec2 u_resolution;
uniform float u_vignette;
uniform float u_aberration;

in vec2 v_texcoord;
out vec4 frag_color;

void main() {
    vec2 uv = v_texcoord;
    vec2 dir = uv - 0.5;
    float dist = length(dir);
    // chromatic aberration: offset per channel
    float r = texture(u_scene, uv + dir * u_aberration).r;
    float g = texture(u_scene, uv).g;
    float b = texture(u_scene, uv - dir * u_aberration).b;
    float vig = smoothstep(0.8, 0.2, dist) * u_vignette + (1.0 - u_vignette);
    frag_color = vec4(vec3(r, g, b) * vig, 1.0);
}
