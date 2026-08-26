#include "swapchain_info_container.hh"
#include <iostream>

auto SwapchainInfo::SwapchainInfoContainer::swap_chain()
    -> vk::raii::SwapchainKHR & {
  return _swap_chain;
}

auto SwapchainInfo::SwapchainInfoContainer::images()
    -> std::vector<vk::Image> & {
  return _images;
}

auto SwapchainInfo::SwapchainInfoContainer::image_views()
    -> std::vector<vk::raii::ImageView> & {
  return _image_views;
}

auto SwapchainInfo::SwapchainInfoContainer::surface_format()
    -> vk::SurfaceFormatKHR & {
  return _surface_format;
}

auto SwapchainInfo::SwapchainInfoContainer::dimensions() -> vk::Extent2D & {
  return _dimensions;
}

auto SwapchainInfo::SwapchainInfoContainer::wipe() -> void {
  _swap_chain = nullptr;
  _images.clear();
  _image_views.clear();
}

auto SwapchainInfo::SwapchainInfoContainer::create(
    SwapchainInfo::SwapchainInfoContainerCreateInfo info,
    SwapchainInfoObjectRefs object_refs)
    -> std::expected<SwapchainInfoContainer, std::string> {
  // Extract info from physical device such as surface capabilities
  auto surface_capabilities =
      object_refs.physical_device_ref.getSurfaceCapabilitiesKHR(
          object_refs.surface_ref);
  auto available_formats = std::vector<vk::SurfaceFormatKHR>{
      object_refs.physical_device_ref.getSurfaceFormatsKHR(
          object_refs.surface_ref)};
  auto available_present_modes = std::vector<vk::PresentModeKHR>{
      object_refs.physical_device_ref.getSurfacePresentModesKHR(
          object_refs.surface_ref)};

  auto maybe_chosen_surface = std::expected<vk::SurfaceFormatKHR, std::string>{
      FactoryHelper::choose_surface_format(available_formats)};
  if (!maybe_chosen_surface)
    return std::unexpected(maybe_chosen_surface.error());

  if (!std::ranges::any_of(available_present_modes,
                           [&info](vk::PresentModeKHR &curr_present_mode) {
                             return curr_present_mode == info.present_mode;
                           }))
    return std::unexpected("Desired present mode was unavailable");

  auto maybe_extent = std::expected<vk::Extent2D, std::string>{};

  // if (GLFWwindow *window = object_refs.weak_window.lock().get()) {
  maybe_extent = std::expected<vk::Extent2D, std::string>{
      FactoryHelper::choose_extent(surface_capabilities, object_refs.window)};

  if (!maybe_extent)
    return std::unexpected(maybe_extent.error());
  // } else {
  //   return std::unexpected(
  //       "Unable to access GLFWwindow weak ptr. Is it still alive?");
  // }

  auto image_count = surface_capabilities.minImageCount + 1;

  if (surface_capabilities.maxImageCount > 0 &&
      image_count > surface_capabilities.maxImageCount) {
    image_count = surface_capabilities.maxImageCount;
  }
  auto swap_chain_create_info = vk::SwapchainCreateInfoKHR{
      .surface = *object_refs.surface_ref,
      .minImageCount = image_count,
      .imageFormat = maybe_chosen_surface.value().format,
      .imageColorSpace = maybe_chosen_surface.value().colorSpace,
      .imageExtent = maybe_extent.value(),
      .imageArrayLayers = 1,
      //==//
      .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
      .imageSharingMode = vk::SharingMode::eExclusive,
      .preTransform = surface_capabilities.currentTransform,
      .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
      .presentMode = info.present_mode,
      .clipped = true};

  auto swap_chain =
      vk::raii::SwapchainKHR(object_refs.device_ref, swap_chain_create_info);
  auto images = swap_chain.getImages();

  std::cout << "Images size: " << images.size() << std::endl;

  auto surface_format = maybe_chosen_surface.value();
  auto extent = maybe_extent.value();

  auto maybe_image_views = FactoryHelper::get_image_views(
      images, surface_format, object_refs.device_ref);

  if (!maybe_image_views)
    return std::unexpected(maybe_image_views.error());

  auto object =
      SwapchainInfoContainer(std::move(swap_chain), std::move(images),
                             std::move(maybe_image_views.value()),
                             std::move(surface_format), std::move(extent));

  return object;
}

auto SwapchainInfo::FactoryHelper::choose_surface_format(
    std::vector<vk::SurfaceFormatKHR> available_formats)
    -> std::expected<vk::SurfaceFormatKHR, std::string> {

  if (available_formats.empty())
    return std::unexpected("There are no available surface formats");

  bool format_was_chosen = false;
  auto chosen_surface_format = vk::SurfaceFormatKHR{};

  for (vk::SurfaceFormatKHR surface_form : available_formats) {
    if (surface_form.format == vk::Format::eB8G8R8A8Srgb &&
        surface_form.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
      chosen_surface_format = surface_form;
      format_was_chosen = true;
      break;
    }
  }

  if (!format_was_chosen)
    return std::unexpected(
        "There were not surface formats which met the requirements");

  return chosen_surface_format;
}

auto SwapchainInfo::FactoryHelper::choose_extent(
    vk::SurfaceCapabilitiesKHR surface_capabilities, GLFWwindow *window)
    -> std::expected<vk::Extent2D, std::string> {
  auto extent = std::pair<int, int>{-1, -1};

  if (surface_capabilities.currentExtent.width !=
      std::numeric_limits<uint32_t>::max()) {
    extent = {static_cast<int>(surface_capabilities.currentExtent.width),
              static_cast<int>(surface_capabilities.currentExtent.height)};
  } else {
    int width, height;

    glfwGetFramebufferSize(window, &width, &height);

    extent.first = static_cast<int>(
        std::clamp<uint32_t>(width, surface_capabilities.minImageExtent.width,
                             surface_capabilities.maxImageExtent.width));
    extent.second = static_cast<int>(
        std::clamp<uint32_t>(height, surface_capabilities.minImageExtent.width,
                             surface_capabilities.maxImageExtent.height));
  }

  if (extent.first < 0 || extent.second < 0)
    return std::unexpected("Not able to determine the swap chain extent");

  return vk::Extent2D{static_cast<uint32_t>(extent.first),
                      static_cast<uint32_t>(extent.second)};
}

auto SwapchainInfo::FactoryHelper::get_image_views(
    std::vector<vk::Image> &images, vk::SurfaceFormatKHR surface_format,
    vk::raii::Device &device)
    -> std::expected<std::vector<vk::raii::ImageView>, std::string> {
  // if (images.empty())
  //   return std::unexpected("Swap chain images array was empty");

  auto image_view_create_info = vk::ImageViewCreateInfo{
      .viewType = vk::ImageViewType::e2D,
      .format = surface_format.format,
      .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};

  auto image_views = std::vector<vk::raii::ImageView>{};

  for (auto &image : images) {
    image_view_create_info.image = image;
    image_views.emplace_back(device, image_view_create_info);
  }

  return image_views;
}
