// V Engine 2.0 — CRT/scanline post effect (retro sample game).
#version 300 es
precision highp float;

uniform sampler2D u_scene;
uniform vec2 u_resolution;
uniform float u_scanline_strength;
uniform float u_curvature;

in vec2 v_texcoord;
out vec4 frag_color;

void main() {
    vec2 uv = v_texcoord;
    vec2 center = uv - 0.5;
    // barrel distortion
    float r2 = dot(center, center);
    uv = 0.5 + center * (1.0 + u_curvature * r2);
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        frag_color = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    vec3 col = texture(u_scene, uv).rgb;
    // scanlines
    float scan = sin(uv.y * u_resolution.y * 3.14159) * 0.5 + 0.5;
    col *= 1.0 - u_scanline_strength * scan;
    // slight RGB shift for CRT feel
    col.r = texture(u_scene, uv + vec2(1.0/u_resolution.x, 0)).r;
    col.b = texture(u_scene, uv - vec2(1.0/u_resolution.x, 0)).b;
    frag_color = vec4(col, 1.0);
}
