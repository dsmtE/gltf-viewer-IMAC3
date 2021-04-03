#version 420

in vec2 vTexCoords;

uniform sampler2D tex;

out vec3 fColor;
  
uniform bool horizontal;
uniform float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main() {             
    vec2 texOffset = 1.0 / textureSize(tex, 0); // gets size of single texel
    vec3 result = texture(tex, vTexCoords).rgb * weight[0]; // current fragment's contribution
    if(horizontal) {
        for(int i = 1; i < 5; ++i) {
            result += texture(tex, vTexCoords + vec2(texOffset.x * i, 0.0)).rgb * weight[i];
            result += texture(tex, vTexCoords - vec2(texOffset.x * i, 0.0)).rgb * weight[i];
        }
    } else {
        for(int i = 1; i < 5; ++i) {
            result += texture(tex, vTexCoords + vec2(0.0, texOffset.y * i)).rgb * weight[i];
            result += texture(tex, vTexCoords - vec2(0.0, texOffset.y * i)).rgb * weight[i];
        }
    }

    fColor = result;
}