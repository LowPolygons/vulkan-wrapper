#ifndef VUKLKAN_WRAPPER_IMPLEMENTATION_HH
#define VUKLKAN_WRAPPER_IMPLEMENTATION_HH

#include "vulkan/vulkan.hpp"
#include "vulkan_wrapper/debugger/debugger_container.hh"
#include "vulkan_wrapper/device/device_and_queue_container.hh"
#include "vulkan_wrapper/glfw/glfw_window_handler.hh"
#include "vulkan_wrapper/instance_and_surface/instance_and_surface_container.hh"
#include "vulkan_wrapper/swapchain/swapchain_info_container.hh"
#include <vulkan/vulkan_core.h>

struct VulkanAppTickState {
  vk::raii::Fence &fence_ref;
  vk::raii::Semaphore &present_complete_sem_ref;
  vk::raii::Semaphore &render_finished_sem_ref;

  uint32_t current_frame_index;
  uint32_t current_image_index;

  vk::SubmitInfo queue_submit_info;
};

struct VulkanAppInterface {
  virtual ~VulkanAppInterface() = default;

  virtual bool is_running() = 0;
  // WARN: Could be more semantic
  // INFO: if it returns nullopt, the swap chain needs recreating
  virtual std::expected<std::optional<VulkanAppTickState>, std::string>
  get_current_state(std::shared_ptr<GLFWwindow> window,
                    const vk::raii::Device &logical_device,
                    SwapchainInfo::SwapchainInfoContainer &swapchain_state) = 0;
};

struct VulkanRootCreateinfo {
  // GLFW
  uint32_t width;
  uint32_t height;
  bool window_resizable;
  std::string window_name;

  // Instance
  vk::ApplicationInfo application_info;
  bool debug_enabled;
  std::vector<const char *> layers;
  std::vector<const char *> instance_extensions;

  // Devices and Queues
  DeviceUtil::DeviceType gpu_type;
  DeviceUtil::QueueTypes queue_reqs;
  std::vector<const char *> device_extensions;

  // Swap Chain
  vk::PresentModeKHR present_mode;
};

struct VulkanRoot {
  static auto create(VulkanRootCreateinfo info)
      -> std::expected<VulkanRoot, std::string>;

  auto print_state() -> void;
  auto run_app(VulkanAppInterface &app) -> std::expected<void, std::string>;
  auto run_frame(VulkanAppTickState &state) -> std::expected<void, std::string>;
  auto recreate_swap_chain() -> std::expected<void, std::string>;

  auto end_glfw_instance() -> void;

private:
  VulkanRoot(vk::PresentModeKHR p_m, GlfwWindowContainer &&w_c,
             InstanceAndSurface::VulkanInstanceAndSurface &&i_a_s,
             Debugging::Debugger &&d,
             DeviceUtil::DeviceAndQueueContainer &&d_a_q,
             SwapchainInfo::SwapchainInfoContainer &&s_c_i)
      : present_mode(p_m), window_container(std::move(w_c)),
        instance_and_surface(std::move(i_a_s)), debugger(std::move(d)),
        device_and_queue(std::move(d_a_q)), swapchain_info(std::move(s_c_i)) {}

public:
  vk::PresentModeKHR present_mode;

  GlfwWindowContainer window_container;
  InstanceAndSurface::VulkanInstanceAndSurface instance_and_surface;
  Debugging::Debugger debugger;
  DeviceUtil::DeviceAndQueueContainer device_and_queue;
  SwapchainInfo::SwapchainInfoContainer swapchain_info;
};

#endif
