#pragma once

#include "window.hpp"

#include <optix.h>
#include <optixu/optixpp_namespace.h>

struct ApplicationCreateInfo {
  GLFWwindow* window;
  unsigned int window_width;
  unsigned int window_height;
};

class Application {
public:
  Application(ApplicationCreateInfo create_info);

  void run();

private:
  GLFWwindow* m_window;
  unsigned int m_window_width;
  unsigned int m_window_height;
  bool m_show_gui;

  optix::Program m_ray_gen_program;
  optix::Program m_exception_program;

  optix::Context m_ctx;
  optix::Buffer m_output_buffer;

  GLuint m_output_texture;
  GLuint m_output_pbo;
  GLuint m_output_program;

  void handle_user_input();
  void display();
  void render_gui();
  void render_scene();
  void update_viewport();
};

