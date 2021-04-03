#version 420

uniform float nearDistance;
uniform float farDistance;

uniform sampler2D uDepth;

out vec4 fColor;

float LinearizeDepth(float depth) {
    float z = depth * 2.0 - 1.0; // Back to NDC 
    return (2.0 * nearDistance * farDistance) / (farDistance + nearDistance - z * (farDistance - nearDistance));	
}

void main() {
    float depth = texelFetch(uDepth, ivec2(gl_FragCoord.xy), 0).r;
    fColor = vec4(vec3(LinearizeDepth(depth) / farDistance), 1.0); // perspective projection
    // fColor = vec4(vec3(depth), 1.0); // orthographic
    // fColor = vec3(pow(depth, 100.f)); // Since the depth is between 0 and 1, pow it to darkness its value
} 