#ifndef VULKAN_WRAPPER_UNIFORM_BUFFER_CONTAINER_HH
#define VULKAN_WRAPPER_UNIFORM_BUFFER_CONTAINER_HH

#include "../device/helpers.hh"
#include <expected>
#include <string>
#include <vulkan/vulkan_raii.hpp>

namespace BufferUtils {

struct UniformBufferContainerCreateInfo {
  vk::ShaderStageFlagBits stage;
  std::size_t max_frames_in_flight;
};

template <typename UB> class UniformBufferContainer {
public:
  UniformBufferContainer() = delete;

  static auto create(UniformBufferContainerCreateInfo info,
                     vk::raii::Device &device,
                     vk::raii::PhysicalDevice &phys_device)
      -> std::expected<UniformBufferContainer, std::string>;

  auto descriptor_set_layout() -> vk::raii::DescriptorSetLayout &;
  auto descriptor_pool() -> vk::raii::DescriptorPool &;
  auto descriptor_set(std::size_t index) -> vk::raii::DescriptorSet &;

  auto uniform_buffer(std::size_t index) -> vk::raii::Buffer &;
  auto uniform_buffer_memory(std::size_t index) -> vk::raii::DeviceMemory &;
  auto uniform_buffer_mapped(std::size_t index) -> void *;

private:
  UniformBufferContainer(vk::raii::DescriptorSetLayout &&d_s_l,
                         vk::raii::DescriptorPool &&d_p,
                         std::vector<vk::raii::DescriptorSet> &&v_d_s,
                         std::vector<vk::raii::Buffer> &&v_u_b,
                         std::vector<vk::raii::DeviceMemory> &&v_u_b_m,
                         std::vector<void *> v_u_b_r_m)
      : _descriptor_set_layout(std::move(d_s_l)),
        _descriptor_pool(std::move(d_p)), _descriptor_sets(std::move(v_d_s)),
        _uniform_buffers(std::move(v_u_b)),
        _uniform_buffers_memory(std::move(v_u_b_m)),
        _uniform_buffers_mapped(std::move(v_u_b_r_m)) {}

private:
  vk::raii::DescriptorSetLayout _descriptor_set_layout;
  vk::raii::DescriptorPool _descriptor_pool;
  std::vector<vk::raii::DescriptorSet> _descriptor_sets;

  std::vector<vk::raii::Buffer> _uniform_buffers;
  std::vector<vk::raii::DeviceMemory> _uniform_buffers_memory;
  std::vector<void *> _uniform_buffers_mapped;
};

} // namespace BufferUtils

template <typename UB>
auto BufferUtils::UniformBufferContainer<UB>::create(
    BufferUtils::UniformBufferContainerCreateInfo info,
    vk::raii::Device &device, vk::raii::PhysicalDevice &phys_device)
    -> std::expected<UniformBufferContainer, std::string> {
  // Descriptor Set Layout
  auto uniform_buffer_layout_binding = vk::DescriptorSetLayoutBinding{
      .binding = 0,
      .descriptorType = vk::DescriptorType::eUniformBuffer,
      .descriptorCount = 1,
      .stageFlags = info.stage};

  auto ubo_layout = vk::DescriptorSetLayoutCreateInfo{
      .bindingCount = 1, .pBindings = &uniform_buffer_layout_binding};

  auto descriptor_set_layout =
      vk::raii::DescriptorSetLayout(device, ubo_layout);

  // Initialise Uniform Buffers
  auto uniform_buffers = std::vector<vk::raii::Buffer>{};
  auto uniform_buffers_memory = std::vector<vk::raii::DeviceMemory>{};
  auto uniform_buffers_mapped = std::vector<void *>{};

  for (auto i = 0; i < info.max_frames_in_flight; i++) {
    auto buffer_size = vk::DeviceSize{sizeof(UB)};
    auto maybe_buf_and_mem = DeviceUtil::create_buffer(
        device, phys_device, buffer_size,
        vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent);

    if (!maybe_buf_and_mem)
      return std::unexpected("3D App Uniform Buffer Init Error: " +
                             maybe_buf_and_mem.error());
    auto [buffer, buffer_mem] = std::move(maybe_buf_and_mem.value());

    uniform_buffers.emplace_back(std::move(buffer));
    uniform_buffers_memory.emplace_back(std::move(buffer_mem));
    uniform_buffers_mapped.emplace_back(
        uniform_buffers_memory.back().mapMemory(0, buffer_size));
  }

  // Remaining Descriptor Data
  auto descriptor_pool_size = vk::DescriptorPoolSize{
      .type = vk::DescriptorType::eUniformBuffer,
      .descriptorCount = static_cast<uint32_t>(info.max_frames_in_flight)};
  auto descriptor_pool_info = vk::DescriptorPoolCreateInfo{
      .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
      .maxSets = static_cast<uint32_t>(info.max_frames_in_flight),
      .poolSizeCount = 1,
      .pPoolSizes = &descriptor_pool_size};

  auto descriptor_pool = vk::raii::DescriptorPool(device, descriptor_pool_info);

  auto layouts = std::vector<vk::DescriptorSetLayout>(info.max_frames_in_flight,
                                                      *descriptor_set_layout);

  auto allocation_info = vk::DescriptorSetAllocateInfo{
      .descriptorPool = descriptor_pool,
      .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
      .pSetLayouts = layouts.data()};

  auto descriptor_sets = device.allocateDescriptorSets(allocation_info);

  for (auto i = 0; i < info.max_frames_in_flight; i++) {
    auto buffer_info = vk::DescriptorBufferInfo{
        .buffer = uniform_buffers[i], .offset = 0, .range = sizeof(UB)};
    auto descriptor_write = vk::WriteDescriptorSet{
        .dstSet = descriptor_sets[i],
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .pBufferInfo = &buffer_info};

    device.updateDescriptorSets(descriptor_write, {});
  }

  auto object = UniformBufferContainer<UB>(
      std::move(descriptor_set_layout), std::move(descriptor_pool),
      std::move(descriptor_sets), std::move(uniform_buffers),
      std::move(uniform_buffers_memory), std::move(uniform_buffers_mapped));

  return object;
}

template <typename UB>
auto BufferUtils::UniformBufferContainer<UB>::descriptor_set_layout()
    -> vk::raii::DescriptorSetLayout & {
  return _descriptor_set_layout;
}

template <typename UB>
auto BufferUtils::UniformBufferContainer<UB>::descriptor_pool()
    -> vk::raii::DescriptorPool & {
  return _descriptor_pool;
}

template <typename UB>
auto BufferUtils::UniformBufferContainer<UB>::descriptor_set(std::size_t index)
    -> vk::raii::DescriptorSet & {
  return _descriptor_sets[index];
}

template <typename UB>
auto BufferUtils::UniformBufferContainer<UB>::uniform_buffer(std::size_t index)
    -> vk::raii::Buffer & {
  return _uniform_buffers[index];
}

template <typename UB>
auto BufferUtils::UniformBufferContainer<UB>::uniform_buffer_memory(
    std::size_t index) -> vk::raii::DeviceMemory & {
  return _uniform_buffers_memory[index];
}

template <typename UB>
auto BufferUtils::UniformBufferContainer<UB>::uniform_buffer_mapped(
    std::size_t index) -> void * {
  return _uniform_buffers_mapped[index];
}

#endif
