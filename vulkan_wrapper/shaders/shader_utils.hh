#ifndef VULKAN_WRAPPER_SHADERS_SHADER_UTILS_HH
#define VULKAN_WRAPPER_SHADERS_SHADER_UTILS_HH

#include <expected>
#include <string>
#include <vector>
#include <vulkan/vulkan_raii.hpp>
namespace ShaderUtils {
auto read_shader(std::string shader_path)
    -> std::expected<std::vector<char>, std::string>;

auto map_shader_bytes_to_shader_module(std::vector<char> bytecode,
                                       const vk::raii::Device &device)
    -> vk::raii::ShaderModule;
} // namespace ShaderUtils

#endif
