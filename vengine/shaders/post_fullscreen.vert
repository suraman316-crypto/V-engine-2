// V Engine 2.0 — Fullscreen post-process vertex shader (GLES 300 es).
#version 300 es
precision highp float;

in vec2 a_position; // [-1,1] triangle/quad

out vec2 v_texcoord;

void main() {
    v_texcoord = a_position * 0.5 + 0.5;
    gl_Position = vec4(a_position, 0.0, 1.0);
}
