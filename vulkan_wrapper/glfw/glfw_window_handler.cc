
#include "vulkan_wrapper/glfw/glfw_window_handler.hh"
#include <iostream>

GlfwWindowContainer::GlfwWindowContainer(
    std::pair<uint32_t, uint32_t> dimensions, std::string window_name,
    bool resizable) {

  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // no opengl
  auto resize_status = resizable ? GLFW_TRUE : GLFW_FALSE;
  glfwWindowHint(GLFW_RESIZABLE, resize_status);

  window = std::shared_ptr<GLFWwindow>(
      glfwCreateWindow(dimensions.first, dimensions.second, window_name.c_str(),
                       nullptr, nullptr),
      [](GLFWwindow *w) { glfwDestroyWindow(w); });

  if (!window) {
    throw std::runtime_error("Failed to create GLFW window");
  } else {
    std::cout << "Established GLFW Window instance" << std::endl;
  }
}

auto GlfwWindowContainer::get() -> std::weak_ptr<GLFWwindow> { return window; }

auto GlfwWindowContainer::shared_get() -> std::shared_ptr<GLFWwindow> {
  return window;
}
