// V Engine 2.0 — Particle vertex shader (GPU billboards).
#version 300 es
precision highp float;

uniform mat4 u_mvp;
uniform vec2 u_size;
uniform float u_time;

in vec3 a_position;   // world position of particle
in vec4 a_color;      // tint + alpha
in float a_age;       // normalized 0..1

out vec4 v_color;

void main() {
    v_color = a_color;
    vec4 pos = u_mvp * vec4(a_position, 1.0);
    // billboard: expand by size in clip space, fade with age
    float fade = 1.0 - a_age;
    pos.xy += u_size * fade;
    gl_Position = pos;
}
