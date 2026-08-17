#include "instance_and_surface_container.hh"
#include "vulkan/vulkan.hpp"

auto InstanceAndSurface::VulkanInstanceAndSurface::instance()
    -> vk::raii::Instance & {
  return _instance;
}

auto InstanceAndSurface::VulkanInstanceAndSurface::surface()
    -> vk::raii::SurfaceKHR & {
  return _surface;
}

auto InstanceAndSurface::VulkanInstanceAndSurface::create(
    VulkanInstanceAndSurfaceCreateInfo info,
    const vk::raii::Context &context_ref, GLFWwindow *window)
    -> std::expected<VulkanInstanceAndSurface, std::string> {

  auto required_layers = std::vector<const char *>{};

  if (info.validation_layers_enabled) {
    for (auto &layer : info.validation_layers)
      required_layers.push_back(layer);

    auto available_layers = context_ref.enumerateInstanceLayerProperties();

    auto maybe_layers_validated = std::expected<void, std::string>{
        FactoryHelper::validate_layers(required_layers, available_layers)};

    if (!maybe_layers_validated)
      return std::unexpected(maybe_layers_validated.error());
  }

  auto available_extensions =
      context_ref.enumerateInstanceExtensionProperties();

  auto glfw_extension_count = uint32_t{0};
  auto required_window_extensions =
      glfwGetRequiredInstanceExtensions(&glfw_extension_count);

  auto all_required_extensions = std::vector<const char *>{
      required_window_extensions,
      required_window_extensions + glfw_extension_count};

  // for (auto &additional_ext : info.additional_extensions)
  //   all_required_extensions.push_back(additional_ext);

  auto maybe_extensions_validated =
      std::expected<void, std::string>{FactoryHelper::validate_extensions(
          all_required_extensions, available_extensions)};

  if (!maybe_extensions_validated)
    return std::unexpected(maybe_extensions_validated.error());

  auto instance_create_info = vk::InstanceCreateInfo{
      .pApplicationInfo = &info.app_info,
      .enabledLayerCount = static_cast<uint32_t>(required_layers.size()),
      .ppEnabledLayerNames = required_layers.data(),
      .enabledExtensionCount =
          static_cast<uint32_t>(all_required_extensions.size()),
      .ppEnabledExtensionNames = all_required_extensions.data()};

  auto vk_instance = vk::raii::Instance(context_ref, instance_create_info);

  // Surface creation is much easier
  auto local_surface = VkSurfaceKHR{};
  if (glfwCreateWindowSurface(*vk_instance, window, nullptr, &local_surface) !=
      0)
    return std::unexpected("Failed to create a window surface");

  auto surface = vk::raii::SurfaceKHR(vk_instance, local_surface);

  auto object =
      VulkanInstanceAndSurface(std::move(vk_instance), std::move(surface));

  return object;
}

auto InstanceAndSurface::FactoryHelper::validate_layers(
    const std::vector<const char *> required,
    const std::vector<vk::LayerProperties> available)
    -> std::expected<void, std::string> {
  auto unsupported_layers_it =
      std::ranges::find_if(required, [&available](auto const &required_layer) {
        return std::ranges::none_of(
            available, [required_layer](auto const &layer_property) {
              return strcmp(layer_property.layerName, required_layer) == 0;
            });
      });

  if (unsupported_layers_it != required.end())
    return std::unexpected("Required layer is not supported: " +
                           std::string(*unsupported_layers_it));

  return {};
}
auto InstanceAndSurface::FactoryHelper::validate_extensions(
    const std::vector<const char *> required,
    const std::vector<vk::ExtensionProperties> available)
    -> std::expected<void, std::string> {
  for (uint32_t i = 0; i < required.size(); i++) {
    if (std::ranges::none_of(
            available, [curr_ext = required[i]](auto const &property) {
              return strcmp(property.extensionName, curr_ext) == 0;
            }))
      return std::unexpected("Required extension is not supported: " +
                             std::string(required[i]));
  }

  return {};
}
