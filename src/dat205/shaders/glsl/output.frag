#version 330

uniform sampler2D tex_sampler;

in vec2 frag_uv;

layout(location = 0, index = 0) out vec4 out_color;

void main() {
  out_color = texture(tex_sampler, frag_uv);
}
