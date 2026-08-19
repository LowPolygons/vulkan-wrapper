#include "3d_app.hh"
#include "buffers/transition_buffer_layout.hh"
#include "device/helpers.hh"
#include "vulkan/vulkan.hpp"
#include <chrono>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/trigonometric.hpp>
#include <print>
#include <vulkan/vulkan_raii.hpp>

auto App3D::create(App3DCreateInfo info, VulkanRoot &root,
                   std::vector<ShaderVertex3D> vertices,
                   std::vector<uint16_t> indices)
    -> std::expected<App3D, std::string> {
  // TODO: move to function and wrapper class
  auto uniform_buffer_layout_binding = vk::DescriptorSetLayoutBinding{
      .binding = 0,
      .descriptorType = vk::DescriptorType::eUniformBuffer,
      .descriptorCount = 1,
      // To be able to be referenced anywhere you can use ::eAllGraphics
      .stageFlags = vk::ShaderStageFlagBits::eVertex};

  auto ubo_layout = vk::DescriptorSetLayoutCreateInfo{
      .bindingCount = 1, .pBindings = &uniform_buffer_layout_binding};

  auto descriptor_set_layout = vk::raii::DescriptorSetLayout(
      root.device_and_queue.logical(), ubo_layout);

  auto maybe_pipeline_container = GraphicsPipeline::PipelineContainer::create(
      info.pipeline_details, root.device_and_queue.logical(),
      root.swapchain_info.surface_format(), &descriptor_set_layout);

  if (!maybe_pipeline_container)
    return std::unexpected("3D App Pipeline Container Init Error: " +
                           maybe_pipeline_container.error());

  auto extracted_pipeline_container =
      std::move(maybe_pipeline_container.value());

  auto maybe_command_buffer =
      BufferUtils::CommandPoolAndBuffersContainer::create(
          BufferUtils::CommandBufferContainerCreateInfo{
              .num_frames_in_flight = info.max_frames_in_flight},
          root.device_and_queue.logical(), root.device_and_queue.queue_index());

  if (!maybe_command_buffer)
    return std::unexpected("3D App Command Buffer Init Error: " +
                           maybe_command_buffer.error());

  auto extracted_command_buffers = std::move(maybe_command_buffer.value());

  auto maybe_data_buffers =
      BufferUtils::DataBufferContainer<ShaderVertex3D, uint16_t>::create(
          BufferUtils::BufferContainerCreateInfo<ShaderVertex3D,
                                                 unsigned short>{
              .num_vertices = vertices.size(),
              .num_indices = indices.size(),
              .vertex_data = vertices,
              .index_data = indices},
          BufferUtils::DeviceBundleRefs{
              .physical_ref = root.device_and_queue.physical(),
              .logical_ref = root.device_and_queue.logical(),
              .queue_ref = root.device_and_queue.queue(),
              .command_pool = extracted_command_buffers.command_pool()});

  if (!maybe_data_buffers)
    return std::unexpected("3D App Data Buffers Init Error: " +
                           maybe_data_buffers.error());

  auto extraced_data_buffers = std::move(maybe_data_buffers.value());

  /*
   *
   * UNIFORM DATA BUFFERS
   *
   */
  std::vector<vk::raii::Buffer> uniform_buffers;
  std::vector<vk::raii::DeviceMemory> uniform_buffers_memory;
  std::vector<void *> uniform_buffers_mapped;

  for (auto i = 0; i < info.max_frames_in_flight; i++) {
    auto buffer_size = vk::DeviceSize{sizeof(App3DUniformBuffer)};
    auto maybe_buf_and_mem = DeviceUtil::create_buffer(
        root.device_and_queue.logical(), root.device_and_queue.physical(),
        buffer_size, vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent);

    if (!maybe_buf_and_mem)
      return std::unexpected("3D App Uniform Buffer Init Error: " +
                             maybe_buf_and_mem.error());
    auto [buffer, buffer_mem] = std::move(maybe_buf_and_mem.value());

    uniform_buffers.emplace_back(std::move(buffer));
    uniform_buffers_memory.emplace_back(std::move(buffer_mem));
    uniform_buffers_mapped.emplace_back(
        uniform_buffers_memory.back().mapMemory(0, buffer_size));
  }

  auto uniform_buffer_data = UniformDataBuffers{
      .uniform_buffers = std::move(uniform_buffers),
      .uniform_buffers_memory = std::move(uniform_buffers_memory),
      .uniform_buffers_mapped = std::move(uniform_buffers_mapped)};

  /*
   *
   * Descriptor Sets
   *
   * TODO: understand any of it
   *
   */

  auto descriptor_pool_size = vk::DescriptorPoolSize{
      .type = vk::DescriptorType::eUniformBuffer,
      .descriptorCount = static_cast<uint32_t>(info.max_frames_in_flight)};
  auto descriptor_pool_info = vk::DescriptorPoolCreateInfo{
      .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
      .maxSets = static_cast<uint32_t>(info.max_frames_in_flight),
      .poolSizeCount = 1,
      .pPoolSizes = &descriptor_pool_size};

  auto descriptor_pool = vk::raii::DescriptorPool(
      root.device_and_queue.logical(), descriptor_pool_info);

  auto layouts = std::vector<vk::DescriptorSetLayout>(info.max_frames_in_flight,
                                                      *descriptor_set_layout);

  auto allocation_info = vk::DescriptorSetAllocateInfo{
      .descriptorPool = descriptor_pool,
      .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
      .pSetLayouts = layouts.data()};

  auto descriptor_sets =
      root.device_and_queue.logical().allocateDescriptorSets(allocation_info);

  for (auto i = 0; i < info.max_frames_in_flight; i++) {
    auto buffer_info = vk::DescriptorBufferInfo{
        .buffer = uniform_buffer_data.uniform_buffers[i],
        .offset = 0,
        .range = sizeof(App3DUniformBuffer)};
    auto descriptor_write = vk::WriteDescriptorSet{
        .dstSet = descriptor_sets[i],
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .pBufferInfo = &buffer_info};

    root.device_and_queue.logical().updateDescriptorSets(descriptor_write, {});
  }
  //

  auto maybe_sync_objects = SyncObjects::SyncObjectsContainer::create(
      info.max_frames_in_flight, root.swapchain_info.images().size(),
      info.max_frames_in_flight, root.device_and_queue.logical());

  if (!maybe_sync_objects)
    return std::unexpected("3D App Sync Objects Init Error: " +
                           maybe_sync_objects.error());

  auto extracted_sync_objects = std::move(maybe_sync_objects.value());

  auto object = App3D(
      std::move(descriptor_set_layout), std::move(descriptor_pool),
      std::move(descriptor_sets), std::move(extracted_pipeline_container),
      std::move(extracted_command_buffers), std::move(extraced_data_buffers),
      std::move(uniform_buffer_data), std::move(extracted_sync_objects),
      info.max_frames_in_flight, info.default_colour);
  return std::move(object);
}

