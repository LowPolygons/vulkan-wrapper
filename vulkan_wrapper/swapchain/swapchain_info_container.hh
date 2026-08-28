#ifndef VULKAN_WRAPPER_SWAPCHAIN_INFO_CONTAINER_HH
#define VULKAN_WRAPPER_SWAPCHAIN_INFO_CONTAINER_HH

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <expected>
#include <vulkan/vulkan_raii.hpp>
namespace SwapchainInfo {

struct SwapchainInfoObjectRefs {
  vk::raii::Instance &instance_ref;
  vk::raii::PhysicalDevice &physical_device_ref;
  vk::raii::Device &device_ref;
  vk::raii::Queue &graphics_queue_ref;
  vk::raii::SurfaceKHR &surface_ref;
  GLFWwindow *window;
};

struct SwapchainInfoContainerCreateInfo {
  vk::PresentModeKHR present_mode;
  // INFO: add more things here if additional variability is needed
};

class SwapchainInfoContainer {
public:
  SwapchainInfoContainer() = delete;

  static auto create(SwapchainInfoContainerCreateInfo info,
                     const SwapchainInfoObjectRefs object_refs)
      -> std::expected<SwapchainInfoContainer, std::string>;

  auto swap_chain() -> vk::raii::SwapchainKHR &;
  auto images() -> std::vector<vk::Image> &;
  auto image_views() -> std::vector<vk::raii::ImageView> &;
  auto surface_format() -> vk::SurfaceFormatKHR &;
  auto dimensions() -> vk::Extent2D &;

  auto wipe() -> void;

private:
  SwapchainInfoContainer(vk::raii::SwapchainKHR &&swap_chain,
                         std::vector<vk::Image> &&images,
                         std::vector<vk::raii::ImageView> &&image_views,
                         vk::SurfaceFormatKHR &&surface_format,
                         vk::Extent2D &&dimensions)
      : _swap_chain(std::move(swap_chain)), _images(std::move(images)),
        _image_views(std::move(image_views)),
        _surface_format(std::move(surface_format)),
        _dimensions(std::move(dimensions)) {}

  vk::raii::SwapchainKHR _swap_chain;
  std::vector<vk::Image> _images;
  std::vector<vk::raii::ImageView> _image_views;
  vk::SurfaceFormatKHR _surface_format;
  vk::Extent2D _dimensions;
};

namespace FactoryHelper {
auto choose_surface_format(std::vector<vk::SurfaceFormatKHR> available_formats)
    -> std::expected<vk::SurfaceFormatKHR, std::string>;

auto choose_extent(vk::SurfaceCapabilitiesKHR surface_capabilities,
                   GLFWwindow *window)
    -> std::expected<vk::Extent2D, std::string>;

auto get_image_views(std::vector<vk::Image> &images,
                     vk::SurfaceFormatKHR surface_format,
                     const vk::raii::Device &device)
    -> std::expected<std::vector<vk::raii::ImageView>, std::string>;

} // namespace FactoryHelper

} // namespace SwapchainInfo
#endif
