#version 330

in vec3 vViewSpacePosition;
in vec3 vViewSpaceNormal;
in vec2 vTexCoords;

uniform vec3 uLightDirection;
uniform vec3 uLightColor;
uniform float uLightIntensity;

uniform vec4 uBaseColorFactor;
uniform float uMetallicFactor;
uniform float uRoughnessFactor;
uniform vec3 uEmissiveFactor;

uniform sampler2D uBaseColorTexture;
uniform sampler2D uMetallicRoughnessTexture;
uniform sampler2D uEmissiveTexture;

uniform float uOcclusionStrength;
uniform sampler2D uOcclusionTexture;

out vec3 fColor;

// ----- Useful constants ----- //
#define PI  3.14159265358979323846264338327
#define GAMMA  2.2

// from http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
vec3 linear2srgb(vec3 color) { return pow(color, vec3(1 / GAMMA)); }

vec4 srgb2linear(vec4 srgbIn) { return vec4(pow(srgbIn.xyz, vec3(GAMMA)), srgbIn.w); }

float speedPow5(float x) {
  float sqrX = x * x;
  return x * sqrX * sqrX;
}

vec3 fresnelSchlick(float vDotH, vec3 F0) {
  return F0 + (1 - F0) * speedPow5(max(1.0 - vDotH, 0.0));
}  


float DistributionGGX(vec3 N, vec3 H, float roughness) {
  float a = roughness*roughness;
  float a2 = a * a;
  float NdotH  = max(dot(N, H), 0);

  float denom = (NdotH*NdotH * (a2 - 1) + 1);

  return a2 / (PI * denom * denom);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
  float r = (roughness + 1.0);
  float k = (r*r) / 8.0;

  float num   = NdotV;
  float denom = NdotV * (1.0 - k) + k;

  return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
  float NdotV = max(dot(N, V), 0.0);
  float NdotL = max(dot(N, L), 0.0);
  float ggx2  = GeometrySchlickGGX(NdotV, roughness);
  float ggx1  = GeometrySchlickGGX(NdotL, roughness);

  return ggx1 * ggx2;
}

void main() {
  vec3 N = normalize(vViewSpaceNormal);
  vec3 V = normalize(-vViewSpacePosition);
  vec3 L = normalize(uLightDirection);
  vec3 H = normalize(L + V);

  vec3 radiance = uLightColor * uLightIntensity;

  vec4 metallicRougnessFromTexture = texture(uMetallicRoughnessTexture, vTexCoords);

  vec4 baseColor = uBaseColorFactor * srgb2linear(texture(uBaseColorTexture, vTexCoords));
  vec3 metallic = vec3(uMetallicFactor * metallicRougnessFromTexture.b);
  float roughness = uRoughnessFactor * metallicRougnessFromTexture.g;

  vec3 dielectricSpecular = vec3(0.04);
  // base reflectivity 
  vec3 F0 = mix(vec3(dielectricSpecular), baseColor.rgb, metallic);

  // cook-torrance brdf
  // source : https://learnopengl.com/PBR/Lighting
  float D = DistributionGGX(N, H, roughness);        
  float G = GeometrySmith(N, V, L, roughness);      
  vec3 F = fresnelSchlick(max(dot(H, V), 0), F0);  

  float NdotL = max(dot(N, L), 0);
  float NdotV = max(dot(N, V), 0);
  
  vec3 specular = (D * G * F) / max(4.0 * NdotV * NdotL, 0.001);  

  vec3 diffuse = baseColor.rgb * (1 - metallic) * (1- F) / PI;    

  vec3 emissive = uEmissiveFactor * srgb2linear(texture2D(uEmissiveTexture, vTexCoords)).rgb;

  vec3 color = (diffuse + specular) * radiance * NdotL + emissive;

  // uOcclusionStrength == 0 mean no occlusion
  if (uOcclusionStrength > 0) {
    float occlusion = texture2D(uOcclusionTexture, vTexCoords).r;
    color = mix(color, color * occlusion, uOcclusionStrength);
  }

  fColor = linear2srgb(color);
}