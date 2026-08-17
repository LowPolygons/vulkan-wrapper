#ifndef VULKAN_WRAPPER_VULKAN_ENGINE_HH
#define VULKAN_WRAPPER_VULKAN_ENGINE_HH

#include "buffers/transition_buffer_layout.hh"
#include "glfw/glfw_window_handler.hh"
#include <GLFW/glfw3.h>
#include <cstdint>
#include <glm/fwd.hpp>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "buffers/command_buffer_container.hh"
#include "buffers/data_buffer_container.hh"
#include "device/device_and_queue_container.hh"
#include "glfw/glfw_window_handler.hh"
#include "instance_and_surface/instance_and_surface_container.hh"
#include "pipeline/graphics_pipeline_container.hh"
#include "swapchain/swapchain_info_container.hh"
#include "syncs/sync_object_container.hh"

using namespace BufferUtils;
using namespace DeviceUtil;
using namespace GraphicsPipeline;
using namespace InstanceAndSurface;
using namespace SwapchainInfo;
using namespace SyncObjects;

struct PersistantStorageContainer {
  glm::f32 power;
};

struct ImageProperties {
  glm::f32 win_x;
  glm::f32 win_y;
  glm::f32 power;
};

struct VulkanAppMetadata {
  uint32_t width;
  uint32_t height;

  std::string app_name;

  vk::PresentModeKHR present_mode;

  std::size_t max_frames_in_flight;

  std::vector<const char *> extensions;
  std::vector<const char *> layers;

  DeviceType gpu_type;

  PipelineContainerCreateInfo pipeline_info;

  vk::ClearValue clear_colour;
};

template <typename VBT, typename IT, typename FPC> struct VulkanWrapper {
  std::size_t max_frames_in_flight;
  vk::ClearValue background_colour;
  vk::PresentModeKHR present_mode;

  GLFWwindow *window_container;
  VulkanInstanceAndSurface instance_and_surface;
  DeviceAndQueueContainer device_and_queue;
  SwapchainInfoContainer swapchain_container;
  PipelineContainer pipeline_container;
  CommandPoolAndBuffersContainer command_pool_and_buffers;
  DataBufferContainer<VBT, IT> data_buffer_container;
  SyncObjectsContainer sync_objects;

  uint32_t current_frame_index = 0;

  auto draw_frame(FPC push_constants) -> std::expected<void, std::string>;
  auto recreate_swap_chain() -> std::expected<void, std::string>;

private:
  auto record_command_buffer(uint32_t image_index, FPC &push_constants) -> void;
};

