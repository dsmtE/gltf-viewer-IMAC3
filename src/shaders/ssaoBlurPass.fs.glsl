#version 420

out float FragColor;
  
in vec2 vTexCoords;
  
uniform sampler2D uSSAO;

void main() {
    vec2 texelSize = 1.0 / vec2(textureSize(uSSAO, 0));
    float result = 0.0;
    for (float x = -2; x < 2; ++x) {
        for (float y = -2; y < 2; ++y) {
            result += texture(uSSAO, vTexCoords + vec2(x, y) * texelSize).r;
        }
    }
    FragColor = result / (4.0 * 4.0);
}  