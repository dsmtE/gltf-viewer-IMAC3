#version 330

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoords;
layout(location = 3) in vec3 tangent;

out vec3 vViewSpacePosition;
out vec3 vViewSpaceNormal;
out vec2 vTexCoords;

// normal mapping
out vec3 vT;
out vec3 vN;

uniform mat4 uModelViewProjMatrix;
uniform mat4 uModelViewMatrix;
uniform mat4 uNormalMatrix;

void main() {
    vViewSpacePosition = vec3(uModelViewMatrix * vec4(position, 1));
	vViewSpaceNormal = normalize(vec3(uNormalMatrix * vec4(normal, 0)));
	vTexCoords = texCoords;
    
    vec3 N = mat3(uNormalMatrix) * normalize(normal);
    vec3 T = mat3(uModelViewMatrix) * normalize(tangent);
    //  Gramm-Schmidt
    // T = normalize(T - dot(T, N) * N);
    vT = T;
    vN = N;

    gl_Position =  uModelViewProjMatrix * vec4(position, 1);
}