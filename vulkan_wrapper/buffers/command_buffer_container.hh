#ifndef VULKAN_WRAPPER_BUFFERS_COMMAND_BUFFER_CONTAINER_HH
#define VULKAN_WRAPPER_BUFFERS_COMMAND_BUFFER_CONTAINER_HH

#include <cstddef>
#include <expected>
#include <vulkan/vulkan_raii.hpp>
namespace BufferUtils {

struct CommandBufferContainerCreateInfo {
  std::size_t num_frames_in_flight;
};

class CommandPoolAndBuffersContainer {
public:
  CommandPoolAndBuffersContainer() = delete;

  auto command_pool() -> vk::raii::CommandPool &;
  auto get_buffer_ref(std::size_t index) -> vk::raii::CommandBuffer &;

  static auto create(CommandBufferContainerCreateInfo info,
                     const vk::raii::Device &device, uint32_t queue_index)
      -> std::expected<CommandPoolAndBuffersContainer, std::string>;

private:
  CommandPoolAndBuffersContainer(vk::raii::CommandPool &&pool,
                                 std::vector<vk::raii::CommandBuffer> &&buffers)
      : _command_pool(std::move(pool)), _command_buffers(std::move(buffers)) {}

  vk::raii::CommandPool _command_pool;
  std::vector<vk::raii::CommandBuffer> _command_buffers;
};

} // namespace BufferUtils

#endif
