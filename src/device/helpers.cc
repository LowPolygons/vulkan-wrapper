#include "helpers.hh"
#include <expected>

auto DeviceUtil::create_buffer(const vk::raii::Device &device,
                               const vk::raii::PhysicalDevice &physical_device,
                               vk::DeviceSize buffer_create_info_size,
                               vk::BufferUsageFlags buffer_create_info_usage,
                               vk::MemoryPropertyFlags property_flags)
    -> std::expected<std::pair<vk::raii::Buffer, vk::raii::DeviceMemory>,
                     std::string> {

  auto buffer_info =
      vk::BufferCreateInfo{.size = buffer_create_info_size,
                           .usage = buffer_create_info_usage,
                           .sharingMode = vk::SharingMode::eExclusive};

  auto buffer = vk::raii::Buffer(device, buffer_info);
  auto memory_reqs = vk::MemoryRequirements{buffer.getMemoryRequirements()};

  auto maybe_memory_type_index = DeviceUtil::find_memory_type(
      physical_device, memory_reqs.memoryTypeBits, property_flags);

  if (!maybe_memory_type_index)
    return std::unexpected(maybe_memory_type_index.error());

  auto memory_alloc_info = vk::MemoryAllocateInfo{
      .allocationSize = memory_reqs.size,
      .memoryTypeIndex = maybe_memory_type_index.value()};
  auto buffer_memory = vk::raii::DeviceMemory(device, memory_alloc_info);

  buffer.bindMemory(*buffer_memory, 0);

  return std::make_pair(std::move(buffer), std::move(buffer_memory));
}

// It isnt (of course) as simple as now allocating the memory, as different
// GPUs may offer differnet types of memory to allocate
auto DeviceUtil::find_memory_type(
    const vk::raii::PhysicalDevice &physical_device, uint32_t type_filter,
    vk::MemoryPropertyFlags property_flags)
    -> std::expected<uint32_t, std::string> {
  // device mem properties
  // Contains memoryTypes and memoryHeaps
  // A mem heap is a 'distinct memotry resource like dedicated vram and swap
  // space in ram for when vram runs out'
  vk::PhysicalDeviceMemoryProperties memory_props =
      physical_device.getMemoryProperties();
  // typefilter is a bit field of suitable memory types
  // this iterates over the bits and checks if the bit == 1
  //
  // we also need the data to be host visible and coherent for this usecase
  // these will be defiend in the property_flags
  for (auto i = 0; i < memory_props.memoryTypeCount; i++) {
    if ((type_filter & (1 << i)) && (memory_props.memoryTypes[i].propertyFlags &
                                     property_flags) == property_flags)
      return i;
  }
  return std::unexpected("Failed to find suitable memory type");
}
