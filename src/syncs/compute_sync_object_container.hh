#ifndef VULKAN_WRAPPER_VULKAN_COMPUTE_SYNC_OBJECTS_HH
#define VULKAN_WRAPPER_VULKAN_COMPUTE_SYNC_OBJECTS_HH

#include <expected>
#include <string>
#include <vector>
#include <vulkan/vulkan_raii.hpp>
namespace SyncObjects {

class ComputeSyncObjects {
public:
  ComputeSyncObjects() = delete;

  static auto create(std::size_t num_semaphores_and_fences,
                     vk::raii::Device &device)
      -> std::expected<ComputeSyncObjects, std::string>;

  auto fence(std::size_t index) -> vk::raii::Fence;
  auto semaphore(std::size_t index) -> vk::raii::Semaphore;

private:
  ComputeSyncObjects(std::vector<vk::raii::Semaphore> &&sems,
                     std::vector<vk::raii::Fence> &&fences)
      : compute_finished_semaphores(std::move(sems)),
        compute_in_flight_fences(std::move(fences)) {}

private:
  std::vector<vk::raii::Semaphore> compute_finished_semaphores;
  std::vector<vk::raii::Fence> compute_in_flight_fences;
};
} // namespace SyncObjects

#endif
