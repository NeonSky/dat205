#pragma once

#include <GL/glew.h>

#include <functional>
#include <string>

GLuint create_program(std::string vert_shader_path, std::string frag_shader_path);
GLuint load_shader(std::string& shader_path, GLenum type);
void use_program(GLuint program, std::function<void()> f);
