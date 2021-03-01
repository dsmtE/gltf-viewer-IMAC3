#version 330

in vec3 vViewSpacePosition;
in vec3 vViewSpaceNormal;
in vec2 vTexCoords;

uniform vec3 uLightDirection;
uniform vec3 uLightColor;
uniform float uLightIntensity;

uniform vec4 uBaseColorFactor;
uniform sampler2D uBaseColorTexture;

out vec3 fColor;

// ----- Useful constants ----- //
#define PI  3.14159265358979323846264338327
#define GAMMA  2.2

// from http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
vec3 linear2srgb(vec3 color) { return pow(color, vec3(1 / GAMMA)); }

vec4 srgb2linear(vec4 srgbIn) { return vec4(pow(srgbIn.xyz, vec3(GAMMA)), srgbIn.w); }

void main() {
  
  vec4 textureCol = srgb2linear(texture(uBaseColorTexture, vTexCoords));
  vec3 diffuse = vec3(1 / PI) * vec3(uBaseColorFactor) * vec3(textureCol);
  float lightIntesity = uLightIntensity * dot(normalize(vViewSpaceNormal), uLightDirection);
  vec3 col = diffuse * lightIntesity * uLightColor;

  fColor = linear2srgb(col);
}
