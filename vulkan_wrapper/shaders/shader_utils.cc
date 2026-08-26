#include "vulkan_wrapper/shaders/shader_utils.hh"
#include <fstream>

auto ShaderUtils::read_shader(std::string shader_path)
    -> std::expected<std::vector<char>, std::string> {
  // End of file lets you immediately see the size of the file
  auto file = std::ifstream(shader_path, std::ios::ate | std::ios::binary);

  if (!file.is_open())
    return std::unexpected("Failed to open shader bytecode");

  auto buffer = std::vector<char>(file.tellg());

  file.seekg(0, std::ios::beg);
  file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));

  file.close();

  return buffer;
}

auto ShaderUtils::map_shader_bytes_to_shader_module(std::vector<char> bytecode,
                                                    vk::raii::Device &device)
    -> vk::raii::ShaderModule {
  auto create_info = vk::ShaderModuleCreateInfo{
      .codeSize = bytecode.size() * sizeof(char),
      // THe size fo the bytecode is specified in bytes, but the bytecode
      // pointer is a uint32_t so you reinterpret cast
      .pCode = reinterpret_cast<const uint32_t *>(bytecode.data())};

  return vk::raii::ShaderModule(device, create_info);
}
