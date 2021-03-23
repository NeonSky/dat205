#version 330

uniform sampler2D samplerHDR;

in vec2 varTexCoord0;

layout(location = 0, index = 0) out vec4 outColor;

void main() {
  outColor = texture(samplerHDR, varTexCoord0);
}
