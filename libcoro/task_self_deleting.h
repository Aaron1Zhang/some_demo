#pragma once

#include <atomic>
#include <coroutine>

#include "task.h"

class TaskSelfDeleting {
 public:
  struct promise_type {
    promise_type() = default;
    ~promise_type() = default;

    auto initial_suspend() noexcept { return std::suspend_always{}; }
    auto final_suspend() noexcept {
      if (executor_size_ != nullptr) {
        executor_size_->fetch_sub(1);
      }
      return std::suspend_never{};
    }
    TaskSelfDeleting get_return_object() { return TaskSelfDeleting(*this); }
    void unhandled_exception() noexcept {}
    void return_void() noexcept {}

    void executor_size(std::atomic<size_t>& size) noexcept {
      executor_size_ = &size;
    }

   private:
    std::atomic<size_t>* executor_size_{nullptr};
  };

  TaskSelfDeleting(promise_type& promise) : promise_{&promise} {}
  ~TaskSelfDeleting() {}
  promise_type& promise() { return *promise_; }
  std::coroutine_handle<promise_type> handle() {
    return std::coroutine_handle<promise_type>::from_promise(*promise_);
  }

  bool resume() {
    auto h = handle();
    if (!h.done()) {
      h.resume();
    }
    return !h.done();
  }

 private:
  promise_type* promise_;
};

TaskSelfDeleting make_self_deleting(Task<void>&& task) {
  co_await task;
  co_return;
}
