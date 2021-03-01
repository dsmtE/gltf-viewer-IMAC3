#version 330

in vec3 vViewSpacePosition;
in vec3 vViewSpaceNormal;
in vec2 vTexCoords;

uniform vec3 uLightDirection;
uniform vec3 uLightColor;
uniform float uLightIntensity;

out vec3 fColor;

void main() {
  fColor = vec3(1. / 3.141592) * uLightColor * uLightIntensity * dot(normalize(vViewSpaceNormal), uLightDirection);
}