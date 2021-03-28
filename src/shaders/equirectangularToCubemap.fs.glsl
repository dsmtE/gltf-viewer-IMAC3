#version 450

out vec4 FragColor;

in vec3 vTexCoords;

uniform sampler2D hdrEquirectangular;

const vec2 invAtan = vec2(0.1591, 0.3183);

vec2 SampleSphericalMap(vec3 v) {
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    return uv * invAtan + 0.5;
}

void main() {
    vec2 uv = SampleSphericalMap(normalize(vTexCoords));
    vec3 color = texture(hdrEquirectangular, uv).rgb;
    
    // almost all HDR maps are in linear color space by default so we need to apply gamma correction.
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2)); 

    FragColor = vec4(color, 1.0);
}