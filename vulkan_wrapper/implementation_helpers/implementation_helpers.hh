#ifndef VULKAN_WRAPPER_IMPLEMENTATION_HELPERS_HH
#define VULKAN_WRAPPER_IMPLEMENTATION_HELPERS_HH

#include "vulkan/vulkan.hpp"
#include "vulkan_wrapper/buffers/transition_buffer_layout.hh"
#include "vulkan_wrapper/pipeline/graphics_pipeline_container.hh"
#include <glm/glm.hpp>

namespace ImplementationHelp {

namespace FragApp {

struct Vertex {
  glm::vec2 position;
  static vk::VertexInputBindingDescription get_binding_descriptions() {
    return {.binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = vk::VertexInputRate::eVertex};
  }
  static std::vector<vk::VertexInputAttributeDescription>
  get_attribute_descriptions() {
    return {{vk::VertexInputAttributeDescription{
        .location = 0,
        .binding = 0,
        .format = vk::Format::eR32G32Sfloat,
        .offset = offsetof(Vertex, position)}}};
  }
};

auto get_frag_app_vertices() -> std::vector<Vertex>;
auto get_frag_app_indices() -> std::vector<uint16_t>;

}; // namespace FragApp

namespace Graphics {

template <typename T>
auto get_partial_graphics_pipeline_details(
    std::string vertex_path, std::string frag_path, std::string vertex_main,
    std::string frag_main, std::size_t screen_width, std::size_t screen_height,
    vk::PipelineColorBlendAttachmentState &app_colour_blend_data)
    -> GraphicsPipeline::PipelineContainerCreateInfo {

  auto pipeline_details = GraphicsPipeline::PipelineContainerCreateInfo{
      .vertex_shader_path = vertex_path,
      .frag_shader_path = frag_path,
      .vertex_main_func_name = vertex_main,
      .frag_main_func_name = frag_main,
      .binding_description = T::get_binding_descriptions(),
      .attribute_descriptions = T::get_attribute_descriptions(),
      .screen_region = {0, 0, static_cast<float>(screen_width),
                        static_cast<float>(screen_height), 0, 1},
      .image_slice = {vk::Offset2D(0, 0),
                      {static_cast<uint32_t>(screen_width),
                       static_cast<uint32_t>(screen_height)}},
      .polygon_mode = vk::PolygonMode::eFill,
      .cull_mode = vk::CullModeFlagBits::eBack,
      .front_face = vk::FrontFace::eClockwise,
      .colour_blend_data =
          vk::PipelineColorBlendStateCreateInfo{.logicOpEnable = vk::False,
                                                .logicOp = vk::LogicOp::eClear,
                                                .attachmentCount = 1,
                                                .pAttachments =
                                                    &app_colour_blend_data},
  };

  return pipeline_details;
}
} // namespace Graphics

namespace CommandBuffer {

template <typename PC_OR_VOID> struct ComputeStage {
  std::array<int, 3> workgroups;
  std::optional<vk::MemoryBarrier2> maybe_proceeding_barrier;
};

enum class IndicesType {
  INT_16,
  INT_32,
};

template <typename PC_OR_VOID> struct GraphicsStage {
  vk::Image &transition_image;
  vk::raii::ImageView &image_view;
  vk::Rect2D render_area;
  vk::raii::Buffer &vertices_buffer_ref;
  vk::raii::Buffer &indices_buffer_ref;
  uint32_t num_indices;
  IndicesType indices_type;
  vk::ClearColorValue default_colour;
};

template <typename PC_OR_VOID>
static auto record_compute_stage(vk::raii::CommandBuffer &command_buffer,
                                 vk::raii::Pipeline &pipeline,
                                 vk::raii::PipelineLayout &layout_ref,
                                 ComputeStage<PC_OR_VOID> stage_info,
                                 std::optional<PC_OR_VOID> maybe_push_constant)
    -> void;

template <typename PC_OR_VOID>
static auto record_graphics_stage(vk::raii::CommandBuffer &command_buffer,
                                  vk::raii::Pipeline &pipeline,
                                  vk::raii::PipelineLayout &layout_ref,
                                  GraphicsStage<PC_OR_VOID> stage_info,
                                  std::optional<PC_OR_VOID> maybe_push_constant)
    -> void;
} // namespace CommandBuffer
} // namespace ImplementationHelp

