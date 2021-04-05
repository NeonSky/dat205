#include "app.hpp"

void Application::handle_mouse_input() {
  ImGuiIO const& io = ImGui::GetIO();

  const ImVec2 mousePosition = ImGui::GetMousePos(); // Mouse coordinate window client rect.
  const int x = int(mousePosition.x);
  const int y = int(mousePosition.y);

  // Ensure mouse is not interacting with the GUI.
  if (!io.WantCaptureMouse) {
    if (ImGui::IsMouseDown(0)) { // Left mouse button
      m_camera.orbit(x, y);
    }
    else if (ImGui::IsMouseDown(1)) { // Right mouse button
      m_camera.pan(x, y);
    }
    else if (io.MouseWheel != 0.0f) { // Mouse wheel scroll
      m_camera.zoom(m_camera_zoom_speed * -io.MouseWheel);
    }
    else {
      m_camera.setBaseCoordinates(x, y);
    }
  }
}

void Application::handle_keyboard_input(GLFWwindow* window, int key, int scancode, int action, int mods) {
  if (key == GLFW_KEY_G && action == GLFW_PRESS) {
    m_show_gui = !m_show_gui;
  }

  if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
    m_paused = !m_paused;
  }
}
