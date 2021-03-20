#pragma once

#include <imgui/imgui.h>

#define IMGUI_DEFINE_MATH_OPERATORS 1
#include <imgui/imgui_internal.h>

#include <imgui/imgui_impl_glfw_gl2.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <functional>
#include <string>

void open_gui(GLFWwindow* window, std::function<void()> f);
void open_window(std::string title, unsigned int width, unsigned int height, std::function<void(GLFWwindow* window)> f);
