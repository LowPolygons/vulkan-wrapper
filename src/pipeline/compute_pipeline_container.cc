#include "compute_pipeline_container.hh"
#include "../shaders/shader_utils.hh"

auto ComputePipeline::ComputePipelineContainer::layout()
    -> vk::raii::PipelineLayout & {
  return _layout;
}

auto ComputePipeline::ComputePipelineContainer::pipeline()
    -> vk::raii::Pipeline & {
  return _pipeline;
}

auto ComputePipeline::ComputePipelineContainer::create(
    ComputePipeline::ComputePipelineCreateInfo info, vk::raii::Device &device)
    -> std::expected<ComputePipeline::ComputePipelineContainer, std::string> {
  if (device == nullptr)
    return std::unexpected(
        "Tried to create a compute pipeline before device was initialised");

  auto maybe_compute_shader_bytecode =
      ShaderUtils::read_shader(info.compute_shader_path);

  if (!maybe_compute_shader_bytecode)
    return std::unexpected("Compute Pipeline Init Error: " +
                           maybe_compute_shader_bytecode.error());

  auto extracted_compute_shader_module =
      vk::raii::ShaderModule{ShaderUtils::map_shader_bytes_to_shader_module(
          maybe_compute_shader_bytecode.value(), device)};

  auto compute_shader_stage_info = vk::PipelineShaderStageCreateInfo{
      .stage = vk::ShaderStageFlagBits::eCompute,
      .module = extracted_compute_shader_module,
      .pName = info.compute_main_func.c_str()};

  auto shader_stages = std::vector{compute_shader_stage_info};

  auto pipeline_layout_create_info = vk::PipelineLayoutCreateInfo{};

  if (!info.push_constant_ranges.empty()) {
    pipeline_layout_create_info.pushConstantRangeCount =
        static_cast<uint32_t>(info.push_constant_ranges.size());
    pipeline_layout_create_info.pPushConstantRanges =
        info.push_constant_ranges.data();
  }

  auto compute_pipeline_layout =
      vk::raii::PipelineLayout(device, pipeline_layout_create_info);

  auto pipeline_create_info = vk::ComputePipelineCreateInfo{
      .stage = compute_shader_stage_info,
      .layout = compute_pipeline_layout,
  };

  auto compute_pipeline = vk::raii::Pipeline{
      device.createComputePipeline(nullptr, pipeline_create_info)};

  auto object = ComputePipelineContainer(std::move(compute_pipeline_layout),
                                         std::move(compute_pipeline));

  return object;
}
