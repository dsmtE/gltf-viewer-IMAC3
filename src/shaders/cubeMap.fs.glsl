#version 420

out vec4 FragColor;

in vec3 vViewSpacePosition;
in vec3 vTexCoords;

uniform samplerCube cubeMap;

void main() {
    FragColor = vec4(texture(cubeMap, vTexCoords).rgb, 0);
}