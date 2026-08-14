#ifndef VULKAN_WRAPPER_VULKAN_ENGINE_HH
#define VULKAN_WRAPPER_VULKAN_ENGINE_HH

#include "glfw/glfw_window_handler.hh"
#include <GLFW/glfw3.h>
#include <cstdint>
#include <string>
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

struct VulkanAppMetadata {
  uint32_t width;
  uint32_t height;

  std::string app_name;

  std::size_t max_frames_in_flight;

  std::vector<const char *> extensions;
  std::vector<const char *> layers;

  DeviceType gpu_type;

  PipelineContainerCreateInfo pipeline_info;

  vk::ClearValue clear_colour;
};

template <typename VBT, typename IT> struct VulkanWrapper {
  std::size_t max_frames_in_flight;
  vk::ClearValue background_colour;

  GlfwWindowContainer window_container;
  VulkanInstanceAndSurface instance_and_surface;
  DeviceAndQueueContainer device_and_queue;
  SwapchainInfoContainer swapchain_container;
  PipelineContainer pipeline_container;
  CommandPoolAndBuffersContainer command_pool_and_buffers;
  DataBufferContainer<VBT, IT> data_buffer_container;
  SyncObjectsContainer sync_objects;

  uint32_t current_frame_index = 0;

  auto run() -> std::expected<void, std::string>;
  auto draw_frame() -> std::expected<void, std::string>;
  auto record_command_buffer(uint32_t image_index)
      -> std::expected<void, std::string>;
};

namespace Implementation {

// TODO: I want a 'PersistantStorageContainer' and a 'PushConstant' class
// somehow passed in Question is, how does this work for potentially multiple
// push constant ranges?
auto record_command_buffer(vk::raii::CommandBuffer &buffer)
    -> std::expected<void, std::string>;

template <typename VBT, typename IT>
auto create_vulkan_wrapper(VulkanAppMetadata app_data,
                           vk::ApplicationInfo vulkan_app_info)
    -> std::expected<VulkanWrapper<VBT, IT>, std::string> {
  auto vulkan_wrapper = VulkanWrapper<VBT, IT>{};

  vulkan_wrapper.max_frames_in_flight = app_data.max_frames_in_flight;
  vulkan_wrapper.background_colour = app_data.clear_colour;

  auto vulkan_context = vk::raii::Context{};

  auto window_container =
      GlfwWindowContainer({app_data.width, app_data.height}, app_data.app_name);

  auto maybe_instance_and_surface = VulkanInstanceAndSurface::create(
      VulkanInstanceAndSurfaceCreateInfo{
          .app_info = vulkan_app_info,
          .validation_layers = app_data.layers,
          .additional_extensions = app_data.extensions,
      },
      vulkan_context, window_container.shared_get());

  if (!maybe_instance_and_surface)
    return std::unexpected(maybe_instance_and_surface.error());

  vulkan_wrapper.instance_and_surface = maybe_instance_and_surface.value();

  auto maybe_device_and_queue_container = DeviceAndQueueContainer::create(
      DeviceCreateInfo{
          .queue_flags = app_data.gpu_type,
          .required_device_extensions = app_data.extensions,
      },
      vulkan_wrapper.instance_and_surface.instance(),
      vulkan_wrapper.instance_and_surface.surface());

  if (!maybe_device_and_queue_container)
    return std::unexpected(maybe_device_and_queue_container.error());

  vulkan_wrapper.device_and_queue = maybe_device_and_queue_container.value();

  auto maybe_swap_chain_info_container = SwapchainInfoContainer::create(
      SwapchainInfoContainerCreateInfo{.present_mode =
                                           vk::PresentModeKHR::eFifo},
      SwapchainInfoObjectRefs{
          .device_ref = vulkan_wrapper.device_and_queue.logical(),
          .physical_device_ref = vulkan_wrapper.device_and_queue.physical(),
          .graphics_queue_ref = vulkan_wrapper.device_and_queue.queue(),
          .instance_ref = vulkan_wrapper.instance_and_surface.instance(),
          .surface_ref = vulkan_wrapper.instance_and_surface.surface(),
          .weak_window = vulkan_wrapper.window_container.get()});

  if (!maybe_swap_chain_info_container)
    return std::unexpected(maybe_swap_chain_info_container.error());

  vulkan_wrapper.swapchain_container = maybe_swap_chain_info_container.value();

  auto maybe_pipeline_container = PipelineContainer::create(
      app_data.pipeline_info, vulkan_wrapper.device_and_queue.logical(),
      vulkan_wrapper.swapchain_container.surface_format());

  if (!maybe_pipeline_container)
    return std::unexpected(maybe_pipeline_container.error());

  vulkan_wrapper.pipeline_container = maybe_pipeline_container.value();

  auto maybe_command_buffer_container = CommandPoolAndBuffersContainer::create(
      CommandBufferContainerCreateInfo{.num_frames_in_flight =
                                           vulkan_wrapper.max_frames_in_flight},
      vulkan_wrapper.device_and_queue.logical(),
      vulkan_wrapper.device_and_queue.queue_index());

  if (!maybe_command_buffer_container)
    return std::unexpected(maybe_command_buffer_container.error());

  vulkan_wrapper.command_pool_and_buffers =
      maybe_command_buffer_container.value();

  auto maybe_data_buffer_container = DataBufferContainer<VBT, IT>::create(
      BufferUtils::BufferContainerCreateInfo<VBT, IT>{},
      BufferUtils::DeviceBundleRefs{
          .command_pool =
              vulkan_wrapper.command_pool_and_buffers.command_pool(),
          .logical_ref = vulkan_wrapper.device_and_queue.logical(),
          .physical_ref = vulkan_wrapper.device_and_queue.physical(),
          .queue_ref = vulkan_wrapper.device_and_queue.queue()});

  if (!maybe_data_buffer_container)
    return std::unexpected(maybe_data_buffer_container.error());

  vulkan_wrapper.data_buffer_container = maybe_data_buffer_container.value();

  auto maybe_sync_objects_container = SyncObjectsContainer::create(
      vulkan_wrapper.max_frames_in_flight,
      vulkan_wrapper.swapchain_container.images().size(),
      vulkan_wrapper.max_frames_in_flight,
      vulkan_wrapper.device_and_queue.logical());

  if (!maybe_sync_objects_container)
    return std::unexpected(maybe_sync_objects_container.error());

  vulkan_wrapper.sync_objects = maybe_sync_objects_container.value();
}

} // namespace Implementation

template <typename VBT, typename IT>
auto VulkanWrapper<VBT, IT>::run() -> std::expected<void, std::string> {
  if (auto window = window_container.shared_get().get()) {
    while (!glfwWindowShouldClose(window)) {
      glfwPollEvents();

      auto status = this->draw_frame();
      if (!status)
        return std::unexpected("Draw frame function failed: " + status.error());
    }
  } else {
    return std::unexpected("Tried to utilise window after GLFWWindow deletion");
  }
  return {};
}

template <typename VBT, typename IT>
auto VulkanWrapper<VBT, IT>::draw_frame() -> std::expected<void, std::string> {
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
    // TODO: Recreate swap chain
    return {};
  }

  if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
    return std::unexpected("Failed to acquire next swap chain");

  device_and_queue.logical().resetFences(
      *sync_objects.fence(current_frame_index));

  // TODO: record command buffer function

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
    // TODO Recreate swap chain
  } else {
    if (result != vk::Result::eSuccess)
      return std::unexpected("Queue Present KHR Failed");
  }

  return {};
}

#endif