// NOTE: for smaller objects, push constants should be used instead
auto App3D::update_uniform_buffer(uint32_t current_image,
                                  vk::Extent2D &dimensions) -> void {
  static auto start_time = std::chrono::high_resolution_clock::now();

  auto current_time = std::chrono::high_resolution_clock::now();

  auto time_diff = std::chrono::duration<float, std::chrono::seconds::period>(
                       current_time - start_time)
                       .count();

  auto ubo = App3DUniformBuffer{};

  ubo.model = glm::rotate(glm::mat4(1.0f), time_diff * glm::radians(90.0f),
                          glm::vec3(0.0f, 0.0f, 1.0f));
  ubo.view =
      glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                  glm::vec3(0.0f, 0.0f, 1.0f));

  ubo.proj = glm::perspective(glm::radians(45.0f),
                              static_cast<float>(dimensions.width) /
                                  static_cast<float>(dimensions.height),
                              0.1f, 10.0f);
  ubo.proj[1][1] *= -1;

  memcpy(uniform_data.uniform_buffers_mapped[current_image], &ubo, sizeof(ubo));
}

auto App3D::get_current_state(
    std::shared_ptr<GLFWwindow> window, vk::raii::Device &logical_device,
    SwapchainInfo::SwapchainInfoContainer &swapchain_state)
    -> std::expected<std::optional<VulkanAppTickState>, std::string> {
  //=// Update Uniform Buffers

  update_uniform_buffer(current_frame_index, swapchain_state.dimensions());

  //=// Vulkan Jargon
  auto &fence_ref = sync_objects.fence(current_frame_index);

  auto fence_result =
      logical_device.waitForFences(*fence_ref, vk::True, UINT64_MAX);

  if (fence_result != vk::Result::eSuccess)
    return std::unexpected("Failed to wait for the draw fences");

  auto &present_complete_sem =
      sync_objects.present_complete_semaphore(current_frame_index);

  auto [result, image_index] = swapchain_state.swap_chain().acquireNextImage(
      UINT64_MAX, present_complete_sem, nullptr);

  if (result == vk::Result::eErrorOutOfDateKHR) {
    // Swap chain requires recreation
    return std::nullopt;
  }

  if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
    return std::unexpected("Failed to acquire next swap chain image");

  record_command_buffer(
      swapchain_state.images()[image_index],
      swapchain_state.image_views()[image_index],
      vk::Rect2D(vk::Offset2D(0, 0), swapchain_state.dimensions()),
      // TODO: viewport and scissor and render_area need to be stored
      vk::Viewport(
          0.0f, 0.0f, static_cast<float>(swapchain_state.dimensions().width),
          static_cast<float>(swapchain_state.dimensions().height), 0.0f, 1.0f),
      vk::Rect2D(vk::Offset2D(0, 0), swapchain_state.dimensions()));

  auto wait_destination_stage_mask =
      vk::PipelineStageFlags{vk::PipelineStageFlagBits::eColorAttachmentOutput};

  auto &render_finished_sem =
      sync_objects.render_finished_semaphore(image_index);

  auto submit_info = vk::SubmitInfo{
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &*present_complete_sem,
      .pWaitDstStageMask = &wait_destination_stage_mask,
      .commandBufferCount = 1,
      .pCommandBuffers =
          &*command_pool_and_buffers.get_buffer_ref(current_frame_index),
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &*render_finished_sem};

  auto state_info =
      VulkanAppTickState{.fence_ref = fence_ref,
                         .present_complete_sem_ref = present_complete_sem,
                         .render_finished_sem_ref = render_finished_sem,
                         .current_frame_index = current_frame_index,
                         .current_image_index = image_index,
                         .queue_submit_info = submit_info};

  current_frame_index = (current_frame_index + 1) % max_frames_in_flight;

  return state_info;
}

