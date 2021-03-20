#version 420

out vec4 FragColor;

layout(location = 0) out vec3 gPosition;
layout(location = 1) out vec3 gNormal;
layout(location = 2) out vec3 gAlbedo;
layout(location = 3) out vec3 gOcclusionRoughnessMetallic;
layout(location = 4) out vec3 gEmissive;

in vec3 vViewSpacePosition;
in vec3 vTexCoords;

uniform samplerCube cubeMap;

void main() {
    gPosition = vViewSpacePosition;
    gNormal = vec3(0);
    gAlbedo = texture(cubeMap, vTexCoords).rgb;
    gOcclusionRoughnessMetallic  = vec3(0);
    gEmissive = vec3(0);
}