#include "app.hpp"

#include <chrono>
#include <iostream>

Application::Application(ApplicationCreateInfo create_info) {
  // Window
  m_window = create_info.window;
  m_window_width = create_info.window_width;
  m_window_height = create_info.window_height;

  // Camera
  m_camera.setViewport(m_window_width, m_window_height);
  m_camera.m_fov = 80.0f;
  m_camera_zoom_speed  = 4.5f;

  // SSAA
  m_ssaa = 1;

  // GUI
  m_show_gui = true;

  // IO
  glfwSetWindowUserPointer(m_window, reinterpret_cast<void*>(this));
  glfwSetKeyCallback(m_window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
    // To use a member function as the callback, we need this WindowUserPointer setup.
    auto* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    app->handle_keyboard_input(window, key, scancode, action, mods);
  });

  // Game
  m_paused = true;
  float game_board_width = 8.0f;
  float game_board_depth = 5.0f;
  m_game = std::unique_ptr<PongGame>(new PongGame(game_board_width, game_board_depth));

  // Optix Rendering
  setup_optix_rendering();

  // Scene
  m_scene = std::unique_ptr<OptixScene>(new OptixScene(m_ctx));
  create_scene();

  // Verify correctness of setup and perform dummy launch to build everything.
  m_ctx->validate();
  m_ctx->launch(0, 0, 0);
};

// Starts the render and update loop of the application.
void Application::run() {

  // Variables for tracking frame rate.
  auto start = std::chrono::system_clock::now();
  unsigned int prev_fps = 0;
  unsigned int frame_counter = 0;

  while (!glfwWindowShouldClose(m_window)) {

    // Handle user input.
    glfwPollEvents();
    handle_mouse_input();

    // Adjust viewport to (potentially new) window dimensions.
    update_viewport();

    update_scene();
    render_scene();

    render_gui(prev_fps);

    // Swap the front and back buffers of the default (double-buffered) framebuffer.
    glfwSwapBuffers(m_window);

    // Update frame rate statistics.
    frame_counter++;

    auto now = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = now - start;

    if (elapsed_seconds.count() > 1.0) {
      prev_fps = frame_counter;
      frame_counter = 0;
      start = now;
    }
  }

  // The OptiX context is no longer needed once the application terminates.
  m_ctx->destroy();
}
