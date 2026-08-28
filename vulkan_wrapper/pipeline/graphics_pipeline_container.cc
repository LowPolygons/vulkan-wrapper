
#include "vulkan_wrapper/pipeline/graphics_pipeline_container.hh"
#include <vulkan/vulkan_raii.hpp>

#include "vulkan_wrapper/shaders/shader_utils.hh"

auto GraphicsPipeline::PipelineContainer::pipeline() -> vk::raii::Pipeline & {
  return _pipeline;
}

auto GraphicsPipeline::PipelineContainer::layout()
    -> vk::raii::PipelineLayout & {
  return _layout;
}

auto GraphicsPipeline::PipelineContainer::viewport() -> vk::Viewport {
  return screen_viewport;
}
auto GraphicsPipeline::PipelineContainer::scissor() -> vk::Rect2D {
  return image_scissor;
}
auto GraphicsPipeline::PipelineContainer::update_dynamic_objects(
    vk::Extent2D &swapchain_image_size) -> void {
  screen_viewport.setWidth(swapchain_image_size.width);
  screen_viewport.setHeight(swapchain_image_size.height);

  image_scissor.setExtent(swapchain_image_size);
}

auto GraphicsPipeline::PipelineContainer::create(
    PipelineContainerCreateInfo info, const vk::raii::Device &device,
    vk::SurfaceFormatKHR &surface_format,
    vk::raii::DescriptorSetLayout *descriptor_set_layout)
    -> std::expected<PipelineContainer, std::string> {
  if (device == nullptr)
    return std::unexpected(
        "Tried to create a graphics pipeline before device was initialised");

  auto maybe_vertex_shader_bytecode =
      ShaderUtils::read_shader(info.vertex_shader_path);
  auto maybe_frag_shader_bytecode =
      ShaderUtils::read_shader(info.frag_shader_path);
  if (!maybe_vertex_shader_bytecode)
    return std::unexpected(maybe_vertex_shader_bytecode.error());
  if (!maybe_frag_shader_bytecode)
    return std::unexpected(maybe_frag_shader_bytecode.error());

  // TODO: Would be nice to be able to choose if you want both stages
  auto vertex_shader_module =
      vk::raii::ShaderModule{ShaderUtils::map_shader_bytes_to_shader_module(
          maybe_vertex_shader_bytecode.value(), device)};
  auto frag_shader_module =
      vk::raii::ShaderModule{ShaderUtils::map_shader_bytes_to_shader_module(
          maybe_frag_shader_bytecode.value(), device)};

  auto vertex_shader_stage_info = vk::PipelineShaderStageCreateInfo{
      .stage = vk::ShaderStageFlagBits::eVertex,
      .module = vertex_shader_module,
      .pName = info.vertex_main_func_name.c_str()};
  auto frag_shader_stage_info = vk::PipelineShaderStageCreateInfo{
      .stage = vk::ShaderStageFlagBits::eFragment,
      .module = frag_shader_module,
      .pName = info.frag_main_func_name.c_str()};
  auto shader_stages =
      std::vector{vertex_shader_stage_info, frag_shader_stage_info};

  // TODO: Read up on dynamic state a bit more
  auto dynamic_states = std::vector<vk::DynamicState>{
      vk::DynamicState::eViewport, vk::DynamicState::eScissor};
  auto dynamic_state_pipeline_info = vk::PipelineDynamicStateCreateInfo{
      .dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
      .pDynamicStates = dynamic_states.data()};

  // Describe the Vertex Input information
  auto vertex_input_info = vk::PipelineVertexInputStateCreateInfo{
      .vertexBindingDescriptionCount = 1,
      .pVertexBindingDescriptions = &info.binding_description,
      .vertexAttributeDescriptionCount =
          static_cast<uint32_t>(info.attribute_descriptions.size()),
      .pVertexAttributeDescriptions = info.attribute_descriptions.data()};

  // Input Assembly
  // TODO: This may need extending or moving out if this grows
  auto input_assembly = vk::PipelineInputAssemblyStateCreateInfo{
      .topology = vk::PrimitiveTopology::eTriangleList};

  auto viewport_info =
      vk::PipelineViewportStateCreateInfo{.viewportCount = 1,
                                          .pViewports = &info.screen_region,
                                          .scissorCount = 1,
                                          .pScissors = &info.image_slice};

  auto rasterisation_info = vk::PipelineRasterizationStateCreateInfo{
      .depthClampEnable = vk::False,
      .rasterizerDiscardEnable = vk::False,
      .polygonMode = info.polygon_mode,
      .cullMode = info.cull_mode,
      .frontFace = info.front_face,
      .depthBiasEnable = vk::False,
      .lineWidth = 1.0f};

  // Multi Sampling disabled
  auto multi_sampling_info = vk::PipelineMultisampleStateCreateInfo{
      .rasterizationSamples = vk::SampleCountFlagBits::e1,
      .sampleShadingEnable = vk::False,
  };

  // TODO: To be looked into
  vk::PipelineDepthStencilStateCreateInfo *depth_stencil_info = nullptr;

  auto pipeline_layout_create_info = vk::PipelineLayoutCreateInfo{};

  if (!info.push_constant_ranges.empty()) {
    pipeline_layout_create_info.pushConstantRangeCount =
        static_cast<uint32_t>(info.push_constant_ranges.size());
    pipeline_layout_create_info.pPushConstantRanges =
        info.push_constant_ranges.data();
  }

  if (descriptor_set_layout != nullptr) {
    pipeline_layout_create_info.setLayoutCount = 1;
    // Typical usage requries a pointer dereference and then a reference, so it
    // needs a double pointer deref here
    pipeline_layout_create_info.pSetLayouts = &**descriptor_set_layout;
  }

  auto graphics_pipeline_layout =
      vk::raii::PipelineLayout(device, pipeline_layout_create_info);

  auto pipeline_rendering_info = vk::PipelineRenderingCreateInfo{
      .colorAttachmentCount = 1,
      .pColorAttachmentFormats = &surface_format.format};

  auto pipeline_info_chain =
      vk::StructureChain<vk::GraphicsPipelineCreateInfo,
                         vk::PipelineRenderingCreateInfo>{
          vk::GraphicsPipelineCreateInfo{
              .stageCount = static_cast<uint32_t>(shader_stages.size()),
              .pStages = shader_stages.data(),
              .pVertexInputState = &vertex_input_info,
              .pInputAssemblyState = &input_assembly,
              .pViewportState = &viewport_info,
              .pRasterizationState = &rasterisation_info,
              .pMultisampleState = &multi_sampling_info,
              .pColorBlendState = &info.colour_blend_data,
              .pDynamicState = &dynamic_state_pipeline_info,
              .layout = graphics_pipeline_layout,
              .renderPass = nullptr},
          vk::PipelineRenderingCreateInfo{.colorAttachmentCount = 1,
                                          .pColorAttachmentFormats =
                                              &surface_format.format}};

  auto graphics_pipeline = vk::raii::Pipeline(
      device, nullptr,
      pipeline_info_chain.get<vk::GraphicsPipelineCreateInfo>());

  auto object = PipelineContainer(std::move(graphics_pipeline_layout),
                                  std::move(graphics_pipeline),
                                  info.screen_region, info.image_slice);

  return object;
}
