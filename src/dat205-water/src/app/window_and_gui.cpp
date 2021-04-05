#include "app.hpp"

void Application::render_gui() {
  render_gui_frame([&]() {
    if (m_show_gui) {
      ImGui::Begin("DAT205");
      {
        if (m_paused) {
          ImGui::Text("Simulation paused.");
        } else {
          ImGui::Text("Simulation active.");
        }
      }
      ImGui::End();
    }
  });
}

void Application::update_viewport() {
  int width, height;
  glfwGetFramebufferSize(m_window, &width, &height);

  // Check if new dimensions are valid and differ.
  if ((width != 0 && height != 0) && (width != m_window_width || height != m_window_height)) {

    m_window_width = width;
    m_window_height = height;
    glViewport(0, 0, m_window_width, m_window_height);

    // Unregister buffer while modifying it so CUDA will be notified of the size change (and not crash).
    unregister_buffer(m_output_buffer, [&]() {

      // Resize output buffer to match window dimensions.
      m_output_buffer->setSize(m_window_width, m_window_height);

      // Initialize with empty data.
      bind_buffer(GL_PIXEL_UNPACK_BUFFER, m_output_buffer->getGLBOId(), [&]() {
        glBufferData(GL_PIXEL_UNPACK_BUFFER,
                     m_output_buffer->getElementSize() * m_window_width * m_window_height,
                     nullptr,
                     GL_STREAM_DRAW);
      });
    });

    m_camera.setViewport(m_window_width, m_window_height);
  }
}