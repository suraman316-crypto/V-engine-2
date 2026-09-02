// V Engine 2.0 — Post-process: bloom (bright pass + composite).
#version 300 es
precision highp float;

uniform sampler2D u_scene;
uniform sampler2D u_bloom;
uniform float u_threshold;
uniform float u_intensity;

in vec2 v_texcoord;
out vec4 frag_color;

void main() {
    vec3 scene = texture(u_scene, v_texcoord).rgb;
    vec3 bloom = texture(u_bloom, v_texcoord).rgb;
    frag_color = vec4(scene + bloom * u_intensity, 1.0);
}
