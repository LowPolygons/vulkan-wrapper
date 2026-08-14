#ifndef VULKAN_WRAPPER_VULKAN_SYNC_OBJECTS_HH
#define VULKAN_WRAPPER_VULKAN_SYNC_OBJECTS_HH

#include <expected>
#include <vulkan/vulkan_raii.hpp>
namespace SyncObjects {

class SyncObjectsContainer {
public:
  SyncObjectsContainer() = delete;

  auto fence(std::size_t index) -> vk::raii::Fence &;
  auto render_finished_semaphore(std::size_t index) -> vk::raii::Semaphore &;
  auto present_complete_semaphore(std::size_t index) -> vk::raii::Semaphore &;

  static auto create(std::size_t present_complete_semaphore_size,
                     std::size_t render_finished_semaphore_size,
                     std::size_t draw_fence_size, vk::raii::Device &device)
      -> std::expected<SyncObjectsContainer, std::string>;

private:
  SyncObjectsContainer(std::vector<vk::raii::Semaphore> &&p_c_s,
                       std::vector<vk::raii::Semaphore> &&r_f_s,
                       std::vector<vk::raii::Fence> &&d_fs)
      : present_complete_semaphores(std::move(p_c_s)),
        render_finished_semaphores(std::move(r_f_s)),
        draw_fences(std::move(d_fs)) {}

private:
  std::vector<vk::raii::Semaphore> present_complete_semaphores;
  std::vector<vk::raii::Semaphore> render_finished_semaphores;
  std::vector<vk::raii::Fence> draw_fences;
};

} // namespace SyncObjects

#endif
