#version 330

in vec3 vViewSpacePosition;
in vec3 vViewSpaceNormal;
in vec2 vTexCoords;

// normal mapping
in vec3 vT;
in vec3 vB;
in vec3 vN;

uniform vec4 uBaseColorFactor;
uniform float uMetallicFactor;
uniform float uRoughnessFactor;
uniform vec3 uEmissiveFactor;
uniform float uOcclusionStrength;
uniform int uNormalEnable;

uniform sampler2D uBaseColorTexture;
uniform sampler2D uNormalTexture;
uniform sampler2D uMetallicRoughnessTexture;
uniform sampler2D uOcclusionTexture;
uniform sampler2D uEmissiveTexture;

// Deferred shading
layout(location = 0) out vec3 gPosition;
layout(location = 1) out vec3 gNormal;
layout(location = 2) out vec3 gAlbedo;
layout(location = 3) out vec3 gOcclusionRoughnessMetallic;
layout(location = 4) out vec3 gEmissive;

// ----- Useful constants ----- //
#define PI  3.14159265358979323846264338327
#define GAMMA  2.2

#define saturate(v) clamp(v, 0, 1)

vec3 linear2srgb(vec3 color) { return pow(color, vec3(1 / GAMMA)); }
vec4 srgb2linear(vec4 srgbIn) { return vec4(pow(srgbIn.xyz, vec3(GAMMA)), srgbIn.w); }

vec3 unpackNormal(vec4 packedNormal) {
  packedNormal.x *= packedNormal.w;

  vec3 normal;
  normal.rg = packedNormal.rg * 2 - 1;
  normal.b = sqrt(1-saturate(dot(normal.rg, normal.rg)));
  return normal;
}

vec3 computeNormal(vec3 T, vec3 B, vec3 N, vec4 normalTextureValue) {
  mat3 TBN = mat3(normalize(T), normalize(B), normalize(N));
  return TBN * unpackNormal(normalTextureValue);
}

void main(){
  vec3 N = normalize(vViewSpaceNormal);
  vec3 V = normalize(-vViewSpacePosition);

  vec4 baseColor = uBaseColorFactor * srgb2linear(texture(uBaseColorTexture, vTexCoords));

  vec4 metallicRougnessFromTexture = texture(uMetallicRoughnessTexture, vTexCoords);
  float metallic = uMetallicFactor * metallicRougnessFromTexture.b;
  float roughness = uRoughnessFactor * metallicRougnessFromTexture.g;
  float occlusion = uOcclusionStrength * texture(uOcclusionTexture, vTexCoords).r;
  vec3 emissive = uEmissiveFactor * srgb2linear(texture(uEmissiveTexture, vTexCoords)).rgb;

  if(uNormalEnable == 1) {
    N = computeNormal(vT, vB, vN, texture(uNormalTexture, vTexCoords));
  }

  // Deferred shading
  gPosition = vViewSpacePosition;
  gNormal = N;
  gAlbedo = baseColor.rgb;
  // texturePacking Occlusion/Roughness/Metallic 
  gOcclusionRoughnessMetallic  = vec3(occlusion, roughness, metallic);
  gEmissive = emissive;

  // gPosition = vec3(1, 0, 0);
  // gNormal = vec3(0, 1, 0);
  // gAlbedo = vec3(0, 0, 1);
  // // texturePacking Occlusion/Roughness/Metallic 
  // gOcclusionRoughnessMetallic  = vec3(1, 0, 1);
  // gEmissive = vec3(1, 1, 0);
}