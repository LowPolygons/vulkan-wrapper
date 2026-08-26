#ifndef VULKAN_WRAPPER_COMPUTE_PIPELINE_CONTAINER_HH
#define VULKAN_WRAPPER_COMPUTE_PIPELINE_CONTAINER_HH

#include <expected>
#include <vulkan/vulkan_raii.hpp>
namespace ComputePipeline {

struct ComputePipelineCreateInfo {
  std::string compute_shader_path;
  std::string compute_main_func;

  std::vector<vk::PushConstantRange> push_constant_ranges;
};

class ComputePipelineContainer {
public:
  ComputePipelineContainer() = delete;

  static auto create(ComputePipeline::ComputePipelineCreateInfo info,
                     vk::raii::Device &device)
      -> std::expected<ComputePipelineContainer, std::string>;

  auto layout() -> vk::raii::PipelineLayout &;
  auto pipeline() -> vk::raii::Pipeline &;

private:
  ComputePipelineContainer(vk::raii::PipelineLayout &&layout,
                           vk::raii::Pipeline &&pipeline)
      : _layout(std::move(layout)), _pipeline(std::move(pipeline)) {}

private:
  vk::raii::PipelineLayout _layout;
  vk::raii::Pipeline _pipeline;
};

}; // namespace ComputePipeline

#endif
