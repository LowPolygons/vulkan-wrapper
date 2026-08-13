#include "device_and_queue_container.hh"
#include <algorithm>
#include <vulkan/vulkan.hpp>

auto DeviceUtil::DeviceAndQueueContainer::physical()
    -> const vk::raii::PhysicalDevice & {
  return this->physical_device;
}

auto DeviceUtil::DeviceAndQueueContainer::logical()
    -> const vk::raii::Device & {
  return this->logical_device;
}

auto DeviceUtil::DeviceAndQueueContainer::queue() -> const vk::raii::Queue & {
  return this->graphics_queue;
}

auto DeviceUtil::DeviceAndQueueContainer::queue_index() -> uint32_t {
  return this->queue_index_var;
}

auto DeviceUtil::DeviceAndQueueContainer::create(
    DeviceCreateInfo info, const vk::raii::Instance &instance,
    const vk::raii::SurfaceKHR &surface)
    -> std::expected<DeviceAndQueueContainer, std::string> {
  auto maybe_physical_device =
      DeviceUtil::FactorHelper::create_physical_device(info, instance);

  if (!maybe_physical_device)
    return std::unexpected(maybe_physical_device.error());

  auto physical_device =
      vk::raii::PhysicalDevice{maybe_physical_device.value()};
  auto queue_index = uint32_t{0};

  auto maybe_logical_device =
      DeviceUtil::FactorHelper::create_logical_device_and_update_queue_index(
          info, instance, physical_device, queue_index, surface);

  if (!maybe_logical_device)
    return std::unexpected(maybe_logical_device.error());

  auto logical_device =
      vk::raii::Device{std::move(maybe_logical_device.value())};

  auto graphics_queue = vk::raii::Queue(logical_device, queue_index, 0);

  auto container = DeviceUtil::DeviceAndQueueContainer(
      std::move(physical_device), std::move(logical_device),
      std::move(graphics_queue), queue_index);

  return container;
}

auto DeviceUtil::FactorHelper::create_physical_device(
    DeviceUtil::DeviceCreateInfo &info, const vk::raii::Instance &instance)
    -> std::expected<vk::raii::PhysicalDevice, std::string> {
  vk::raii::PhysicalDevice physical_device = nullptr;

  if (instance == nullptr)
    return std::unexpected(
        "Tried to construct a DeviceContainer before instance was created");

  auto all_physical_devices = instance.enumeratePhysicalDevices();

  if (all_physical_devices.empty())
    return std::unexpected("There are no physical devices with vulkan support");

  auto gpu_type = vk::PhysicalDeviceType{
      (info.queue_flags == DeviceUtil::DeviceType::DISCRETE)
          ? vk::PhysicalDeviceType::eDiscreteGpu
          : vk::PhysicalDeviceType::eIntegratedGpu};

  for (auto physical : all_physical_devices) {
    auto properties = physical.getProperties();
    auto features = physical.getFeatures();

    if (properties.deviceType == gpu_type && features.geometryShader &&
        properties.apiVersion >= vk::ApiVersion13) {
      physical_device = physical;
      break;
    }
  }

  if (physical_device == nullptr)
    return std::unexpected("Failed to pick a physical device");

  auto queue_families = physical_device.getQueueFamilyProperties();
  auto available_device_extensions =
      physical_device.enumerateDeviceExtensionProperties();

  // Device Queue Families must support graphics
  if (!(std::ranges::any_of(queue_families, [](auto const &properties) {
        return static_cast<bool>(properties.queueFlags &
                                 vk::QueueFlagBits::eGraphics);
      })))
    return std::unexpected(
        "The queue families of the physical device don't support graphics");

  // Iterate over all of the extensions:
  //  -> they must all be found in the available device extensions
  if (!(std::ranges::all_of(
          info.required_device_extensions,
          [&available_device_extensions](auto const &required_extension) {
            return std::ranges::any_of(
                available_device_extensions,
                [required_extension](auto const &curr_avil_extn) {
                  return strcmp(curr_avil_extn.extensionName,
                                required_extension) == 0;
                });
          })))
    return std::unexpected(
        "The physical device does not support the right extensions");

  auto device_features = physical_device.template getFeatures2<
      vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan11Features,
      vk::PhysicalDeviceVulkan13Features,
      vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();

  // clang-format off
    if (!(
          device_features.template get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters &&
          device_features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
          device_features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState
         ))
      throw std::runtime_error("The device does not support the right features");
  // clang-format on

  return std::move(physical_device);
}

auto DeviceUtil::FactorHelper::create_logical_device_and_update_queue_index(
    DeviceUtil::DeviceCreateInfo info, const vk::raii::Instance &instance,
    vk::raii::PhysicalDevice &physical_device, uint32_t &mut_queue_index_ref,
    const vk::raii::SurfaceKHR &surface)
    -> std::expected<vk::raii::Device, std::string> {
  if (instance == nullptr || physical_device == nullptr)
    std::unexpected("Instance or physical device were not created before "
                    "logical device creation");

  // Get a list of the queue family properties
  auto queue_fam_properties = std::vector<vk::QueueFamilyProperties>{
      physical_device.getQueueFamilyProperties()};

  mut_queue_index_ref = ~0;

  for (uint32_t index = 0; index < queue_fam_properties.size(); index++) {
    if ((queue_fam_properties[index].queueFlags &
         vk::QueueFlagBits::eGraphics) &&
        physical_device.getSurfaceSupportKHR(index, *surface)) {
      mut_queue_index_ref = index;
      break;
    }
  }

  auto device_feature_chain =
      vk::StructureChain<vk::PhysicalDeviceFeatures2,
                         vk::PhysicalDeviceVulkan11Features,
                         vk::PhysicalDeviceVulkan13Features,
                         vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>{
          {},
          {.shaderDrawParameters = true},
          {.synchronization2 = true, .dynamicRendering = true},
          {.extendedDynamicState = true}};
  auto queue_priority = float{0.5};

  auto device_and_queue_create_info =
      vk::DeviceQueueCreateInfo{.queueFamilyIndex = mut_queue_index_ref,
                                .queueCount = 1,
                                .pQueuePriorities = &queue_priority};

  auto logical_device_info = vk::DeviceCreateInfo{
      .pNext = &device_feature_chain.get<vk::PhysicalDeviceFeatures2>(),
      .queueCreateInfoCount = 1,
      .pQueueCreateInfos = &device_and_queue_create_info,
      .enabledExtensionCount =
          static_cast<uint32_t>(info.required_device_extensions.size()),
      .ppEnabledExtensionNames = info.required_device_extensions.data()};

  return vk::raii::Device(physical_device, logical_device_info);
}
