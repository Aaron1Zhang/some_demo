#pragma once

#include "task.h"

template <typename Task>
struct ScheduleTask {
  template <typename Fut>
  explicit ScheduleTask(Fut&& fut) : task_(std::forward<Fut>(fut)) {
    if (task_.valid() && !task_.done()) {
      get_event_loop().call_soon(task_.handle_);
    }
  }

  decltype(auto) operator co_await() const& {
    return task_.operator co_await();
  }

  decltype(auto) operator co_await() const&& {
    return task_.operator co_await();
  }

  decltype(auto) get_result() & { return task_.get_result(); }
  decltype(auto) get_result() && { return std::move(task_).get_result(); }
  bool valid() const { return task_.valid(); }
  bool done() const { return task_.done(); }

 private:
  Task task_;
};

template <typename Fut>
ScheduleTask(Fut&&) -> ScheduleTask<Fut>;

template <typename Fut>
ScheduleTask<Fut> schedule_task(Fut&& fut) {
  return ScheduleTask{std::forward<Fut>(fut)};
}