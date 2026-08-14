#ifndef VULKAN_WRAPPER_PIPELINE_GRAPHICS_PIPELINE_CONTAINER_HH
#define VULKAN_WRAPPER_PIPELINE_GRAPHICS_PIPELINE_CONTAINER_HH

#include "../shaders/shader_utils.hh"

#include "vulkan/vulkan.hpp"
#include <expected>
#include <string>
#include <vector>
#include <vulkan/vulkan_raii.hpp>
namespace GraphicsPipeline {

struct PipelineContainerCreateInfo {
  std::string vertex_shader_path;
  std::string frag_shader_path;
  std::string vertex_main_func_name;
  std::string frag_main_func_name;

  // Info on the types being passed to the shader
  vk::VertexInputBindingDescription binding_description;
  std::vector<vk::VertexInputAttributeDescription> attribute_descriptions;

  // These affect how much of the frame buffer gets drawn to, and how much of
  // the final image appears
  vk::Viewport screen_region;
  vk::Rect2D image_slice;

  // Render Info
  vk::PolygonMode polygon_mode;
  vk::CullModeFlagBits cull_mode;

  vk::PipelineColorBlendStateCreateInfo colour_blend_data;

  std::vector<vk::PushConstantRange> push_constant_ranges;
};

class PipelineContainer {
public:
  PipelineContainer() = delete;

  auto layout() -> vk::raii::PipelineLayout &;
  auto pipeline() -> vk::raii::Pipeline &;

  static auto create(PipelineContainerCreateInfo info, vk::raii::Device &device,
                     vk::SurfaceFormatKHR &surface_format)
      -> std::expected<PipelineContainer, std::string>;

private:
  PipelineContainer(vk::raii::PipelineLayout &&layout,
                    vk::raii::Pipeline &&pipeline)
      : _layout(std::move(layout)), _pipeline(std::move(pipeline)) {}

private:
  vk::raii::PipelineLayout _layout;
  vk::raii::Pipeline _pipeline;
};

} // namespace GraphicsPipeline

#endif
