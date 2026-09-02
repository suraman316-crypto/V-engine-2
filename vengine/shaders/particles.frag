// V Engine 2.0 — Particle fragment shader: soft circular sprite.
#version 300 es
precision highp float;

uniform sampler2D u_texture;

in vec4 v_color;
in vec2 v_texcoord;

out vec4 frag_color;

void main() {
    vec4 tex = texture(u_texture, v_texcoord);
    frag_color = tex * v_color;
}
