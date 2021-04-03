#version 420

in vec2 vTexCoords;

uniform sampler2D uScene;

uniform float uThreshold;

out vec3 fColor;

float toGray(vec3 color) { return dot(color, vec3(0.2126, 0.7152, 0.0722)); }

void main() {

  fColor = texture(uScene, vTexCoords).rgb;

  if (toGray(fColor) < uThreshold) {
    fColor = vec3(0);
  }
}