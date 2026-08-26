# Vulkan Wrapper 

## Conways Game of Life App
![Conways](conways_hello_world.png)

## VulkanRoot
```cpp
struct VulkanRoot {
  run_app(VulkanAppInterface&) -> std::expected<void, std::string>
  run_frame(VulkanAppTickState&) -> std::expected<void, std::string>
  recreate_swap_chain() -> std::expected<void, std::string>
  end_glfw_instance() -> void;
  ...
  GlfwWindow;
  VulkanInstanceAndSurface;
  DeviceAndQueueContainer;
  SwapchainInfoContainer;
};
```

This VulkanRoot contains all the information that is shared across many different vulkan apps

## VulkanApp
```cpp
struct VulkanAppInterface {
  is_running() -> bool
  get_current_state() -> std::expected<std::optional<VulkanAppTickState>, std::string>
};
```

An app to be used by the Vulkan Renderer must implement this interface

## VulkanAppTickState 
```cpp
struct VulkanAppTickState {
  vk::raii::Fence &fence_ref;
  vk::raii::Semaphore &present_complete_sem_ref;
  vk::raii::Semaphore &render_finished_sem_ref;

  uint32_t current_frame_index;
  uint32_t current_image_index;

  vk::SubmitInfo queue_submit_info;
};
```

## Example Usage
```cpp

VulkanRoot vulkan{..initialised..};
MandelbulbApp app1{..initialised..};

std::expected<void, string> success = vulkan.run_app(chosen_app);
```

```cpp
VulkanRoot::run_app(App& app) {
  while (app.is_running() and glfwWindowRunning) {
    poll_glfw_events()

    current_state = app.get_current_state()
    .. validate it ..

    if (swapchain needs recreating) {
      recreate_swap_chain()
    }
  
    run_frame(current_state)

    .. confirm success ..
  }
}
```

