#ifndef VULKAN_WRAPPER_BUFFERS_DATA_BUFFER_CONTAINER_HH
#define VULKAN_WRAPPER_BUFFERS_DATA_BUFFER_CONTAINER_HH

#include "vulkan_wrapper/buffers/buffer_copy.hh"
#include <cstddef>
#include <expected>
#include <string>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace BufferUtils {

template <typename VBT, typename IT> // Vertex Buffer Type, Index Type
  requires std::is_arithmetic_v<IT>
struct BufferContainerCreateInfo {
  std::size_t num_vertices;
  std::size_t num_indices;
  std::vector<VBT> &vertex_data;
  std::vector<IT> &index_data;
};

struct BufferAndDeviceMemory {
  vk::raii::Buffer buffer;
  vk::raii::DeviceMemory memory;
  std::size_t size;
};

struct DeviceBundleRefs {
  vk::raii::PhysicalDevice &physical_ref;
  vk::raii::Device &logical_ref;
  vk::raii::Queue &queue_ref;
  vk::raii::CommandPool &command_pool;
};

struct MaybeHostAndDeviceMem {
  std::expected<std::pair<vk::raii::Buffer, vk::raii::DeviceMemory>,
                std::string>
      host;
  std::expected<std::pair<vk::raii::Buffer, vk::raii::DeviceMemory>,
                std::string>
      device;
};

template <typename VBT, typename IT> // Vertex Buffer Type, Index Type
  requires std::is_arithmetic_v<IT>
class DataBufferContainer {
public:
  DataBufferContainer() = delete;

  static auto create(BufferContainerCreateInfo<VBT, IT> info,
                     DeviceBundleRefs devices)
      -> std::expected<DataBufferContainer<VBT, IT>, std::string>;

  auto vertices() -> BufferAndDeviceMemory &;
  auto indices() -> BufferAndDeviceMemory &;

private:
  DataBufferContainer<VBT, IT>(BufferAndDeviceMemory &&vertices,
                               BufferAndDeviceMemory &&indices)
      : _vertices(std::move(vertices)), _indices(std::move(indices)) {}

private:
  BufferAndDeviceMemory _vertices;
  BufferAndDeviceMemory _indices;
};

namespace FactoryHelper {

auto allocate_memory_on_host_and_device(
    const vk::raii::Device &device,
    const vk::raii::PhysicalDevice &physical_device, vk::DeviceSize buffer_size)
    -> MaybeHostAndDeviceMem;

// staging_mem.mapMemory(DeviceSize offset, DeviceSize size)
template <typename T>
auto copy_data_to_host_buffer(vk::raii::DeviceMemory &staging_memory,
                              vk::DeviceSize offset, vk::DeviceSize buffer_size,
                              T *data_pointer) -> void;

} // namespace FactoryHelper

} // namespace BufferUtils
  //
template <typename VBT, typename IT> // Vertex Buffer Type, Index Type
  requires std::is_arithmetic_v<IT>
auto BufferUtils::DataBufferContainer<VBT, IT>::indices()
    -> BufferAndDeviceMemory & {
  return _indices;
}

template <typename VBT, typename IT> // Vertex Buffer Type, Index Type
  requires std::is_arithmetic_v<IT>
auto BufferUtils::DataBufferContainer<VBT, IT>::vertices()
    -> BufferAndDeviceMemory & {
  return _vertices;
}

template <typename VBT, typename IT> // Vertex Buffer Type, Index Type
  requires std::is_arithmetic_v<IT>
auto BufferUtils::DataBufferContainer<VBT, IT>::create(
    BufferUtils::BufferContainerCreateInfo<VBT, IT> info,
    const BufferUtils::DeviceBundleRefs devices)
    -> std::expected<BufferUtils::DataBufferContainer<VBT, IT>, std::string> {
  auto vertex_buffer_size = sizeof(VBT) * info.num_vertices;
  auto index_buffer_size = sizeof(IT) * info.num_indices;

  auto maybe_vertex_data = FactoryHelper::allocate_memory_on_host_and_device(
      devices.logical_ref, devices.physical_ref, vertex_buffer_size);
  auto maybe_index_data = FactoryHelper::allocate_memory_on_host_and_device(
      devices.logical_ref, devices.physical_ref, index_buffer_size);

  if (!maybe_vertex_data.host)
    return std::unexpected(maybe_vertex_data.host.error());
  if (!maybe_vertex_data.device)
    return std::unexpected(maybe_vertex_data.device.error());

  if (!maybe_index_data.host)
    return std::unexpected(maybe_index_data.host.error());
  if (!maybe_index_data.device)
    return std::unexpected(maybe_index_data.device.error());

  auto [vert_stage_buff, vert_stage_mem] =
      std::move(maybe_vertex_data.host.value());
  auto [indx_stage_buff, indx_stage_mem] =
      std::move(maybe_index_data.host.value());

  auto [vert_buff, vert_mem] = std::move(maybe_vertex_data.device.value());
  auto [indx_buff, indx_mem] = std::move(maybe_index_data.device.value());

  // Copy data to host memory
  FactoryHelper::copy_data_to_host_buffer(vert_stage_mem, 0, vertex_buffer_size,
                                          info.vertex_data.data());
  FactoryHelper::copy_data_to_host_buffer(indx_stage_mem, 0, index_buffer_size,
                                          info.index_data.data());
  // GPU copy now available
  BufferUtils::copy_host_buffer_to_gpu_buffer(
      devices.logical_ref, devices.queue_ref, devices.command_pool,
      vert_stage_buff, vert_buff,
      BufferUtils::BufferCopyData{.buff_size = vertex_buffer_size});
  BufferUtils::copy_host_buffer_to_gpu_buffer(
      devices.logical_ref, devices.queue_ref, devices.command_pool,
      indx_stage_buff, indx_buff,
      BufferUtils::BufferCopyData{.buff_size = index_buffer_size});

  auto object = DataBufferContainer<VBT, IT>(
      BufferAndDeviceMemory{.buffer = std::move(vert_buff),
                            .memory = std::move(vert_mem),
                            .size = vertex_buffer_size},
      BufferAndDeviceMemory{.buffer = std::move(indx_buff),
                            .memory = std::move(indx_mem),
                            .size = index_buffer_size});
  return object;
}

template <typename T>
auto BufferUtils::FactoryHelper::copy_data_to_host_buffer(
    vk::raii::DeviceMemory &staging_memory, vk::DeviceSize offset,
    vk::DeviceSize buffer_size, T *data_pointer) -> void {
  void *mem_location = staging_memory.mapMemory(offset, buffer_size);

  memcpy(mem_location, data_pointer, static_cast<std::size_t>(buffer_size));

  staging_memory.unmapMemory();
}

#endif
