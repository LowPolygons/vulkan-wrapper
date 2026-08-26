#ifndef VULKAN_WRAPPER_INSTANCE_AND_SURFACE_CONTAINER_HH
#define VULKAN_WRAPPER_INSTANCE_AND_SURFACE_CONTAINER_HH

#include <expected>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan_raii.hpp>

namespace InstanceAndSurface {

struct VulkanInstanceAndSurfaceCreateInfo {
  vk::ApplicationInfo app_info;
  bool validation_layers_enabled;
  std::vector<const char *> validation_layers;
  std::vector<const char *> additional_extensions;
};

class VulkanInstanceAndSurface {
public:
  VulkanInstanceAndSurface() = delete;

  static auto create(VulkanInstanceAndSurfaceCreateInfo info,
                     const vk::raii::Context &context_ref, GLFWwindow *window)
      -> std::expected<VulkanInstanceAndSurface, std::string>;

  auto instance() -> vk::raii::Instance &;
  auto surface() -> vk::raii::SurfaceKHR &;

private:
  VulkanInstanceAndSurface(vk::raii::Instance &&instance,
                           vk::raii::SurfaceKHR &&surface)
      : _instance(std::move(instance)), _surface(std::move(surface)) {}

private:
  vk::raii::Instance _instance;
  vk::raii::SurfaceKHR _surface;
};

namespace FactoryHelper {
auto validate_layers(const std::vector<const char *> required,
                     const std::vector<vk::LayerProperties> available)
    -> std::expected<void, std::string>;
auto validate_extensions(const std::vector<const char *> required,
                         const std::vector<vk::ExtensionProperties> available)
    -> std::expected<void, std::string>;
} // namespace FactoryHelper

} // namespace InstanceAndSurface
#endif
