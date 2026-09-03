#include "vulkan_wrapper/debugger/debugger_container.hh"

// TODO: better understand this function signature to allow a more modular
// wrapper
static VKAPI_ATTR vk::Bool32 VKAPI_CALL callback_function(
    vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
    vk::DebugUtilsMessageTypeFlagsEXT type,
    const vk::DebugUtilsMessengerCallbackDataEXT *p_callback_data, void *) {
  std::cerr << "\n========== VULKAN DEBUG ==========\n";
  std::cerr << "Severity: " << to_string(severity) << '\n';
  std::cerr << "Type:     " << to_string(type) << '\n';
  std::cerr << "Message:  " << p_callback_data->pMessage << '\n';
  std::cerr << "==================================\n\n";

  return vk::False;
}

auto Debugging::Debugger::create(const vk::raii::Instance &instance)
    -> std::expected<Debugger, std::string> {
#define MessageSeverity vk::DebugUtilsMessageSeverityFlagBitsEXT
#define MessageType vk::DebugUtilsMessageTypeFlagBitsEXT

  vk::DebugUtilsMessageSeverityFlagsEXT severity_flags(
      MessageSeverity::eWarning | MessageSeverity::eError);
  vk::DebugUtilsMessageTypeFlagsEXT message_type_flags(
      MessageType::eGeneral | MessageType::ePerformance |
      MessageType::eValidation);

  vk::DebugUtilsMessengerCreateInfoEXT debug_utils_messenger_info{
      .messageSeverity = severity_flags,
      .messageType = message_type_flags,
      // pointer to call back function
      .pfnUserCallback = &callback_function,
  };

  auto debug_messenger =
      instance.createDebugUtilsMessengerEXT(debug_utils_messenger_info);

  auto object = Debugger(std::move(debug_messenger));

  return object;
}
