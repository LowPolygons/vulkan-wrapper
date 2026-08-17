#ifndef VULKAN_WRAPPER_DEVICE_CONTAINER_HG
#define VULKAN_WRAPPER_DEVICE_CONTAINER_HG

#include <expected>
#include <vulkan/vulkan_raii.hpp>

namespace DeviceUtil {

enum class DeviceType { DISCRETE, INTEGRATED };

struct DeviceCreateInfo {
  DeviceType queue_flags;
  std::vector<const char *> required_device_extensions;
};

class DeviceAndQueueContainer {

public:
  [[nodiscard]] static auto create(DeviceCreateInfo info,
                                   const vk::raii::Instance &instance,
                                   const vk::raii::SurfaceKHR &surface)
      -> std::expected<DeviceAndQueueContainer, std::string>;

  DeviceAndQueueContainer() = delete;

  auto physical() -> vk::raii::PhysicalDevice &;
  auto logical() -> vk::raii::Device &;
  auto queue() -> vk::raii::Queue &;
  auto queue_index() -> uint32_t;

private:
  DeviceAndQueueContainer(vk::raii::PhysicalDevice &&physical_device,
                          vk::raii::Device &&logical_device,
                          vk::raii::Queue &&queue, uint32_t queue_index)
      : physical_device(std::move(physical_device)),
        logical_device(std::move(logical_device)),
        graphics_queue(std::move(queue)), queue_index_var(queue_index) {}

private:
  vk::raii::PhysicalDevice physical_device;
  vk::raii::Device logical_device;
  vk::raii::Queue graphics_queue;
  uint32_t queue_index_var;
};

namespace FactorHelper {
[[nodiscard]] auto create_physical_device(DeviceUtil::DeviceCreateInfo &info,
                                          const vk::raii::Instance &instance)
    -> std::expected<vk::raii::PhysicalDevice, std::string>;

[[nodiscard]] auto create_logical_device_and_update_queue_index(
    DeviceUtil::DeviceCreateInfo info, const vk::raii::Instance &instance,
    vk::raii::PhysicalDevice &physical_device, uint32_t &mut_queue_index_ref,
    const vk::raii::SurfaceKHR &surface)
    -> std::expected<vk::raii::Device, std::string>;
} // namespace FactorHelper

} // namespace DeviceUtil
#endif