namespace Implementation {

template <typename VBT, typename IT, typename FPC>
auto create_vulkan_wrapper(GLFWwindow *glfw_window, VulkanAppMetadata app_data,
                           vk::ApplicationInfo vulkan_app_info,
                           std::vector<VBT> vertices, std::vector<IT> indices)
    -> std::expected<VulkanWrapper<VBT, IT, FPC>, std::string> {

  auto vulkan_context = vk::raii::Context{};

  auto maybe_instance_and_surface = VulkanInstanceAndSurface::create(
      VulkanInstanceAndSurfaceCreateInfo{
          .app_info = vulkan_app_info,
          .validation_layers = app_data.layers,
          .additional_extensions = app_data.extensions,
      },
      vulkan_context, glfw_window);

  if (!maybe_instance_and_surface)
    return std::unexpected(maybe_instance_and_surface.error());

  auto vulkan_wrapper_instance_and_surface =
      std::move(maybe_instance_and_surface.value());

  auto maybe_device_and_queue_container = DeviceAndQueueContainer::create(
      DeviceCreateInfo{
          .queue_flags = app_data.gpu_type,
          .required_device_extensions = app_data.extensions,
      },
      vulkan_wrapper_instance_and_surface.instance(),
      vulkan_wrapper_instance_and_surface.surface());

  if (!maybe_device_and_queue_container)
    return std::unexpected(maybe_device_and_queue_container.error());

  auto vulkan_wrapper_device_and_queue =
      std::move(maybe_device_and_queue_container.value());

  auto maybe_swap_chain_info_container = SwapchainInfoContainer::create(
      SwapchainInfoContainerCreateInfo{.present_mode = app_data.present_mode},
      SwapchainInfoObjectRefs{
          .instance_ref = vulkan_wrapper_instance_and_surface.instance(),
          .physical_device_ref = vulkan_wrapper_device_and_queue.physical(),
          .device_ref = vulkan_wrapper_device_and_queue.logical(),
          .graphics_queue_ref = vulkan_wrapper_device_and_queue.queue(),
          .surface_ref = vulkan_wrapper_instance_and_surface.surface(),
          .window = glfw_window});

  if (!maybe_swap_chain_info_container)
    return std::unexpected(maybe_swap_chain_info_container.error());

  auto vulkan_wrapper_swapchain_container =
      std::move(maybe_swap_chain_info_container.value());

  auto maybe_pipeline_container = PipelineContainer::create(
      app_data.pipeline_info, vulkan_wrapper_device_and_queue.logical(),
      vulkan_wrapper_swapchain_container.surface_format());

  if (!maybe_pipeline_container)
    return std::unexpected(maybe_pipeline_container.error());

  auto vulkan_wrapper_pipeline_container =
      std::move(maybe_pipeline_container.value());

  auto maybe_command_buffer_container = CommandPoolAndBuffersContainer::create(
      CommandBufferContainerCreateInfo{.num_frames_in_flight =
                                           app_data.max_frames_in_flight},
      vulkan_wrapper_device_and_queue.logical(),
      vulkan_wrapper_device_and_queue.queue_index());

  if (!maybe_command_buffer_container)
    return std::unexpected(maybe_command_buffer_container.error());

  auto vulkan_wrapper_command_pool_and_buffers =
      std::move(maybe_command_buffer_container.value());

  auto maybe_data_buffer_container = DataBufferContainer<VBT, IT>::create(
      BufferUtils::BufferContainerCreateInfo<VBT, IT>{
          .num_vertices = vertices.size(),
          .num_indices = indices.size(),
          .vertex_data = vertices,
          .index_data = indices},
      BufferUtils::DeviceBundleRefs{
          .physical_ref = vulkan_wrapper_device_and_queue.physical(),
          .logical_ref = vulkan_wrapper_device_and_queue.logical(),
          .queue_ref = vulkan_wrapper_device_and_queue.queue(),
          .command_pool =
              vulkan_wrapper_command_pool_and_buffers.command_pool()});

  if (!maybe_data_buffer_container)
    return std::unexpected(maybe_data_buffer_container.error());

  auto vulkan_wrapper_data_buffer_container =
      std::move(maybe_data_buffer_container.value());

  auto maybe_sync_objects_container = SyncObjectsContainer::create(
      app_data.max_frames_in_flight,
      vulkan_wrapper_swapchain_container.images().size(),
      app_data.max_frames_in_flight, vulkan_wrapper_device_and_queue.logical());

  if (!maybe_sync_objects_container)
    return std::unexpected(maybe_sync_objects_container.error());

  auto vulkan_wrapper_sync_objects =
      std::move(maybe_sync_objects_container.value());

  auto vulkan_wrapper = VulkanWrapper<VBT, IT, FPC>{
      .max_frames_in_flight = app_data.max_frames_in_flight,
      .background_colour = app_data.clear_colour,
      .present_mode = app_data.present_mode,
      .window_container = glfw_window,
      .instance_and_surface = std::move(vulkan_wrapper_instance_and_surface),
      .device_and_queue = std::move(vulkan_wrapper_device_and_queue),
      .swapchain_container = std::move(vulkan_wrapper_swapchain_container),
      .pipeline_container = std::move(vulkan_wrapper_pipeline_container),
      .command_pool_and_buffers =
          std::move(vulkan_wrapper_command_pool_and_buffers),
      .data_buffer_container = std::move(vulkan_wrapper_data_buffer_container),
      .sync_objects = std::move(vulkan_wrapper_sync_objects),
      .current_frame_index = 0};

  return std::move(vulkan_wrapper);
}

} // namespace Implementation

template <typename VBT, typename IT, typename FPC>
auto VulkanWrapper<VBT, IT, FPC>::draw_frame(FPC push_constant)
    -> std::expected<void, std::string> {
  auto fence_result = device_and_queue.logical().waitForFences(
      *sync_objects.fence(current_frame_index), vk::True, UINT64_MAX);

  if (fence_result != vk::Result::eSuccess)
    return std::unexpected("Failed to wait for the draw fence");

  auto [result, image_index] =
      swapchain_container.swap_chain().acquireNextImage(
          UINT64_MAX,
          sync_objects.present_complete_semaphore(current_frame_index),
          nullptr);

  if (result == vk::Result::eErrorOutOfDateKHR) {
    if (!this->recreate_swap_chain())
      return std::unexpected("Failed to recreate swap chain");
    return {};
  }

  if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
    return std::unexpected("Failed to acquire next swap chain");

  device_and_queue.logical().resetFences(
      *sync_objects.fence(current_frame_index));

  this->record_command_buffer(image_index, push_constant);

  auto wait_destination_stage_mask =
      vk::PipelineStageFlags{vk::PipelineStageFlagBits::eColorAttachmentOutput};

  auto submit_info = vk::SubmitInfo{
      .waitSemaphoreCount = 1,
      .pWaitSemaphores =
          &*sync_objects.present_complete_semaphore(current_frame_index),
      .pWaitDstStageMask = &wait_destination_stage_mask,
      .commandBufferCount = 1,
      .pCommandBuffers =
          &*command_pool_and_buffers.get_buffer_ref(current_frame_index),
      .signalSemaphoreCount = 1,
      .pSignalSemaphores =
          &*sync_objects.render_finished_semaphore(image_index)};

  device_and_queue.queue().submit(submit_info,
                                  *sync_objects.fence(current_frame_index));

  current_frame_index = (current_frame_index + 1) % max_frames_in_flight;

  auto present_info = vk::PresentInfoKHR{
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &*sync_objects.render_finished_semaphore(image_index),
      .swapchainCount = 1,
      .pSwapchains = &*swapchain_container.swap_chain(),
      .pImageIndices = &image_index};

  result = device_and_queue.queue().presentKHR(present_info);

  if ((result == vk::Result::eSuboptimalKHR) ||
      (result == vk::Result::eErrorOutOfDateKHR)) {
    if (!this->recreate_swap_chain())
      return std::unexpected("Failed to recreate swap chain");
  } else {
    if (result != vk::Result::eSuccess)
      return std::unexpected("Queue Present KHR Failed");
  }

  return {};
}

