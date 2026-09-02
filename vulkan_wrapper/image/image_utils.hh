#ifndef VULKAN_WRAPPER_IMAGE_UTILS_HH
#define VULKAN_WRAPPER_IMAGE_UTILS_HH

#include "vulkan/vulkan.hpp"
#include <expected>
#include <utility>
#include <vulkan/vulkan_raii.hpp>
namespace ImageUtils {
auto create_image(vk::raii::Device &device, vk::raii::PhysicalDevice &physical,
                  std::pair<uint32_t, uint32_t> image_dimensions,
                  vk::Format image_format, vk::ImageTiling tiling,
                  vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties)
    -> std::expected<std::pair<vk::raii::Image, vk::raii::DeviceMemory>,
                     std::string>;
}

#endif
