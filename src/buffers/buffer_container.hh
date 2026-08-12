#ifndef VULKAN_WRAPPER_BUFFERS_BUFFER_CONTAINER_HH
#define VULKAN_WRAPPER_BUFFERS_BUFFER_CONTAINER_HH

#include <cstddef>
#include <expected>
#include <string>
#include <vulkan/vulkan_raii.hpp>
namespace BufferUtils {

template <typename VBT> // Vertex Buffer Type
struct BufferContainerCreateInfo {
  std::size_t num_indices;
  std::size_t num_vertices;
};

struct BufferAndDeviceMemory {
  vk::raii::Buffer buffer;
  vk::raii::DeviceMemory memory;
};

struct DeviceBundleRefs {
  vk::raii::PhysicalDevice &physical_ref;
  vk::raii::Device &logical_ref;
};

template <typename VBT> class BufferContainer {
public:
  BufferContainer() = delete;

private:
  static auto create(BufferContainerCreateInfo<VBT> info,
                     DeviceBundleRefs devices)
      -> std::expected<BufferContainer, std::string>;

  BufferContainer<VBT>(BufferAndDeviceMemory &&vertices,
                       BufferAndDeviceMemory &&indices)
      : _vertices(std::move(vertices)), _indices(std::move(indices)) {}

  auto vertices() -> BufferAndDeviceMemory &;
  auto indices() -> BufferAndDeviceMemory &;

private:
  BufferAndDeviceMemory _vertices;
  BufferAndDeviceMemory _indices;
};

} // namespace BufferUtils
#endif
