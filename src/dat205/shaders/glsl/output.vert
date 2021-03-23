#version 330

layout(location = 0) in vec4 attrPosition;
layout(location = 8) in vec2 attrTexCoord0;

out vec2 varTexCoord0;

void main() {
  gl_Position  = attrPosition;
  varTexCoord0 = attrTexCoord0;
}
