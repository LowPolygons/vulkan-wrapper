#ifndef VULKAN_WRAPPER_DEBUGGER_CONTAINER_HH
#define VULKAN_WRAPPER_DEBUGGER_CONTAINER_HH

#include <expected>
#include <iostream>
#include <vulkan/vulkan_raii.hpp>
namespace Debugging {

class Debugger {
public:
  Debugger() = delete;

  static auto create(vk::raii::Instance &instance)
      -> std::expected<Debugger, std::string>;

  auto messenger() -> vk::raii::DebugUtilsMessengerEXT &;

private:
  Debugger(vk::raii::DebugUtilsMessengerEXT &&d_m)
      : debug_messenger(std::move(d_m)) {}

private:
  vk::raii::DebugUtilsMessengerEXT debug_messenger;
};
} // namespace Debugging

#endif
