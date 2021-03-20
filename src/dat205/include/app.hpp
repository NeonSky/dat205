#pragma once

#include "window.hpp"

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
};

