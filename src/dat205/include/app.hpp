#pragma once

#include "camera.hpp"
#include "util/optix.hpp"
#include "util/window.hpp"

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
  optix::Program m_miss_program;
  optix::Program m_closest_hit_program;

  optix::Program m_boundingbox_triangle_indexed;
  optix::Program m_intersection_triangle_indexed;

  optix::Context m_ctx;
  optix::Buffer m_output_buffer;

  GLuint m_output_texture;
  GLuint m_output_pbo;
  GLuint m_output_program;

  optix::Group        m_root_group;
  optix::Acceleration m_root_acceleration;

  optix::Material m_opaque_mat;

  PinholeCamera m_camera;
  float m_mouse_speed;

  void handle_user_input();
  void display();
  void render_gui();
  void render_scene();
  void update_viewport();

  void create_scene();
  optix::Geometry createPlane(const int tessU, const int tessV, const int upAxis);
  optix::Geometry createSphere(const int tessU, const int tessV, const float radius, const float maxTheta);
  optix::Geometry createGeometry(std::vector<VertexAttributes> const& attributes, std::vector<unsigned int> const& indices);

};

