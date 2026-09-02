// V Engine 2.0 — SDF font fragment shader: crisp scalable text with outline.
#version 300 es
precision highp float;

uniform sampler2D u_atlas;
uniform vec4 u_color;
uniform float u_smoothing;   // 1/edge width
uniform float u_outline;     // outline threshold
uniform vec4 u_outline_color;

in vec2 v_texcoord;
in vec4 v_color;

out vec4 frag_color;

void main() {
    float dist = texture(u_atlas, v_texcoord).r;
    float alpha = smoothstep(0.5 - u_smoothing, 0.5 + u_smoothing, dist);
    float outline_a = smoothstep(0.5 - u_outline, 0.5 + u_smoothing, dist) - alpha;
    vec4 col = mix(u_outline_color, u_color * v_color, alpha);
    frag_color = vec4(col.rgb, alpha + outline_a * u_outline_color.a);
}
