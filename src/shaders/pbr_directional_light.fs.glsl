#version 330

in vec3 vViewSpacePosition;
in vec3 vViewSpaceNormal;
in vec2 vTexCoords;

uniform vec3 uLightDirection;
uniform vec3 uLightIntensity;

uniform vec4 uBaseColorFactor;

uniform sampler2D uBaseColorTexture;

out vec3 fColor;

void main()
{
  vec3 viewSpaceNormal = normalize(vViewSpaceNormal);
  float oneOverPi = 1. / 3.14;
  vec4 baseColorFromTexture = texture(uBaseColorTexture, vTexCoords);

  vec3 c_diff = vec3(uBaseColorFactor) * vec3(baseColorFromTexture);
  vec3 diffuse = c_diff * oneOverPi;
  fColor = diffuse * uLightIntensity*dot(viewSpaceNormal,uLightDirection);
}