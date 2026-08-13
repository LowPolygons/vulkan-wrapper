
#include "data_buffer_container.hh"
#include "../device/helpers.hh"
#include <vulkan/vulkan_raii.hpp>

auto BufferUtils::FactoryHelper::allocate_memory_on_host_and_device(
    vk::raii::Device &device, vk::raii::PhysicalDevice &physical_device,
    vk::DeviceSize buffer_size) -> MaybeHostAndDeviceMem {
#define HOST_BUFFER_USAGE_FLAGS vk::BufferUsageFlagBits::eTransferSrc
#define HOST_MEMORY_PROPERTY_FLAGS                                             \
  vk::MemoryPropertyFlagBits::eHostVisible |                                   \
      vk::MemoryPropertyFlagBits::eHostCoherent
#define DEVICE_BUFFER_USAGE_FLAGS                                              \
  vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst
#define DEVICE_MEMORY_PROPERTY_FLAGS vk::MemoryPropertyFlagBits::eDeviceLocal

  auto maybe_host_vertex_buf_and_mem =
      std::expected<std::pair<vk::raii::Buffer, vk::raii::DeviceMemory>,
                    std::string>{DeviceUtil::create_buffer(
          device, physical_device, buffer_size, HOST_BUFFER_USAGE_FLAGS,
          HOST_MEMORY_PROPERTY_FLAGS)};
  auto maybe_device_vertex_buf_and_mem =
      std::expected<std::pair<vk::raii::Buffer, vk::raii::DeviceMemory>,
                    std::string>{DeviceUtil::create_buffer(
          device, physical_device, buffer_size, DEVICE_BUFFER_USAGE_FLAGS,
          DEVICE_MEMORY_PROPERTY_FLAGS)};

  return BufferUtils::MaybeHostAndDeviceMem{
      .host = std::move(maybe_host_vertex_buf_and_mem),
      .device = std::move(maybe_device_vertex_buf_and_mem)};
}
