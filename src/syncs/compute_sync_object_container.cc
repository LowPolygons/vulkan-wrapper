#include "compute_sync_object_container.hh"

auto SyncObjects::ComputeSyncObjects::create(
    std::size_t num_semaphores_and_fences, vk::raii::Device &device)
    -> std::expected<ComputeSyncObjects, std::string> {
  if (device == nullptr)
    return std::unexpected("Device wasn't initialised before trying to create "
                           "compute sync objects");

  auto fences = std::vector<vk::raii::Fence>{};
  auto semaphores = std::vector<vk::raii::Semaphore>{};

  for (auto i = 0; i < num_semaphores_and_fences; i++) {
    fences.emplace_back(vk::raii::Fence(
        device,
        vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled}));
    semaphores.emplace_back(
        vk::raii::Semaphore(device, vk::SemaphoreCreateInfo{}));
  }

  auto object = ComputeSyncObjects(std::move(semaphores), std::move(fences));

  return object;
}
