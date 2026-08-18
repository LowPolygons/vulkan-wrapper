#include "pipeline/graphics_pipeline_container.hh"
#include "vulkan/vulkan.hpp"
#include "vulkan_engine.hh"

#include <GLFW/glfw3.h>
#include <cstdlib>
#include <iostream>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <glm/glm.hpp>

auto PrintError(std::string msg) -> int {
  std::cerr << "Error in Execution: " << msg << std::endl;
  return EXIT_FAILURE;
}

struct ShaderVertex {
  glm::vec2 position;
  glm::vec3 colour;
  glm::f32 number;
  static vk::VertexInputBindingDescription get_binding_descriptions() {
    return {// Index of the binding in the arrya of bindings /shrug
            .binding = 0,
            // The number of bytes from one entry to the next
            .stride = sizeof(ShaderVertex),
            // eVertex moves to next data entry after each vertex, as opposed to
            // eInstance
            .inputRate = vk::VertexInputRate::eVertex};
  }
  static std::vector<vk::VertexInputAttributeDescription>
  get_attribute_descriptions() {
    return {{vk::VertexInputAttributeDescription{
                 .location = 0,
                 .binding = 0,
                 .format = vk::Format::eR32G32Sfloat,
                 .offset = offsetof(ShaderVertex, position)},
             vk::VertexInputAttributeDescription{
                 .location = 1,
                 .binding = 0,
                 .format = vk::Format::eR32G32B32Sfloat,
                 .offset = offsetof(ShaderVertex, colour)},
             vk::VertexInputAttributeDescription{
                 .location = 2,
                 .binding = 0,
                 .format = vk::Format::eR32Sfloat,
                 .offset = offsetof(ShaderVertex, number)}}};
  }
};

const std::vector<ShaderVertex> vertices = {
    {{-1.0, -1.0}, {0.0, 0.0, 0.0}, 5.0},
    {{0.0, -1.0}, {1.0, 0.0, 0.0}, 5.0},
    {{1.0, 1.0}, {0.0, 1.0, 0.0}, 5.0},
    {{-1.0, 1.0}, {0.0, 0.0, 1.0}, 5.0}};

const std::vector<uint16_t> indices = {0, 1, 2, 2, 3, 0};

auto main() -> int {
  auto vulkan_context = vk::raii::Context{};
  auto app_colour_blend_data = vk::PipelineColorBlendAttachmentState{
      .blendEnable = vk::False,
      .colorWriteMask =
          vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
          vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};

  // TODO: there is a lot of data here that isnt actually being utilised
  auto app_data = VulkanAppMetadata{
      .width = 800,
      .height = 600,
      .app_name = "Sin-Animated Mandelbulb",
      .present_mode = vk::PresentModeKHR::eFifo,
      .max_frames_in_flight = 2,
      .extensions = {vk::KHRSwapchainExtensionName},
      .layers = {"VK_LAYER_KHRONOS_validation"},
      .gpu_type = DeviceType::DISCRETE,
      .pipeline_info =
          PipelineContainerCreateInfo{
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
      .clear_colour = vk::ClearColorValue(1.0f, 0.0f, 0.0f, 1.0f)};

  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // no opengl
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  auto glfw_window =
      glfwCreateWindow(app_data.width, app_data.height,
                       app_data.app_name.c_str(), nullptr, nullptr);

  auto maybe_vulkan_wrapper =
      Implementation::create_vulkan_wrapper<ShaderVertex, uint16_t,
                                            ImageProperties>(
          glfw_window, app_data,
          vk::ApplicationInfo{.pApplicationName = app_data.app_name.c_str(),
                              .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
                              .apiVersion = vk::ApiVersion13},
          vertices, indices);

  if (!maybe_vulkan_wrapper)
    return PrintError("Fail On Init: " + maybe_vulkan_wrapper.error());

  auto vulkan_wrapper = std::move(maybe_vulkan_wrapper.value());

  auto power = float{0};

  while (!glfwWindowShouldClose(glfw_window)) {
    power = power + (1.0 / 300.0) * (3.14159265 / 2.0);

    ImageProperties push_consts{
        .win_x = static_cast<float>(
            vulkan_wrapper.swapchain_container.dimensions().width),
        .win_y = static_cast<float>(
            vulkan_wrapper.swapchain_container.dimensions().height),
        .power = 10 * std::sin(power)};

    glfwPollEvents();

    auto status = vulkan_wrapper.draw_frame(push_consts);

    if (!status)
      return PrintError("Draw frame function failed: " + status.error());
  }

  glfwDestroyWindow(glfw_window);
  glfwTerminate();
}
