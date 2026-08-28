#ifndef VULKAN_WRAPPER_ARBITRARY_GPU_DATA_BUFFER_HH
#define VULKAN_WRAPPER_ARBITRARY_GPU_DATA_BUFFER_HH

#include "vulkan_wrapper/buffers/buffer_copy.hh"
#include <expected>
#include <print>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "vulkan_wrapper/device/helpers.hh"

namespace BufferUtils {

// WARN: data buffer has an identical class, this is just me being lazy
struct NeededObjects {
  vk::raii::PhysicalDevice &physical_ref;
  vk::raii::Device &logical_ref;
  vk::raii::Queue &queue_ref;
  vk::raii::CommandPool &command_pool;
};

template <typename BT> class ArbitraryGpuDataContainer {
public:
  ArbitraryGpuDataContainer<BT>() = delete;

  static auto create(const NeededObjects object_refs,
                     std::vector<BT> initial_data)
      -> std::expected<ArbitraryGpuDataContainer, std::string>;

  auto buffer() -> vk::raii::Buffer &;
  auto memory() -> vk::raii::DeviceMemory &;
  auto buffer_size() -> std::size_t;
  auto address(const vk::raii::Device &device) -> vk::DeviceAddress;

private:
  ArbitraryGpuDataContainer<BT>(vk::raii::Buffer &&buffer,
                                vk::raii::DeviceMemory &&memory,
                                std::size_t size)
      : _buffer(std::move(buffer)), _memory(std::move(memory)), _size(size) {}

private:
  vk::raii::Buffer _buffer;
  vk::raii::DeviceMemory _memory;
  std::size_t _size;
};
} // namespace BufferUtils

template <typename BT>
auto BufferUtils::ArbitraryGpuDataContainer<BT>::memory()
    -> vk::raii::DeviceMemory & {
  return _memory;
}

template <typename BT>
auto BufferUtils::ArbitraryGpuDataContainer<BT>::buffer()
    -> vk::raii::Buffer & {
  return _buffer;
}
template <typename BT>
auto BufferUtils::ArbitraryGpuDataContainer<BT>::address(
    const vk::raii::Device &device) -> vk::DeviceAddress {
  auto info = vk::BufferDeviceAddressInfo{};
  info.buffer = _buffer;

  return device.getBufferAddress(info);
}

template <typename BT>
auto BufferUtils::ArbitraryGpuDataContainer<BT>::buffer_size() -> std::size_t {
  return _size;
}

template <typename BT>
auto BufferUtils::ArbitraryGpuDataContainer<BT>::create(
    const NeededObjects object_refs, std::vector<BT> initial_data)
    -> std::expected<ArbitraryGpuDataContainer, std::string> {
  auto buffer_size = initial_data.size() * sizeof(BT);

  // Staging buffer - host accessible, perform memcpy of initial data
  auto maybe_staging_buff_and_mem = DeviceUtil::create_buffer(
      object_refs.logical_ref, object_refs.physical_ref, buffer_size,
      vk::BufferUsageFlagBits::eTransferSrc,
      vk::MemoryPropertyFlagBits::eHostVisible |
          vk::MemoryPropertyFlagBits::eHostCoherent);

  if (!maybe_staging_buff_and_mem)
    return std::unexpected("Couldn't create staging buffer: " +
                           maybe_staging_buff_and_mem.error());

  auto [staging_buff, staging_mem] =
      std::move(maybe_staging_buff_and_mem.value());

  void *raw_memory = staging_mem.mapMemory(0, buffer_size);
  memcpy(raw_memory, initial_data.data(),
         static_cast<std::size_t>(buffer_size));
  staging_mem.unmapMemory();

  // Device Buffer -> this is the buffer that is stored in the object
  auto maybe_device_buff_and_mem = DeviceUtil::create_buffer(
      object_refs.logical_ref, object_refs.physical_ref, buffer_size,
      vk::BufferUsageFlagBits::eTransferDst |
          vk::BufferUsageFlagBits::eShaderDeviceAddress |
          vk::BufferUsageFlagBits::eStorageBuffer |
          vk::BufferUsageFlagBits::eShaderDeviceAddress,
      vk::MemoryPropertyFlagBits::eDeviceLocal);

  if (!maybe_device_buff_and_mem)
    return std::unexpected("Couldn't create the device buffer: " +
                           maybe_device_buff_and_mem.error());

  auto [device_buff, device_mem] = std::move(maybe_device_buff_and_mem.value());

  BufferUtils::copy_host_buffer_to_gpu_buffer(
      object_refs.logical_ref, object_refs.queue_ref, object_refs.command_pool,
      staging_buff, device_buff,
      BufferUtils::BufferCopyData{.buff_size = buffer_size});

  auto object = BufferUtils::ArbitraryGpuDataContainer<BT>(
      std::move(device_buff), std::move(device_mem), buffer_size);

  std::println(
      "[ArbitraryGpuDataContainer Info] Allocated a buffer of size {} Bytes "
      "on the device",
      buffer_size);
  return object;
}
#endif
