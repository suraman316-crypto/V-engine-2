// V Engine 2.0 — Sprite lighting shader: diffuse + normal map lighting.
#version 300 es
precision highp float;

uniform sampler2D u_texture;
uniform sampler2D u_normal_map;
uniform vec3 u_light_pos;     // screen space
uniform vec4 u_light_color;
uniform float u_light_radius;
uniform vec2 u_texel_size;

in vec2 v_texcoord;
out vec4 frag_color;

void main() {
    vec4 albedo = texture(u_texture, v_texcoord);
    vec3 n = texture(u_normal_map, v_texcoord).rgb * 2.0 - 1.0;
    vec3 light_dir = vec3(u_light_pos.xy * u_texel_size - v_texcoord, u_light_pos.z);
    float dist = length(light_dir.xy);
    float atten = clamp(1.0 - dist / u_light_radius, 0.0, 1.0);
    float diff = max(dot(normalize(n), normalize(light_dir)), 0.0);
    vec3 lit = albedo.rgb * (0.3 + diff * atten * 0.7) * u_light_color.rgb;
    frag_color = vec4(lit, albedo.a);
}
