#include "opengl/shader.hpp"

#include <sutil.h>

#include <fstream>
#include <iostream>

void check_info_log(GLuint gl_object);

std::string shader_root_path() {
  return "./dat205/shaders/glsl/";
}

GLuint create_program(std::string vert_shader_path, std::string frag_shader_path) {

  // Load shaders from files.
  GLuint vert_shader  = load_shader(vert_shader_path, GL_VERTEX_SHADER);
  GLuint frag_shader  = load_shader(frag_shader_path, GL_FRAGMENT_SHADER);

  // Attach the shaders to a new program.
  GLuint program = glCreateProgram();
  glAttachShader(program, vert_shader);
  glAttachShader(program, frag_shader);

  // Delete the temporary shader modules.
  glDeleteShader(vert_shader);
  glDeleteShader(frag_shader);

  // Link program and verify that it was successful.
  glLinkProgram(program);
  
  check_info_log(program);
  GLint ok = 0;
  glGetProgramiv(program, GL_LINK_STATUS, &ok);
  if (!ok) {
    std::cerr << "Failed to link program object with its attached shaders." << std::endl;
  }

  return program;
}

GLuint load_shader(std::string& shader_path, GLenum type) {

  // Check if file exists
  std::string full_shader_path = shader_root_path() + shader_path;
  std::ifstream file(full_shader_path);
  if (file.fail()) {
    std::cerr << "No shader file found at " << full_shader_path << std::endl;
  }

  // Read shader file as bytes into memory.
  std::string src((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

  // Create shader module.
  GLuint shader = glCreateShader(type);

  // Compile the shader module.
  const char* raw_shader = src.c_str();
  glShaderSource(shader, 1, &raw_shader, nullptr);
  glCompileShader(shader);

  // Verify that compilation was successful.
  check_info_log(shader);
  int ok = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    std::cerr << "Failed to compile shader " << shader_path << std::endl;
  }

  return shader;
}

void use_program(GLuint program, std::function<void()> f) {
  glUseProgram(program);
  f();
  glUseProgram(0);
}

void check_info_log(GLuint gl_object) {
  GLint max_length;

  if (glIsProgram(gl_object)) {
    glGetProgramiv(gl_object, GL_INFO_LOG_LENGTH, &max_length);
  } else {
    glGetShaderiv(gl_object, GL_INFO_LOG_LENGTH, &max_length);
  }

  if (max_length > 0) {
    GLchar* info_log = (GLchar*) malloc(max_length);

    if (glIsProgram(gl_object)) {
      glGetProgramInfoLog(gl_object, max_length, nullptr, info_log);
    } else {
      glGetShaderInfoLog(gl_object, max_length, nullptr, info_log);
    }

    std::cout << info_log << std::endl;

    free(info_log);
  }
}