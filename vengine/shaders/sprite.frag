// V Engine 2.0 — Sprite fragment shader (GLES 300 es).
#version 300 es
precision highp float;

uniform sampler2D u_texture;
uniform vec4 u_tint;

in vec2 v_texcoord;
in vec4 v_color;

out vec4 frag_color;

void main() {
    vec4 tex = texture(u_texture, v_texcoord);
    frag_color = tex * v_color * u_tint;
}
