#include "app.hpp"

// Renders game statistics and frame rate.
void Application::render_gui(unsigned int fps) {
  render_gui_frame([&]() {
    if (m_show_gui) {
      ImGui::Begin("DAT205");
      ImGui::Text("FPS: %u", fps);
      ImGui::SliderInt("SSAA", &m_ssaa, 1, 10);
      if (m_game->winner() == 0) {
        ImGui::Text("Player 1 points: %d", m_game->player1_score());
        ImGui::Text("Player 2 points: %d", m_game->player2_score());
      } else {
        ImGui::Text("The winner is player %d", m_game->winner());
        if (ImGui::Button("Reset Game")) {
          m_game->reset();
        }
      }
      ImGui::End();
    }
  });
}

// Updates render viewport to match window.
void Application::update_viewport() {

  // Fetch current window framebuffer size.
  int width, height;
  glfwGetFramebufferSize(m_window, &width, &height);

  // Check if new dimensions are valid and differ.
  if ((width != 0 && height != 0) && (width != m_window_width || height != m_window_height)) {

    // Update OpenGL viewport.
    m_window_width = width;
    m_window_height = height;
    glViewport(0, 0, m_window_width, m_window_height);

    // Unregister buffer while modifying it so CUDA will be notified of the size change (and not crash).
    unregister_buffer(m_output_buffer, [&]() {

      // Resize output buffer to match new window dimensions.
      m_output_buffer->setSize(m_window_width, m_window_height);

      // Initialize the output buffer with empty data.
      bind_buffer(GL_PIXEL_UNPACK_BUFFER, m_output_buffer->getGLBOId(), [&]() {
        glBufferData(GL_PIXEL_UNPACK_BUFFER,
                     m_output_buffer->getElementSize() * m_window_width * m_window_height,
                     nullptr,
                     GL_STREAM_DRAW);
      });
    });

    // Update camera.
    m_camera.setViewport(m_window_width, m_window_height);
  }
}
