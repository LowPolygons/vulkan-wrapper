#include "graphics_depth_image_container.hh"
#include "vulkan_wrapper/image/image_utils.hh"

auto GraphicsPipeline::DepthDataContainer::image() -> vk::Image {
  return *depth_image;
}
auto GraphicsPipeline::DepthDataContainer::memory()
    -> vk::raii::DeviceMemory & {
  return image_memory;
}
auto GraphicsPipeline::DepthDataContainer::view() -> vk::raii::ImageView & {
  return image_view;
}
auto GraphicsPipeline::DepthDataContainer::create(
    GraphicsPipeline::DepthDataContainerCreateInfo info,
    vk::raii::Device &device, vk::raii::PhysicalDevice &physical_device)
    -> std::expected<DepthDataContainer, std::string> {
  auto depth_image = vk::raii::Image{nullptr};
  auto depth_image_memory = vk::raii::DeviceMemory{nullptr};

  auto maybe_image_and_memory = ImageUtils::create_image(
      device, physical_device, {info.swapchain_width, info.swapchain_height},
      info.preferred_format, vk::ImageTiling::eOptimal,
      vk::ImageUsageFlagBits::eDepthStencilAttachment,
      vk::MemoryPropertyFlagBits::eDeviceLocal);

  if (!maybe_image_and_memory)
    return std::unexpected("DepthDataContainer Init error: " +
                           maybe_image_and_memory.error());

  std::tie(depth_image, depth_image_memory) =
      std::move(maybe_image_and_memory.value());

  auto create_image_view = [&device](const vk::raii::Image &image,
                                     vk::Format format) {
    auto view_info = vk::ImageViewCreateInfo{
        .image = image,
        .viewType = vk::ImageViewType::e2D,
        .format = format,
        .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eDepth,
                             .baseMipLevel = 0,
                             .levelCount = 1,
                             .baseArrayLayer = 0,
                             .layerCount = 1}};
    return vk::raii::ImageView(device, view_info);
  };

  auto image_view = create_image_view(depth_image, info.preferred_format);

  auto object =
      DepthDataContainer(std::move(depth_image), std::move(depth_image_memory),
                         std::move(image_view));

  return object;
}
