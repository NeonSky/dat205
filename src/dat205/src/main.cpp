#include "app.hpp"

#include <iostream>

// Entrypoint of the application.
// Opens a GLFW window with a ImGUI layer on top and then creates and attaches the application.
int main() {
  std::cout << "DAT205 application started." << std::endl;

  unsigned int window_width = 1280;
  unsigned int window_height = 720;

  // Open GLFW window.
  open_window("DAT205 Project by Jacob Eriksson",
              window_width,
              window_height,
              [&](GLFWwindow* window) {

    // Open ImGUI gui.
    open_gui(window, [&]() {

      // Create application.
      ApplicationCreateInfo create_info {
        .window = window,
        .window_width = window_width,
        .window_height = window_height,
      };
      Application app(create_info);

      // Run application.
      app.run();

    });

  });

  std::cout << "DAT205 application terminated." << std::endl;
}
