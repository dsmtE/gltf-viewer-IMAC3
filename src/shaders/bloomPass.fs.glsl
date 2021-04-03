#version 420

in vec2 vTexCoords;

uniform sampler2D uScene;
uniform sampler2D uBloomBlur;

uniform bool uUseBloom;
uniform vec3 uBloomTint;
uniform float uBloomIntensity;

// ----- Useful constants ----- //
#define PI  3.14159265358979323846264338327
#define GAMMA  2.2

#define saturate(v) clamp(v, 0, 1)

// from http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
vec3 linear2srgb(vec3 color) { return pow(color, vec3(1 / GAMMA)); }

vec4 srgb2linear(vec4 srgbIn) { return vec4(pow(srgbIn.xyz, vec3(GAMMA)), srgbIn.w); }

out vec3 fColor;

void main() {
    // hdr scene color
    vec3 sceneColor = srgb2linear(texture(uScene, vTexCoords)).rgb;      
    vec3 bloomColor = srgb2linear(texture(uBloomBlur, vTexCoords)).rgb;
    if (uUseBloom)
        sceneColor += bloomColor * uBloomTint * uBloomIntensity; // additive blending
    
    vec3 result = sceneColor;

    fColor = linear2srgb(result);
} 