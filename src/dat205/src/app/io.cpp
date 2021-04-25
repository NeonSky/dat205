#include "app.hpp"

// Updates application state based on input from the user's mouse.
void Application::handle_mouse_input() {
  ImGuiIO const& io = ImGui::GetIO();

  // Fetch mouse position (window coordinates).
  const ImVec2 mousePosition = ImGui::GetMousePos();
  const int x = (int) mousePosition.x;
  const int y = (int) mousePosition.y;

  // Camera Manipulation
  //
  // Ensure that the mouse is not interacting with the GUI.
  if (!io.WantCaptureMouse) {

    // Left Mouse Button
    if (ImGui::IsMouseDown(0)) {
      m_camera.orbit(x, y);
    }

    // Right Mouse Button
    else if (ImGui::IsMouseDown(1)) {
      m_camera.pan(x, y);
    }

    // Mouse Scroll Wheel
    else if (io.MouseWheel != 0.0f) {
      m_camera.zoom(m_camera_zoom_speed * -io.MouseWheel);
    }

    // Otherwise, we update the reference/base coordinates of the camera.
    else {
      m_camera.setBaseCoordinates(x, y);
    }

  }
}

// Updates application state based on input from the user's keyboard.
void Application::handle_keyboard_input(GLFWwindow* window,
                                        int key,
                                        int scancode,
                                        int action,
                                        int mods) {

  // Updates bool state based on state of key with keycode `k`.
  auto update_key_state = [&](bool& is_down, int k) {
    if (key == k) {
      if (action == GLFW_PRESS) {
        is_down = true;
      } else if(action == GLFW_RELEASE) {
        is_down = false;
      }
    }
  };

  // NOTE: this setup is awkward but reduces input lag
  static bool d_down = false;
  static bool f_down = false;
  static bool j_down = false;
  static bool k_down = false;

  // Update the key states.
  update_key_state(d_down, GLFW_KEY_D);
  update_key_state(f_down, GLFW_KEY_F);
  update_key_state(j_down, GLFW_KEY_J);
  update_key_state(k_down, GLFW_KEY_K);

  // Update velocity of player 1's paddle.
  if (d_down) {
    m_player1_velocity = -1.0f;
  } else if (f_down) {
    m_player1_velocity = 1.0f;
  } else {
    m_player1_velocity = 0.0f;
  }

  // Update velocity of player 2's paddle.
  if (k_down) {
    m_player2_velocity = -1.0f;
  } else if (j_down) {
    m_player2_velocity = 1.0f;
  } else {
    m_player2_velocity = 0.0f;
  }

  // Enable/disble GUI.
  if (key == GLFW_KEY_G && action == GLFW_PRESS) {
    m_show_gui = !m_show_gui;
  }

  // Pause/resume scene updates.
  if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
    m_paused = !m_paused;
  }
}
