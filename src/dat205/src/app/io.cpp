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
  // NOTE: this setup is awkward but reduces input lag
  static bool d_down = false;
  static bool f_down = false;
  static bool j_down = false;
  static bool k_down = false;

  auto update_key_state = [&](bool& is_down, int k) {
    if (key == k) {
      if (action == GLFW_PRESS) {
        is_down = true;
      } else if(action == GLFW_RELEASE) {
        is_down = false;
      }
    }
  };

  update_key_state(d_down, GLFW_KEY_D);
  update_key_state(f_down, GLFW_KEY_F);
  update_key_state(j_down, GLFW_KEY_J);
  update_key_state(k_down, GLFW_KEY_K);

  if (d_down) {
    m_player1_velocity = -1.0f;
  } else if (f_down) {
    m_player1_velocity = 1.0f;
  } else {
    m_player1_velocity = 0.0f;
  }

  if (k_down) {
    m_player2_velocity = -1.0f;
  } else if (j_down) {
    m_player2_velocity = 1.0f;
  } else {
    m_player2_velocity = 0.0f;
  }

  if (key == GLFW_KEY_G && action == GLFW_PRESS) {
    m_show_gui = !m_show_gui;
  }

  if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
    m_paused = !m_paused;
  }
}
