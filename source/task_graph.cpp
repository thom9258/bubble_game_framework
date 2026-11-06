#include "task_graph.hpp"
#include <algorithm>
#include <ranges>
#include <vulkan/vulkan_enums.hpp>

namespace bubble {

auto TaskGraph::add_dependency(TaskID id, ComesAfter, TaskID comes_after)
    -> void {
  auto dep = Dependency{};
  dep.task = id;
  dep.comes_after = comes_after;
  _dependencies.push_back(std::move(dep));
}

auto TaskGraph::add_task(TaskID id, TaskCall call) -> void {
  auto t = Task{};
  t.id = id;
  t.call = std::move(call);
  _tasks.push_back(std::move(t));
}

auto TaskGraph::set_end(TaskID end) -> void { _end = end; }

auto TaskGraph::get_task(TaskID id) -> Task * {
  for (auto &task : _tasks) {
    if (task.id == id) {
      return &task;
    }
  }

  return nullptr;
}

auto TaskGraph::get_parent_dependencies(TaskID id)
    -> std::vector<Dependency *> {
  std::vector<Dependency *> parent_dependencies;
  for (auto &dependency : _dependencies) {
    if (dependency.comes_after == id) {
      parent_dependencies.push_back(&dependency);
    }
  }

  return parent_dependencies;
}

auto TaskGraph::get_child_dependencies(TaskID id) -> std::vector<Dependency *> {
  std::vector<Dependency *> child_dependencies;
  for (auto &dependency : _dependencies) {
    if (dependency.task == id) {
      child_dependencies.push_back(&dependency);
    }
  }

  return child_dependencies;
}

auto TaskGraph::prune_graph() -> void {}

auto TaskGraph::evaluate() -> vk::Semaphore {
  prune_graph();

  for (auto &dependency : _dependencies) {
    if (!dependency.semaphore) {
      dependency.semaphore =
          ContextImpl::device().createSemaphoreUnique(vk::SemaphoreCreateInfo{});
    }
  }

  const auto commandbuffer_info =
      vk::CommandBufferAllocateInfo{}
          .setCommandPool(ContextImpl::instance().commandpool.get())
          .setLevel(vk::CommandBufferLevel::ePrimary)
          .setCommandBufferCount(1);

  for (auto &task : _tasks) {
    task.commandbuffer =
        std::move(ContextImpl::device()
                      .allocateCommandBuffersUnique(commandbuffer_info)
                      .front());
  }

  _finished_signal =
      ContextImpl::device().createSemaphoreUnique(vk::SemaphoreCreateInfo{});

  struct Batch {
    std::vector<vk::Semaphore> wait_semaphores;
    std::vector<vk::PipelineStageFlags> wait_dst_stage_flags;
    std::vector<vk::Semaphore> signal_semaphores;
  };

  std::vector<Batch> batches;
  std::vector<vk::SubmitInfo> submits;

  for (auto &task : _tasks) {
    task.commandbuffer.value().get().begin(vk::CommandBufferBeginInfo{});
    std::invoke(task.call, **task.commandbuffer);
    task.commandbuffer.value().get().end();

    batches.emplace_back();
    const auto parent_dependencies = get_parent_dependencies(task.id);
    for (Dependency *dependency : parent_dependencies) {
      batches.back().wait_semaphores.push_back(dependency->semaphore->get());
      batches.back().wait_dst_stage_flags.push_back(
          vk::PipelineStageFlagBits::eAllCommands);
    }

    const auto children_dependencies = get_child_dependencies(task.id);
    for (auto *dependency : children_dependencies) {
      batches.back().signal_semaphores.push_back(dependency->semaphore->get());
    }

    if (task.id == _end) {
      batches.back().signal_semaphores.push_back(
          _finished_signal.value().get());
    }

    submits.push_back(
        vk::SubmitInfo{}
            .setCommandBuffers(**task.commandbuffer)
            .setWaitSemaphores(batches.back().wait_semaphores)
            .setWaitDstStageMask(batches.back().wait_dst_stage_flags)
            .setSignalSemaphores(batches.back().signal_semaphores));
  }

  ContextImpl::instance().queue.submit(submits);
  return _finished_signal.value().get();
}

} // namespace bubble
