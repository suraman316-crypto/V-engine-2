// V Engine 2.0 — Post-process: FXAA 3.11 (console-quality edge AA).
#version 300 es
precision highp float;

uniform sampler2D u_scene;
uniform vec2 u_texel_size; // 1/resolution
uniform float u_subpix;    // 0.75
uniform float u_edge_threshold; // 0.166
uniform float u_edge_threshold_min; // 0.0833

in vec2 v_texcoord;
out vec4 frag_color;

void main() {
    vec3 rgbM = texture(u_scene, v_texcoord).rgb;
    vec3 rgbN = texture(u_scene, v_texcoord + vec2(0, -u_texel_size.y)).rgb;
    vec3 rgbS = texture(u_scene, v_texcoord + vec2(0,  u_texel_size.y)).rgb;
    vec3 rgbW = texture(u_scene, v_texcoord + vec2(-u_texel_size.x, 0)).rgb;
    vec3 rgbE = texture(u_scene, v_texcoord + vec2( u_texel_size.x, 0)).rgb;
    float lumaM = dot(rgbM, vec3(0.299, 0.587, 0.114));
    float lumaN = dot(rgbN, vec3(0.299, 0.587, 0.114));
    float lumaS = dot(rgbS, vec3(0.299, 0.587, 0.114));
    float lumaW = dot(rgbW, vec3(0.299, 0.587, 0.114));
    float lumaE = dot(rgbE, vec3(0.299, 0.587, 0.114));
    float rangeMin = min(lumaM, min(min(lumaN, lumaW), min(lumaS, lumaE)));
    float rangeMax = max(lumaM, max(max(lumaN, lumaW), max(lumaS, lumaE)));
    float range = rangeMax - rangeMin;
    if (range < max(u_edge_threshold_min, rangeMax * u_edge_threshold)) {
        frag_color = vec4(rgbM, 1.0);
        return;
    }
    // simple horizontal/vertical blend toward neighbors
    float edgeH = abs(-2.0*lumaW + 2.0*lumaE) + abs(-2.0*lumaN + lumaM) + abs(-2.0*lumaS + lumaM);
    float edgeV = abs(-2.0*lumaN + 2.0*lumaS) + abs(-2.0*lumaW + lumaM) + abs(-2.0*lumaE + lumaM);
    bool horizontal = edgeH > edgeV;
    vec3 blend = mix(rgbN, rgbS, 0.5);
    if (horizontal) blend = mix(rgbW, rgbE, 0.5);
    frag_color = vec4(mix(rgbM, blend, u_subpix * 0.5), 1.0);
}
