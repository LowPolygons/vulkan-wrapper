#include "transition_buffer_layout.hh"

// WARN: copied directly from
// The idea is that images are generic to be specialisable fro different
// things, such as for presentation to the screen or for color attachements
// etc
// This function transitions the iamge layout from one layout to another
// https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/03_Drawing/01_Command_buffers.html
void BufferUtils::transition_image_layout_on_buffer(
    vk::raii::CommandBuffer &buffer_ref, vk::Image &image_ref,
    vk::ImageLayout old_layout, vk::ImageLayout new_layout,
    vk::AccessFlags2 src_access_mask, vk::AccessFlags2 dst_access_mask,
    vk::PipelineStageFlags2 src_stage_mask,
    vk::PipelineStageFlags2 dst_stage_mask) {
  vk::ImageMemoryBarrier2 barrier = {
      .srcStageMask = src_stage_mask,
      .srcAccessMask = src_access_mask,
      .dstStageMask = dst_stage_mask,
      .dstAccessMask = dst_access_mask,
      .oldLayout = old_layout,
      .newLayout = new_layout,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = image_ref,
      .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor,
                           .baseMipLevel = 0,
                           .levelCount = 1,
                           .baseArrayLayer = 0,
                           .layerCount = 1}};
  vk::DependencyInfo dependency_info = {.dependencyFlags = {},
                                        .imageMemoryBarrierCount = 1,
                                        .pImageMemoryBarriers = &barrier};
  buffer_ref.pipelineBarrier2(dependency_info);
}
