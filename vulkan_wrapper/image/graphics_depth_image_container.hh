#ifndef VULKAN_WRAPPER_PIPELINE_GRPAHICS_DEPTH_CONTAINER_HH
#define VULKAN_WRAPPER_PIPELINE_GRPAHICS_DEPTH_CONTAINER_HH

#include <expected>
#include <vulkan/vulkan_raii.hpp>
namespace GraphicsPipeline {

struct DepthDataContainerCreateInfo {
  uint32_t swapchain_width;
  uint32_t swapchain_height;
  vk::Format preferred_format;
};

class DepthDataContainer {
public:
  DepthDataContainer() = delete;

  static auto create(GraphicsPipeline::DepthDataContainerCreateInfo info,
                     vk::raii::Device &device,
                     vk::raii::PhysicalDevice &physical_device)
      -> std::expected<DepthDataContainer, std::string>;

  auto image() -> vk::Image;
  auto memory() -> vk::raii::DeviceMemory &;
  auto view() -> vk::raii::ImageView &;

private:
  DepthDataContainer(vk::raii::Image &&d_i, vk::raii::DeviceMemory &&i_m,
                     vk::raii::ImageView &&i_v)
      : depth_image(std::move(d_i)), image_memory(std::move(i_m)),
        image_view(std::move(i_v)) {}

private:
  vk::raii::Image depth_image;
  vk::raii::DeviceMemory image_memory;
  vk::raii::ImageView image_view;
};
} // namespace GraphicsPipeline
#endif
