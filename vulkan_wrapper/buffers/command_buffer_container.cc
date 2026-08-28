#include "vulkan_wrapper/buffers/command_buffer_container.hh"

auto BufferUtils::CommandPoolAndBuffersContainer::create(
    CommandBufferContainerCreateInfo info, const vk::raii::Device &device,
    uint32_t queue_index)
    -> std::expected<CommandPoolAndBuffersContainer, std::string> {
  if (device == nullptr)
    return std::unexpected(
        "Tried to create command buffers when device was nullptr");

  auto pool_info = vk::CommandPoolCreateInfo{
      .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
      .queueFamilyIndex = queue_index};

  auto command_pool = vk::raii::CommandPool(device, pool_info);

  auto buffer_alloc_info = vk::CommandBufferAllocateInfo{
      .commandPool = command_pool,
      .level = vk::CommandBufferLevel::ePrimary,
      .commandBufferCount = static_cast<uint32_t>(info.num_frames_in_flight)};

  auto command_buffers = vk::raii::CommandBuffers(device, buffer_alloc_info);

  auto object = CommandPoolAndBuffersContainer(std::move(command_pool),
                                               std::move(command_buffers));

  return object;
}

auto BufferUtils::CommandPoolAndBuffersContainer::get_buffer_ref(
    std::size_t index) -> vk::raii::CommandBuffer & {
  return _command_buffers[index];
}

auto BufferUtils::CommandPoolAndBuffersContainer::command_pool()
    -> vk::raii::CommandPool & {
  return _command_pool;
}
