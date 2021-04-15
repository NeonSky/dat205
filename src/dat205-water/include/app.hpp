#pragma once

#include "camera.hpp"
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

  // Water Simulation
  bool m_paused;
  float m_box_width;
  float m_box_height;
  float m_box_depth;

  optix::Acceleration m_water_acceleration;
  optix::Buffer m_particles_buffer;
  int m_particles_count;

  optix::Buffer m_hash_buffer;

  void setup_water_simulation();
  void setup_water_particles();
  void setup_water_geometry();
  void setup_water_physics();

  void update_water_simulation(float dt);
  void reset_hash_table();

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
  void create_scene_lights();
  void update_scene();
  void render_scene();

};

