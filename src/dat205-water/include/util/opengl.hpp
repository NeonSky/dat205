#pragma once

#include <GL/glew.h>

#include <functional>
#include <string>

// Buffers
void bind_buffer(GLenum target, GLuint buffer, std::function<void()> f);

// Shaders
GLuint create_program(std::string vert_shader_path, std::string frag_shader_path);
GLuint load_shader(std::string& shader_path, GLenum type);
void use_program(GLuint program, std::function<void()> f);

// Textures
void bind_texture(GLenum target, GLuint texture, std::function<void()> f);