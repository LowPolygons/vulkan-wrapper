# Vulkan Implementation

## TODO
DeviceAndQueueContainer needs a flag to specify if the root needs graphics, compute or both

Theoretical idea:

## VulkanRoot
```cpp
struct VulkanRoot {
  GlfwWindow;
  VulkanInstanceAndSurface;
  DeviceAndQueueContainer;
  SwapchainInfoContainer;

  Allocators?;
  Memory Objects;
};
```

This VulkanRoot contains all the information that is shared across many 'VulkanApp'

## VulkanApp
```cpp
struct VulkanApp {
  PipelineContainer;
  ...
};
```
In a polymorphic manner

## VulkanTickObject
```cpp
struct VulkanTickObject {
    vk::CommandBuffer command_buffer;
    vk::Semaphore image_available;
    vk::Semaphore render_finished;
    vk::Fence fence;

    uint32_t swapchain_image;
};
```

## Example Usage
```cpp

VulkanRoot vulkan{};

VulkanAppImpl1 app1{};
VulkanAppImpl2 app2{};
VulkanAppImpl3 app3{};

auto& chosen_app = app2;

std::expected<void, string> success = vulkan.run_app(chosen_app);
```

```cpp
vulkan.run_app 

run_app(App& app) {
  while (app.is_running()) {
    auto curr_tick = create_tick_object();
    
    app.record_tick_operation(curr_tick);

    submit_for_execution(curr_tick);
  }
}
```

