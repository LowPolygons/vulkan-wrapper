
#ifndef VULKAN_WRAPPER_BUFFERS_UTIL_TRANSITION_LAYOUT_HH
#define VULKAN_WRAPPER_BUFFERS_UTIL_TRANSITION_LAYOUT_HH

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace BufferUtils {

void transition_image_layout_on_buffer(vk::raii::CommandBuffer &buffer_ref,
                                       vk::Image &image_ref,
                                       vk::ImageLayout old_layout,
                                       vk::ImageLayout new_layout,
                                       vk::AccessFlags2 src_access_mask,
                                       vk::AccessFlags2 dst_access_mask,
                                       vk::PipelineStageFlags2 src_stage_mask,
                                       vk::PipelineStageFlags2 dst_stage_mask);
} // namespace BufferUtils

#endif
