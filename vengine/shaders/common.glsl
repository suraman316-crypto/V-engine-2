// V Engine 2.0 — Common shader uniforms/functions include.
// Declared as a single-source snippet included by fragment shaders via
// `#extension` / preprocessor include in the ShaderLibrary. Kept as plain
// GLSL for offline tooling; the engine injects version/precision headers.

// Standard engine samplers bound at fixed texture units.
#define TEX_SCENE     0
#define TEX_LUT       1
#define TEX_BLOOM     2
#define TEX_NORMALMAP 3
#define TEX_PARTICLE  4
#define TEX_FONT_ATLAS 5

// Linear <-> sRGB conversions (mobile may not have HW conversion).
vec3 srgb_to_linear(vec3 c) {
    return mix(c / 12.92, pow((c + 0.055) / 1.055, vec3(2.4)), step(0.04045, c));
}
vec3 linear_to_srgb(vec3 c) {
    return mix(c * 12.92, 1.055 * pow(c, vec3(1.0/2.4)) - 0.055, step(0.0031308, c));
}

// Reinhard tone map fallback (used when ACES is disabled for battery saving).
vec3 reinhard(vec3 c) { return c / (1.0 + c); }

// Random hash for noise/dithering.
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}