template <typename VBT, typename IT, typename FPC>
auto VulkanWrapper<VBT, IT, FPC>::recreate_swap_chain()
    -> std::expected<void, std::string> {
  device_and_queue.logical().waitIdle();

  swapchain_container.wipe();

  auto maybe_swap_chain_info_container = SwapchainInfoContainer::create(
      SwapchainInfoContainerCreateInfo{.present_mode = present_mode},
      SwapchainInfoObjectRefs{.instance_ref = instance_and_surface.instance(),
                              .physical_device_ref =
                                  device_and_queue.physical(),
                              .device_ref = device_and_queue.logical(),
                              .graphics_queue_ref = device_and_queue.queue(),
                              .surface_ref = instance_and_surface.surface(),
                              .window = window_container});

  if (!maybe_swap_chain_info_container)
    return std::unexpected(maybe_swap_chain_info_container.error());

  swapchain_container = std::move(maybe_swap_chain_info_container.value());

  return {};
}

template <typename VBT, typename IT, typename FPC>
auto VulkanWrapper<VBT, IT, FPC>::record_command_buffer(uint32_t image_index,
                                                        FPC &push_constants)
    -> void {
  auto &command_buffer =
      command_pool_and_buffers.get_buffer_ref(current_frame_index);

  command_buffer.reset();
  command_buffer.begin({});

  BufferUtils::transition_image_layout_on_buffer(
      command_buffer, swapchain_container.images()[image_index],
      vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, {},
      vk::AccessFlagBits2::eColorAttachmentWrite,
      vk::PipelineStageFlagBits2::eColorAttachmentOutput,
      vk::PipelineStageFlagBits2::eColorAttachmentOutput);

  auto attachment_info = vk::RenderingAttachmentInfo{
      .imageView = swapchain_container.image_views()[image_index],
      .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .clearValue = background_colour};

  auto rendering_info = vk::RenderingInfo{
      .renderArea = {.offset = {0, 0},
                     .extent = swapchain_container.dimensions()},
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &attachment_info};

  command_buffer.beginRendering(rendering_info);

  command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
                              pipeline_container.pipeline());
  command_buffer.pushConstants(pipeline_container.layout(),
                               vk::ShaderStageFlagBits::eFragment, 0,
                               sizeof(FPC), &push_constants);
  command_buffer.setViewport(
      0,
      vk::Viewport(0.0f, 0.0f,
                   static_cast<float>(swapchain_container.dimensions().width),
                   static_cast<float>(swapchain_container.dimensions().height),
                   0.0f, 1.0f));
  command_buffer.setScissor(
      0, vk::Rect2D(vk::Offset2D(0, 0), swapchain_container.dimensions()));

  command_buffer.bindVertexBuffers(0, *data_buffer_container.vertices().buffer,
                                   {0});
  command_buffer.bindIndexBuffer(data_buffer_container.indices().buffer, 0,
                                 vk::IndexType::eUint16);

  command_buffer.drawIndexed(
      static_cast<uint32_t>(data_buffer_container.indices().size), 1, 0, 0, 0);

  BufferUtils::transition_image_layout_on_buffer(
      command_buffer, swapchain_container.images()[image_index],
      vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR,
      vk::AccessFlagBits2::eColorAttachmentWrite, {},
      vk::PipelineStageFlagBits2::eColorAttachmentOutput,
      vk::PipelineStageFlagBits2::eBottomOfPipe);

  command_buffer.end();
}
#endif
