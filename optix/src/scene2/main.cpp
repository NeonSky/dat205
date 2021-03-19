#define RT_USE_TEMPLATED_RTCALLABLEPROGRAM 1
#define DENOMINATOR_EPSILON 1.0e-6f
#define USE_DEBUG_EXCEPTIONS 0
#define RT_FUNCTION __forceinline__ __device__

#include "app.h"

#include <sutil.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

static Application* g_app = nullptr;

static bool displayGUI = true;

static void error_callback(int error, const char* description) {
  std::cerr << "Error: "<< error << ": " << description << std::endl;
}

int main(int argc, char *argv[])
{
  int  windowWidth  = 512;
  int  windowHeight = 512;
  int  devices      = 3210;  // Decimal digits encode OptiX device ordinals. Default 3210 means to use all four first installed devices, when available.
  bool interop      = true;  // Use OpenGL interop Pixel-Bufferobject to display the resulting image. Disable this when running on multi-GPU or TCC driver mode.
  int  stackSize    = 1024;  // Command line parameter just to be able to find the smallest working size.

  std::string filenameScreenshot;
  bool hasGUI = true;

  glfwSetErrorCallback(error_callback);

  if (!glfwInit())
  {
    error_callback(1, "GLFW failed to initialize.");
    return 1;
  }

  GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "App", NULL, NULL);
  if (!window)
  {
    error_callback(2, "glfwCreateWindow() failed.");
    glfwTerminate();
    return 2;
  }

  glfwMakeContextCurrent(window);

  if (glewInit() != GL_NO_ERROR)
  {
    error_callback(3, "GLEW failed to initialize.");
    glfwTerminate();
    return 3;
  }

  g_app = new Application(window, windowWidth, windowHeight,
                          devices, stackSize, interop);

  if (!g_app->isValid())
  {
    error_callback(4, "Application initialization failed.");
    glfwTerminate();
    return 4;
  }

  // Main loop
  while (!glfwWindowShouldClose(window))
  {
    glfwPollEvents(); // Render continuously.

    glfwGetFramebufferSize(window, &windowWidth, &windowHeight);

    g_app->reshape(windowWidth, windowHeight);

    if (hasGUI)
    {
      g_app->guiNewFrame();

      //g_app->guiReferenceManual(); // DAR HACK The ImGui "Programming Manual" as example code.

      g_app->guiWindow(); // The OptiX introduction example GUI window.

      g_app->guiEventHandler(); // Currently only reacting on SPACE to toggle the GUI window.

      g_app->render();  // OptiX rendering and OpenGL texture update.
      g_app->display(); // OpenGL display.

      g_app->guiRender(); // Render all ImGUI elements at last.

      glfwSwapBuffers(window);
    }
    else
    {
      g_app->render();  // OptiX rendering and OpenGL texture update.
      g_app->screenshot(filenameScreenshot);

      glfwSetWindowShouldClose(window, 1);
    }

    //glfwWaitEvents(); // Render only when an event is happening. Needs some glfwPostEmptyEvent() to prevent GUI lagging one frame behind when ending an action.
  }

  // Cleanup
  delete g_app;

  glfwTerminate();

  return 0;
}
