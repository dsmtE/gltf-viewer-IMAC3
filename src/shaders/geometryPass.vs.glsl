#version 330

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoords;
layout(location = 3) in vec4 tangent;

out vec3 vViewSpacePosition;
out vec3 vViewSpaceNormal;
out vec2 vTexCoords;

// normal mapping
out vec3 vT;
out vec3 vB;
out vec3 vN;

uniform mat4 uModelViewProjMatrix;
uniform mat4 uModelViewMatrix;
uniform mat4 uNormalMatrix;

void main() {
    vViewSpacePosition = vec3(uModelViewMatrix * vec4(position, 1));
	vViewSpaceNormal = normalize(vec3(uNormalMatrix * vec4(normal, 0)));
	vTexCoords = texCoords;
    
    vN = mat3(uNormalMatrix) * normalize(normal);
    vT = mat3(uModelViewMatrix) * normalize(tangent.xyz);
    vT = normalize(vT - dot(vT, vN) * vN); // re-orthogonalize the TBN vectors (Gramm-Schmidt)
    vB = cross(vN, vT) * tangent.w; // Specifications here:  https://github.com/KhronosGroup/glTF/blob/master/specification/2.0/README.md#geometry

    gl_Position =  uModelViewProjMatrix * vec4(position, 1);
}
