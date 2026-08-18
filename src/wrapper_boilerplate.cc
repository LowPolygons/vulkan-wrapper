#include "wrapper_boilerplate.hh"
#include "instance_and_surface/instance_and_surface_container.hh"
#include "swapchain/swapchain_info_container.hh"
#include "vulkan/vulkan.hpp"
#include <GLFW/glfw3.h>
#include <print>
#include <vulkan/vulkan_raii.hpp>

auto VulkanRoot::end_glfw_instance() -> void {
  glfwDestroyWindow(window_container.shared_get().get());
  glfwTerminate();
};

auto VulkanRoot::print_state() -> void { std::println("To be completed!"); }

auto VulkanRoot::run_app(VulkanAppInterface &app)
    -> std::expected<void, std::string> {
  while (app.is_running() and
         !glfwWindowShouldClose(window_container.shared_get().get())) {
    glfwPollEvents();

    auto maybe_current_run_state =
        app.get_current_state(window_container.shared_get(),
                              device_and_queue.logical(), swapchain_info);

    if (!maybe_current_run_state)
      return std::unexpected("App Failed to get the current state: " +
                             maybe_current_run_state.error());

    auto current_run_state = maybe_current_run_state.value();

    if (current_run_state == std::nullopt) {
      if (!recreate_swap_chain())
        return std::unexpected("Failed to recreate swap chain");
    } else {
      auto status = run_frame(current_run_state.value());
      if (!status)
        return std::unexpected("Run Frame Failed: " + status.error());
    }
  }

  return {};
}

auto VulkanRoot::run_frame(VulkanAppTickState &state)
    -> std::expected<void, std::string> {
  // INFO: fences already waited for
  // INFO: Command Buffer already recorded
  // INFO: Submit Info already recorded

  device_and_queue.queue().submit(state.queue_submit_info, *state.fence_ref);

  // INFO: App is expected to track the current frame index

  auto present_info =
      vk::PresentInfoKHR{.waitSemaphoreCount = 1,
                         .pWaitSemaphores = &*state.render_finished_sem_ref,
                         .swapchainCount = 1,
                         .pSwapchains = &*swapchain_info.swap_chain(),
                         .pImageIndices = &state.current_image_index};

  auto result = device_and_queue.queue().presentKHR(present_info);

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

auto VulkanRoot::create(VulkanRootCreateinfo info)
    -> std::expected<VulkanRoot, std::string> {
  auto glfw_window_container = GlfwWindowContainer(
      {info.width, info.height}, info.window_name, info.window_resizable);

  auto vulkan_context = vk::raii::Context{};

  auto maybe_instance_and_surface =
      InstanceAndSurface::VulkanInstanceAndSurface::create(
          InstanceAndSurface::VulkanInstanceAndSurfaceCreateInfo{
              .app_info = info.application_info,
              .validation_layers = info.layers,
              .additional_extensions = info.instance_extensions,
          },
          vulkan_context, glfw_window_container.shared_get().get());

  if (!maybe_instance_and_surface)
    return std::unexpected("InstanceAndSurface Init Error: " +
                           maybe_instance_and_surface.error());
  auto extracted_instance_and_surface =
      std::move(maybe_instance_and_surface.value());

  auto maybe_device_and_queue_container =
      DeviceUtil::DeviceAndQueueContainer::create(
          DeviceUtil::DeviceCreateInfo{.queue_flags = info.gpu_type,
                                       .required_device_extensions =
                                           info.device_extensions},
          extracted_instance_and_surface.instance(),
          extracted_instance_and_surface.surface());

  if (!maybe_device_and_queue_container)
    return std::unexpected("DeviceAndQueueContainer Init error: " +
                           maybe_device_and_queue_container.error());

  auto extracted_device_and_queue_container =
      std::move(maybe_device_and_queue_container.value());

  auto maybe_swap_chain_info_container =
      SwapchainInfo::SwapchainInfoContainer::create(
          SwapchainInfo::SwapchainInfoContainerCreateInfo{
              .present_mode = info.present_mode},
          SwapchainInfo::SwapchainInfoObjectRefs{
              .instance_ref = extracted_instance_and_surface.instance(),
              .physical_device_ref =
                  extracted_device_and_queue_container.physical(),
              .device_ref = extracted_device_and_queue_container.logical(),
              .graphics_queue_ref =
                  extracted_device_and_queue_container.queue(),
              .surface_ref = extracted_instance_and_surface.surface(),
              .window = glfw_window_container.shared_get().get(),
          });

  if (!maybe_swap_chain_info_container)
    return std::unexpected("SwapchainInfoContainer Init Error: " +
                           maybe_swap_chain_info_container.error());

  auto extracted_swap_chain_info =
      std::move(maybe_swap_chain_info_container.value());

  auto object = VulkanRoot(info.present_mode, std::move(glfw_window_container),
                           std::move(extracted_instance_and_surface),
                           std::move(extracted_device_and_queue_container),
                           std::move(extracted_swap_chain_info));

  return object;
}

auto VulkanRoot::recreate_swap_chain() -> std::expected<void, std::string> {
  device_and_queue.logical().waitIdle();

  swapchain_info.wipe();

  auto maybe_swap_chain_info_container =
      SwapchainInfo::SwapchainInfoContainer::create(
          SwapchainInfo::SwapchainInfoContainerCreateInfo{.present_mode =
                                                              present_mode},
          SwapchainInfo::SwapchainInfoObjectRefs{
              .instance_ref = instance_and_surface.instance(),
              .physical_device_ref = device_and_queue.physical(),
              .device_ref = device_and_queue.logical(),
              .graphics_queue_ref = device_and_queue.queue(),
              .surface_ref = instance_and_surface.surface(),
              .window = window_container.shared_get().get()});

  if (!maybe_swap_chain_info_container)
    return std::unexpected(maybe_swap_chain_info_container.error());

  swapchain_info = std::move(maybe_swap_chain_info_container.value());

  return {};
}
