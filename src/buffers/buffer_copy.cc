#include "buffer_copy.hh"

auto BufferUtil::copy_buffer(vk::raii::Device &device, vk::raii::Queue &queue,
                             vk::raii::CommandPool &command_pool,
                             vk::raii::Buffer &source_buffer,
                             vk::raii::Buffer &dest_buffer,
                             BufferUtil::BufferCopyData buff_data) -> void {
  // Spins up a temporary command buffer for copying the data across
  auto command_buffer_alloc_info =
      vk::CommandBufferAllocateInfo{.commandPool = command_pool,
                                    .level = vk::CommandBufferLevel::ePrimary,
                                    .commandBufferCount = 1};

  auto command_buffer = vk::raii::CommandBuffer{std::move(
      device.allocateCommandBuffers(command_buffer_alloc_info).front())};

  // Comand recording
  command_buffer.begin(
      {.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
  // The 0, 0 would be the source and dest offset from buff_data
  command_buffer.copyBuffer(*source_buffer, *dest_buffer,
                            vk::BufferCopy(0, 0, buff_data.buff_size));
  command_buffer.end();

  queue.submit(vk::SubmitInfo{.commandBufferCount = 1,
                              .pCommandBuffers = &*command_buffer},
               nullptr);
  queue.waitIdle();
}
