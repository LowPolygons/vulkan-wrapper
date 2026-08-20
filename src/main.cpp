
#include "3d_app.hh"
#include "mandelbulb_app.hh"
#include "vulkan/vulkan.hpp"
#include "wrapper_boilerplate.hh"

#include <GLFW/glfw3.h>
#include <cstdlib>
#include <iostream>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <glm/glm.hpp>

auto main() -> int {
  auto vulkan_app_data = VulkanRootCreateinfo{
      .width = 800,
      .height = 600,
      .window_resizable = true,
      .window_name = "Morphing Mandelbulb",
      .application_info =
          vk::ApplicationInfo{.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
                              .apiVersion = vk::ApiVersion13},
      .layers = {"VK_LAYER_KHRONOS_validation"},
      .instance_extensions = {},
      .gpu_type = DeviceUtil::DeviceType::DISCRETE,
      .device_extensions = {vk::KHRSwapchainExtensionName},
      .present_mode = vk::PresentModeKHR::eFifo};

  auto maybe_vulkan_root = VulkanRoot::create(vulkan_app_data);

  if (!maybe_vulkan_root) {
    std::cerr << maybe_vulkan_root.error() << std::endl;
    return EXIT_FAILURE;
  }

  auto vulkan_root = std::move(maybe_vulkan_root.value());

  // auto maybe_app = create_mandelbulb_app(vulkan_root);
  auto maybe_app = create_3d_app(vulkan_root);

  if (!maybe_app) {
    std::cerr << maybe_app.error() << std::endl;
    return EXIT_FAILURE;
  }

  auto mandelbulb_app = std::move(maybe_app.value());

  auto status = vulkan_root.run_app(mandelbulb_app);

  vulkan_root.end_glfw_instance();

  if (!status) {
    std::cerr << status.error() << std::endl;
    return EXIT_FAILURE;
  }
}
