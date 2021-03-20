#include "app.hpp"

Application::Application(ApplicationCreateInfo create_info) {
  m_window = create_info.window;
  m_window_width = create_info.window_width;
  m_window_height = create_info.window_height;
};

void Application::run() {
  while (!glfwWindowShouldClose(m_window)) {
    ImGui::Render();
    ImGui_ImplGlfwGL2_RenderDrawData(ImGui::GetDrawData());
  }
}
