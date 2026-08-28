#ifndef VULKAN_WRAPPER_BUFFERS_COPY_BUFFER_HH
#define VULKAN_WRAPPER_BUFFERS_COPY_BUFFER_HH

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
namespace BufferUtils {

struct BufferCopyData {
  vk::DeviceSize buff_size;
  std::size_t source_offset_UNUSED;
  std::size_t dest_offset_UNUSED;
};

auto copy_host_buffer_to_gpu_buffer(const vk::raii::Device &device,
                                    const vk::raii::Queue &queue,
                                    const vk::raii::CommandPool &command_pool,
                                    vk::raii::Buffer &source_buffer,
                                    vk::raii::Buffer &dest_buffer,
                                    BufferCopyData buff_data) -> void;
} // namespace BufferUtils
#endif
