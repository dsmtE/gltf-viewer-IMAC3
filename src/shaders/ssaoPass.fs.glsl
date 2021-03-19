#version 420

in vec2 vTexCoords;

uniform sampler2D uGPosition;
uniform sampler2D uGNormal;

#define MAX_SSAO_SAMPLES 128

uniform vec3 samples[MAX_SSAO_SAMPLES];
uniform int samplesSize;
uniform float radius = 0.5;
uniform float bias = 0.025;

uniform mat4 uProjMatrix;

out vec3 fColor;

vec2 hash2(vec2 p) {
	vec3 p3 = fract(vec3(p.xyx) * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yzx+33.33);
    return fract((p3.xx+p3.yz)*p3.zy);
}

void main() {
    vec3 fragPos = texelFetch(uGPosition, ivec2(gl_FragCoord.xy), 0).xyz;
    vec3 normal = texelFetch(uGNormal, ivec2(gl_FragCoord.xy), 0).xyz;

    // compute random vector around z axis
    vec3 randomVec = normalize(vec3(hash2(gl_FragCoord.xy), 0.0));

    // Create TBN tangent-space matrix
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    mat3 TBN = mat3(tangent, cross(normal, tangent), normal);

    float occlusion = 0.0;
    for (int i = 0; i < samplesSize; ++i) {
        vec3 samplePos = TBN * samples[i]; // from tangent to view-space
        samplePos = fragPos + samplePos * radius; 
        
        // Project sample position (to sample texture) (to get position on screen/texture)
        vec4 offset = uProjMatrix * vec4(samplePos, 1.0); // From view to clip-space
        offset.xyz /= offset.w; // Perspective divide
        offset.xyz = offset.xyz * 0.5 + 0.5; // Transform to range 0.0 - 1.0
        
        // Get sample depth
        float sampleDepth = texture(uGPosition, offset.xy).z; // Get depth value of kernel sample
        
        // Range check & accumulate
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
        occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;      

    }
    occlusion = 1.0 - (occlusion / samplesSize);
    
    fColor = vec3(occlusion);
}