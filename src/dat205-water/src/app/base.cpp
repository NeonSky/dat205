#include "app.hpp"

Application::Application(ApplicationCreateInfo create_info) {
  // Window
  m_window = create_info.window;
  m_window_width = create_info.window_width;
  m_window_height = create_info.window_height;

  // Camera
  m_camera.setViewport(m_window_width, m_window_height);
  m_camera.m_fov = 80.0f;
  m_camera_zoom_speed  = 4.5f;

  // GUI
  m_show_gui = true;

  // IO
  glfwSetWindowUserPointer(m_window, reinterpret_cast<void*>(this));
  glfwSetKeyCallback(m_window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    app->handle_keyboard_input(window, key, scancode, action, mods);
  });

  // Optix Rendering
  setup_optix_rendering();

  // Scene
  m_scene = std::unique_ptr<OptixScene>(new OptixScene(m_ctx));
  create_scene();

  // Water Simulation
  setup_water_simulation();

  // Verify correctness of setup and perform dummy launch to build everything.
  m_ctx->validate();
  m_ctx->launch(0, 0, 0);
};

void Application::run() {
  while (!glfwWindowShouldClose(m_window)) {
    glfwPollEvents();
    handle_mouse_input();

    // Adjust viewport to (potentially new) window dimensions.
    update_viewport();

    update_scene();

    render_scene();
    render_gui();

    // Swap the front and back buffers of the default (double-buffered) framebuffer.
    glfwSwapBuffers(m_window);
  }
  m_ctx->destroy();
}