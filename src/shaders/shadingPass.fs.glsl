#version 330

uniform vec3 uLightDirection;
uniform vec3 uLightColor;
uniform float uLightIntensity;
uniform float uOcclusionStrength;

uniform int uDeferredShadingDisplayId;

// Deferred shading (already in view space)
uniform sampler2D uGPosition;
uniform sampler2D uGNormal;
uniform sampler2D uGAlbedo;
uniform sampler2D uGOcclusionRoughnessMetallic;
uniform sampler2D uGEmissive;

out vec3 fColor;

// ----- Useful constants ----- //
#define PI  3.14159265358979323846264338327
#define GAMMA  2.2

#define saturate(v) clamp(v, 0, 1)

// from http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
vec3 linear2srgb(vec3 color) { return pow(color, vec3(1 / GAMMA)); }

vec4 srgb2linear(vec4 srgbIn) { return vec4(pow(srgbIn.xyz, vec3(GAMMA)), srgbIn.w); }

float speedPow5(float x) {
  float sqrX = x * x;
  return x * sqrX * sqrX;
}

vec3 fresnelSchlick(float vDotH, vec3 F0) {
  return F0 + (1 - F0) * speedPow5(saturate(1 - vDotH));
}  

float DistributionGGX(vec3 N, vec3 H, float roughness) {
  float a = roughness*roughness;
  float a2 = a * a;
  float NdotH  = saturate(dot(N, H));

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
  float NdotV = saturate(dot(N, V));
  float NdotL = saturate(dot(N, L));
  float ggx2  = GeometrySchlickGGX(NdotV, roughness);
  float ggx1  = GeometrySchlickGGX(NdotL, roughness);

  return ggx1 * ggx2;
}

void main() {
  // retrive Gbuffers data using texelFetch
  vec3 position = texelFetch(uGPosition, ivec2(gl_FragCoord.xy), 0).xyz;

  vec3 N = texelFetch(uGNormal, ivec2(gl_FragCoord.xy), 0).xyz;
  vec3 V = normalize(-position);
  vec3 L = normalize(uLightDirection);
  vec3 H = normalize(L + V);

  vec3 radiance = uLightColor * uLightIntensity;

  vec3 baseColor = texelFetch(uGAlbedo, ivec2(gl_FragCoord.xy), 0).xyz;
  vec3 occlusionRoughnessMetallic = texelFetch(uGOcclusionRoughnessMetallic, ivec2(gl_FragCoord.xy), 0).xyz;
  float occlusion = occlusionRoughnessMetallic.r;
  float roughness = occlusionRoughnessMetallic.g;
  float metallic = occlusionRoughnessMetallic.b;
  vec3 emissive = texelFetch(uGEmissive, ivec2(gl_FragCoord.xy), 0).xyz;

  vec3 dielectricSpecular = vec3(0.04);
  // base reflectivity 
  vec3 F0 = mix(vec3(dielectricSpecular), baseColor, metallic);

  // cook-torrance brdf
  // source : https://learnopengl.com/PBR/Lighting
  float D = DistributionGGX(N, H, roughness);        
  float G = GeometrySmith(N, V, L, roughness);      
  vec3 F = fresnelSchlick(saturate(dot(H, V)), F0);  

  float NdotL = saturate(dot(N, L));
  float NdotV = saturate(dot(N, V));
  
  vec3 specular = (D * G * F) / max(4.0 * NdotV * NdotL, 0.001);  

  vec3 diffuse = baseColor * (1 - metallic) * (1- F) / PI;    


  vec3 color = (diffuse + specular) * radiance * NdotL + emissive;

  // uOcclusionStrength == 0 mean no occlusion
  color = mix(color, color * occlusion, uOcclusionStrength);

  color = linear2srgb(color);

  switch (uDeferredShadingDisplayId) {
    case 0:
      // nothing show all
      break;
    case 1:
      color = position;
      break;
    case 2:
      color = N;
      break;
    case 3:
      color = baseColor;
      break;
    case 4:
      color = vec3(roughness);
      break;
    case 5:
      color = vec3(metallic);
      break;
    case 6:
      color = vec3(occlusion);
      break;
    case 7:
      color = emissive;
      break;
  }

  fColor = color;
}