auto App3D::is_running() -> bool { return true; }

auto App3D::record_command_buffer(vk::Image &transition_image,
                                  vk::raii::ImageView &image_view,
                                  vk::Rect2D render_area, vk::Viewport viewport,
                                  vk::Rect2D scissor) -> void {
  //
  auto &command_buffer =
      command_pool_and_buffers.get_buffer_ref(current_frame_index);

  command_buffer.reset();
  command_buffer.begin({});

  BufferUtils::transition_image_layout_on_buffer(
      command_buffer, transition_image, vk::ImageLayout::eUndefined,
      vk::ImageLayout::eColorAttachmentOptimal, {},
      vk::AccessFlagBits2::eColorAttachmentWrite,
      vk::PipelineStageFlagBits2::eColorAttachmentOutput,
      vk::PipelineStageFlagBits2::eColorAttachmentOutput);

  auto attachment_info = vk::RenderingAttachmentInfo{
      .imageView = image_view,
      .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .clearValue = default_colour,
  };

  auto rendering_info =
      vk::RenderingInfo{.renderArea = render_area,
                        .layerCount = 1,
                        .colorAttachmentCount = 1,
                        .pColorAttachments = &attachment_info};

  command_buffer.beginRendering(rendering_info);
  command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
                              pipeline_data.pipeline());

  command_buffer.setViewport(0, viewport);
  command_buffer.setScissor(0, scissor);

  command_buffer.bindVertexBuffers(0, *data_buffers.vertices().buffer, {0});
  command_buffer.bindIndexBuffer(data_buffers.indices().buffer, 0,
                                 vk::IndexType::eUint16);

  command_buffer.bindDescriptorSets(
      vk::PipelineBindPoint::eGraphics, pipeline_data.layout(), 0,
      *descriptor_sets[current_frame_index], nullptr);

  command_buffer.drawIndexed(static_cast<uint32_t>(data_buffers.indices().size),
                             1, 0, 0, 0);
  BufferUtils::transition_image_layout_on_buffer(
      command_buffer, transition_image,
      vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR,
      vk::AccessFlagBits2::eColorAttachmentWrite, {},
      vk::PipelineStageFlagBits2::eColorAttachmentOutput,
      vk::PipelineStageFlagBits2::eBottomOfPipe);

  command_buffer.end();
}

auto create_3d_app(VulkanRoot &vulkan_root)
    -> std::expected<App3D, std::string> {
  auto app_colour_blend_data = vk::PipelineColorBlendAttachmentState{
      .blendEnable = vk::False,
      .colorWriteMask =
          vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
          vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};

  auto app_create_info = App3DCreateInfo{
      .pipeline_details =
          GraphicsPipeline::PipelineContainerCreateInfo{
              .vertex_shader_path = "shaders/3d_shader.spv",
              .frag_shader_path = "shaders/3d_shader.spv",
              .vertex_main_func_name = "vertMain",
              .frag_main_func_name = "fragMain",
              .binding_description = ShaderVertex3D::get_binding_descriptions(),
              .attribute_descriptions =
                  ShaderVertex3D::get_attribute_descriptions(),
              .screen_region = {0, 0, 800, 600, 0, 1},
              .image_slice = {vk::Offset2D(0, 0), {800, 600}},
              .polygon_mode = vk::PolygonMode::eFill,
              .cull_mode = vk::CullModeFlagBits::eBack,
              .front_face = vk::FrontFace::eCounterClockwise,
              .colour_blend_data =
                  vk::PipelineColorBlendStateCreateInfo{
                      .logicOpEnable = vk::False,
                      .logicOp = vk::LogicOp::eClear,
                      .attachmentCount = 1,
                      .pAttachments = &app_colour_blend_data}},
      .default_colour = vk::ClearColorValue(0.3, 0.3, 0.3, 1.0f),
      .max_frames_in_flight = 2};

  auto maybe_app = App3D::create(app_create_info, vulkan_root,
                                 {{{-0.5, -0.5}, {0.0, 0.0, 0.0}, 5.0},
                                  {{0.5, -0.5}, {1.0, 0.0, 0.0}, 5.0},
                                  {{0.5, 0.5}, {0.0, 1.0, 0.0}, 5.0},
                                  {{-0.5, 0.5}, {0.0, 0.0, 1.0}, 5.0}},
                                 {0, 1, 2, 2, 3, 0});

  return maybe_app;
}
