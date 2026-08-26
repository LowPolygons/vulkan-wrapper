#ifndef VULKAN_WRAPPER_HELPER_FUNCTIONS_HH
#define VULKAN_WRAPPER_HELPER_FUNCTIONS_HH

#include <expected>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace DeviceUtil {

auto create_buffer(const vk::raii::Device &device,
                   const vk::raii::PhysicalDevice &physical_device,
                   vk::DeviceSize buffer_create_info_size,
                   vk::BufferUsageFlags buffer_create_info_usage,
                   vk::MemoryPropertyFlags property_flags)
    -> std::expected<std::pair<vk::raii::Buffer, vk::raii::DeviceMemory>,
                     std::string>;

auto find_memory_type(const vk::raii::PhysicalDevice &phys_device,
                      uint32_t type_filter,
                      vk::MemoryPropertyFlags property_flags)
    -> std::expected<uint32_t, std::string>;
} // namespace DeviceUtil

#endif
