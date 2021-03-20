#version 420

layout (location = 0) in vec3 position;

out vec3 vViewSpacePosition;
out vec3 vTexCoords;

uniform mat4 ProjMatrix;
uniform mat4 ViewMatrix;

void main() {
    vTexCoords = position;
    vec4 pos = ProjMatrix * ViewMatrix * vec4(position, 1);
    vViewSpacePosition = pos.xyz;
    // trick from https://learnopengl.com/Advanced-OpenGL/Cubemaps
    gl_Position = pos.xyww;
}  