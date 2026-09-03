// V Engine 2.0 — Post-process: gaussian blur (separable).
#version 300 es
precision highp float;

uniform sampler2D u_scene;
uniform vec2 u_texel_size; // 1/resolution
uniform vec2 u_direction;  // (1,0) horizontal, (0,1) vertical
uniform float u_radius;

in vec2 v_texcoord;
out vec4 frag_color;

void main() {
    vec2 dir = u_direction * u_texel_size * u_radius;
    vec3 sum = vec3(0.0);
    float weights[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);
    sum += texture(u_scene, v_texcoord).rgb * weights[0];
    for (int i = 1; i < 5; ++i) {
        sum += texture(u_scene, v_texcoord + dir * float(i)).rgb * weights[i];
        sum += texture(u_scene, v_texcoord - dir * float(i)).rgb * weights[i];
    }
    frag_color = vec4(sum, 1.0);
}