template <typename PC_OR_VOID>
auto ImplementationHelp::CommandBuffer::record_compute_stage(
    vk::raii::CommandBuffer &command_buffer, vk::raii::Pipeline &pipeline,
    vk::raii::PipelineLayout &layout_ref, ComputeStage<PC_OR_VOID> stage_info,
    std::optional<PC_OR_VOID> maybe_push_constant) -> void {

  command_buffer.bindPipeline(vk::PipelineBindPoint::eCompute, pipeline);

  if constexpr (!std::is_void_v<PC_OR_VOID>)
    command_buffer.pushConstants(layout_ref, vk::ShaderStageFlagBits::eCompute,
                                 0, sizeof(PC_OR_VOID),
                                 &maybe_push_constant.value());

  command_buffer.dispatch(stage_info.workgroups[0], stage_info.workgroups[1],
                          stage_info.workgroups[2]);

  if (stage_info.maybe_proceeding_barrier.has_value()) {
    command_buffer.pipelineBarrier2(vk::DependencyInfo{
        .memoryBarrierCount = 1,
        .pMemoryBarriers = &stage_info.maybe_proceeding_barrier.value()});
  }
}

template <typename PC_OR_VOID>
auto ImplementationHelp::CommandBuffer::record_graphics_stage(
    vk::raii::CommandBuffer &command_buffer, vk::raii::Pipeline &pipeline,
    vk::raii::PipelineLayout &layout_ref, GraphicsStage<PC_OR_VOID> stage_info,
    std::optional<PC_OR_VOID> maybe_push_constant) -> void {

  BufferUtils::transition_image_layout_on_buffer(
      command_buffer, stage_info.transition_image, vk::ImageLayout::eUndefined,
      vk::ImageLayout::eColorAttachmentOptimal, {},
      vk::AccessFlagBits2::eColorAttachmentWrite,
      vk::PipelineStageFlagBits2::eColorAttachmentOutput,
      vk::PipelineStageFlagBits2::eColorAttachmentOutput);

  auto attachment_info = vk::RenderingAttachmentInfo{
      .imageView = stage_info.image_view,
      .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .clearValue = stage_info.default_colour,
  };

  auto rendering_info =
      vk::RenderingInfo{.renderArea = stage_info.render_area,
                        .layerCount = 1,
                        .colorAttachmentCount = 1,
                        .pColorAttachments = &attachment_info};

  command_buffer.beginRendering(rendering_info);
  command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

  if constexpr (!std::is_void_v<PC_OR_VOID>)
    command_buffer.pushConstants(
        layout_ref, vk::ShaderStageFlagBits::eAllGraphics, 0,
        sizeof(PC_OR_VOID), &maybe_push_constant.value());

  command_buffer.bindVertexBuffers(0, *stage_info.vertices_buffer_ref, {0});

  if (stage_info.indices_type == IndicesType::INT_16) {
    command_buffer.bindIndexBuffer(stage_info.indices_buffer_ref, 0,
                                   vk::IndexType::eUint16);
  } else {
    command_buffer.bindIndexBuffer(stage_info.indices_buffer_ref, 0,
                                   vk::IndexType::eUint32);
  }

  command_buffer.drawIndexed(stage_info.num_indices, 1, 0, 0, 0);

  BufferUtils::transition_image_layout_on_buffer(
      command_buffer, stage_info.transition_image,
      vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR,
      vk::AccessFlagBits2::eColorAttachmentWrite, {},
      vk::PipelineStageFlagBits2::eColorAttachmentOutput,
      vk::PipelineStageFlagBits2::eBottomOfPipe);
}
#endif
