#pragma once

#include "context.hpp"

#include <cstdint>
#include <functional>
#include <limits>
#include <vector>

namespace bubble {

using TaskID = std::uint32_t;
using TaskCall = std::function<void(vk::CommandBuffer)>;
inline constexpr const auto invalid_task_id =
    std::numeric_limits<TaskID>::max();

struct ComesAfter {
  explicit constexpr ComesAfter(int ignored) : _ignored{ignored} {}
  int _ignored;
};

inline constexpr const auto comes_after = ComesAfter(7);

struct Dependency {
  TaskID task{invalid_task_id};
  TaskID comes_after{invalid_task_id};
  std::optional<vk::UniqueSemaphore> semaphore;
};

struct Task {
  TaskID id{invalid_task_id};
  TaskCall call;
  std::optional<vk::UniqueCommandBuffer> commandbuffer;
};

class TaskGraph {
public:
  auto add_dependency(TaskID id, ComesAfter, TaskID comes_after) -> void;

  auto add_task(TaskID id, TaskCall call) -> void;

  auto set_end(TaskID end) -> void;

  [[nodiscard]]
  auto evaluate() -> vk::Semaphore;

private:
  auto prune_graph() -> void;

  [[nodiscard]]
  auto get_task(TaskID id) -> Task *;

  [[nodiscard]]
  auto get_parent_dependencies(TaskID id) -> std::vector<Dependency *>;

  [[nodiscard]]
  auto get_child_dependencies(TaskID id) -> std::vector<Dependency *>;

  TaskID _end{invalid_task_id};
  std::vector<Task> _tasks;
  std::vector<Dependency> _dependencies;
  std::optional<vk::UniqueSemaphore> _finished_signal;
};

} // namespace bubble
