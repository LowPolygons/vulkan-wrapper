
#include "mandelbulb_app.hh"
#include "vulkan/vulkan.hpp"
#include "vulkan_engine.hh"
#include "wrapper_boilerplate.hh"

#include <GLFW/glfw3.h>
#include <cstdlib>
#include <iostream>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <glm/glm.hpp>

auto main() -> int {
  auto vulkan_app_data = VulkanRootCreateinfo{
      .width = 800,
      .height = 600,
      .window_resizable = true,
      .window_name = "Morphing Mandelbulb",
      .application_info =
          vk::ApplicationInfo{.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
                              .apiVersion = vk::ApiVersion13},
      .layers = {"VK_LAYER_KHRONOS_validation"},
      .instance_extensions = {},
      .gpu_type = DeviceUtil::DeviceType::DISCRETE,
      .device_extensions = {vk::KHRSwapchainExtensionName},
      .present_mode = vk::PresentModeKHR::eFifo};

  auto maybe_vulkan_root = VulkanRoot::create(vulkan_app_data);

  if (!maybe_vulkan_root) {
    std::cerr << maybe_vulkan_root.error() << std::endl;
    return EXIT_FAILURE;
  }

  auto vulkan_root = std::move(maybe_vulkan_root.value());

  auto app_colour_blend_data = vk::PipelineColorBlendAttachmentState{
      .blendEnable = vk::False,
      .colorWriteMask =
          vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
          vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};
  auto app_create_info = MandelbulbAppCreateInfo{
      .pipeline_details =
          GraphicsPipeline::PipelineContainerCreateInfo{
              .vertex_shader_path = "shaders/slang.spv",
              .frag_shader_path = "shaders/slang.spv",
              .vertex_main_func_name = "vertMain",
              .frag_main_func_name = "fragMain",
              .binding_description = ShaderVertex::get_binding_descriptions(),
              .attribute_descriptions =
                  ShaderVertex::get_attribute_descriptions(),
              .screen_region = {0, 0, 800, 600, 0, 1},
              .image_slice = {vk::Offset2D(0, 0), {800, 600}},
              .polygon_mode = vk::PolygonMode::eFill,
              .cull_mode = vk::CullModeFlagBits::eBack,
              .colour_blend_data =
                  vk::PipelineColorBlendStateCreateInfo{
                      .logicOpEnable = vk::False,
                      .logicOp = vk::LogicOp::eCopy,
                      .attachmentCount = 1,
                      .pAttachments = &app_colour_blend_data},
              .push_constant_ranges = {vk::PushConstantRange{
                  .stageFlags = vk::ShaderStageFlagBits::eFragment,
                  .offset = 0,
                  .size = sizeof(ImageProperties)}}},
      .default_colour = vk::ClearColorValue(0.0, 0.0, 0.0, 1.0f),
      .max_frames_in_flight = 2};

  auto maybe_app = MandelbulbApp::create(app_create_info, vulkan_root,
                                         {{{-1.0, -1.0}, {0.0, 0.0, 0.0}, 5.0},
                                          {{1.0, -1.0}, {1.0, 0.0, 0.0}, 5.0},
                                          {{1.0, 1.0}, {0.0, 1.0, 0.0}, 5.0},
                                          {{-1.0, 1.0}, {0.0, 0.0, 1.0}, 5.0}},
                                         {0, 1, 2, 2, 3, 0});

  if (!maybe_app) {
    std::cerr << maybe_app.error() << std::endl;
    return EXIT_FAILURE;
  }

  auto mandelbulb_app = std::move(maybe_app.value());

  auto status = vulkan_root.run_app(mandelbulb_app);

  vulkan_root.end_glfw_instance();

  if (!status) {
    std::cerr << status.error() << std::endl;
    return EXIT_FAILURE;
  }
}
