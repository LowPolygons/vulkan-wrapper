#include "image_utils.hh"
#include "vulkan_wrapper/device/helpers.hh"

auto ImageUtils::create_image(vk::raii::Device &device,
                              vk::raii::PhysicalDevice &physical,
                              std::pair<uint32_t, uint32_t> image_dimensions,
                              vk::Format image_format, vk::ImageTiling tiling,
                              vk::ImageUsageFlags usage,
                              vk::MemoryPropertyFlags properties)
    -> std::expected<std::pair<vk::raii::Image, vk::raii::DeviceMemory>,
                     std::string> {
  auto image_create_info = vk::ImageCreateInfo{
      .imageType = vk::ImageType::e2D,
      .format = image_format,
      .extent = {image_dimensions.first, image_dimensions.second, 1},
      .mipLevels = 1,
      .arrayLayers = 1,
      .samples = vk::SampleCountFlagBits::e1,
      .tiling = tiling,
      .usage = usage,
      .sharingMode = vk::SharingMode::eExclusive};

  auto image = vk::raii::Image(device, image_create_info);

  auto memory_requirements =
      vk::MemoryRequirements{image.getMemoryRequirements()};

  auto maybe_memory_valid = DeviceUtil::find_memory_type(
      physical, memory_requirements.memoryTypeBits, properties);

  if (!maybe_memory_valid)
    return std::unexpected("Image Creation Error: " +
                           maybe_memory_valid.error());

  auto allocation_info = vk::MemoryAllocateInfo{
      .allocationSize = memory_requirements.size,
      .memoryTypeIndex = maybe_memory_valid.value(),
  };

  auto image_memory = vk::raii::DeviceMemory(device, allocation_info);

  image.bindMemory(image_memory, 0);

  return std::pair{std::move(image), std::move(image_memory)};
}
