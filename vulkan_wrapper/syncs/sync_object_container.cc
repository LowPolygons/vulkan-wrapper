#include "vulkan_wrapper/syncs/sync_object_container.hh"

auto SyncObjects::SyncObjectsContainer::fence(std::size_t index)
    -> vk::raii::Fence & {
  return draw_fences.at(index);
}

auto SyncObjects::SyncObjectsContainer::render_finished_semaphore(
    std::size_t index) -> vk::raii::Semaphore & {
  return render_finished_semaphores.at(index);
}

auto SyncObjects::SyncObjectsContainer::present_complete_semaphore(
    std::size_t index) -> vk::raii::Semaphore & {
  return present_complete_semaphores.at(index);
}

auto SyncObjects::SyncObjectsContainer::create(
    std::size_t present_complete_semaphore_size,
    std::size_t render_finished_semaphore_size, std::size_t draw_fence_size,
    const vk::raii::Device &device)
    -> std::expected<SyncObjectsContainer, std::string> {
  if (device == nullptr)
    return std::unexpected(
        "Device was not initialised before use in making sync objects");

  std::vector<vk::raii::Semaphore> present_complete_semaphores;
  std::vector<vk::raii::Semaphore> render_finished_semaphores;
  std::vector<vk::raii::Fence> fences;

  for (auto i = 0; i < present_complete_semaphore_size; i++)
    present_complete_semaphores.emplace_back(device, vk::SemaphoreCreateInfo());

  for (auto i = 0; i < render_finished_semaphore_size; i++)
    render_finished_semaphores.emplace_back(device, vk::SemaphoreCreateInfo());

  for (auto i = 0; i < draw_fence_size; i++)
    fences.emplace_back(
        device,
        vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});

  auto object = SyncObjectsContainer(std::move(present_complete_semaphores),
                                     std::move(render_finished_semaphores),
                                     std::move(fences));

  return object;
}
