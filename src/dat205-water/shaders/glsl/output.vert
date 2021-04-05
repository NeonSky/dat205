#version 330

layout(location = 0) in vec4 vert_pos;
layout(location = 8) in vec2 vert_uv;

out vec2 frag_uv;

void main() {
  gl_Position = vert_pos;
  frag_uv     = vert_uv;
}
