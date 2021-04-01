#pragma once

#include "camera.hpp"
#include "game.hpp"
#include "util/opengl.hpp"
#include "util/optix.hpp"
#include "util/window.hpp"

#include <memory>

// Enable the assert() macro.
#undef NDEBUG
#include <assert.h>

struct ApplicationCreateInfo {
  GLFWwindow* window;
  unsigned int window_width;
  unsigned int window_height;
};

class Application {
public:

  // Base
  Application(ApplicationCreateInfo create_info);

  void run();

private:

  // Window
  GLFWwindow* m_window;
  unsigned int m_window_width;
  unsigned int m_window_height;

  // Camera
  PinholeCamera m_camera;
  float m_camera_zoom_speed;

  void update_viewport();

  // GUI
  bool m_show_gui;

  void render_gui();

  // IO
  void handle_mouse_input();
  void handle_keyboard_input(GLFWwindow* window, int key, int scancode, int action, int mods);

  // Background
  void create_background_geometry();

  // Game
  std::unique_ptr<PongGame> m_game;
  float m_player1_velocity;
  float m_player2_velocity;
  bool m_paused;

  // TODO: water simulation

  // OptiX Rendering
  optix::Buffer m_output_buffer;
  GLuint m_output_texture;
  GLuint m_output_pbo;
  GLuint m_output_program;

  optix::Program m_ray_gen_program;
  optix::Program m_exception_program;
  optix::Program m_miss_program;

  void setup_optix_rendering();
  void display();

  // OptiX Scene
  optix::Context m_ctx;

  optix::Group        m_root_group;
  optix::Acceleration m_root_acceleration;
  std::unique_ptr<OptixScene> m_scene;

  void create_scene();
  void update_scene();
  void render_scene();

};

