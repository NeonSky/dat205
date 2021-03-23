#version 420

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

out vec3 outColor;

uniform mat4 model_view_projection;

void main() {
  gl_Position = model_view_projection * vec4(position, 1.0);
  outColor = color;
}
