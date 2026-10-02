#pragma once

#include <atomic>
#include <condition_variable>
#include <coroutine>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

#include "task.h"
#include "task_self_deleting.h"

class ThreadPool {
 public:
  class ScheduleAwaiter {
    friend class ThreadPool;
    explicit ScheduleAwaiter(ThreadPool& tp) noexcept : tp_(tp) {}

   public:
    bool await_ready() noexcept { return false; }
    void await_suspend(std::coroutine_handle<> handle) noexcept {
      tp_.schedule_impl(handle);
    }
    void await_resume() noexcept {}

   private:
    ThreadPool& tp_;
  };

  struct Options {
    uint32_t thread_count = std::thread::hardware_concurrency() / 2;
    std::function<void(size_t)> on_thread_start_functor = nullptr;
    std::function<void(size_t)> on_thread_stop_functor = nullptr;
  };

  explicit ThreadPool(Options&& opt) : opt_(std::move(opt)) {
    threads_.reserve(opt_.thread_count);
  }
  ~ThreadPool() { shutdown(); }
  static std::unique_ptr<ThreadPool> make_unique(Options opt) {
    auto tp = std::make_unique<ThreadPool>(std::move(opt));
    for (size_t i = 0; i < tp->opt_.thread_count; ++i) {
      tp->threads_.emplace_back([tp = tp.get(), i]() { tp->executor(i); });
    }

    return tp;
  }
  ThreadPool(const ThreadPool&) = delete;
  ThreadPool(ThreadPool&&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;
  ThreadPool& operator=(ThreadPool&&) = delete;

 public:
  ScheduleAwaiter schedule() {
    task_num_.fetch_and(1);
    if (!shutdown_requested_.load()) {
      return ScheduleAwaiter{*this};
    } else {
      throw std::runtime_error("tp is shutdown, unable to schedule new task");
    }
  }
  bool spawn_detached(Task<void>&& task) {
    task_num_.fetch_add(1);
    auto wrapper_task = make_self_deleting(std::move(task));
    wrapper_task.promise().executor_size(task_num_);
    return resume(wrapper_task.handle());
  }

  auto spawn_joinable() {}

  template <typename R>
  Task<R> schedule(Task<R> task) {
    co_await schedule();
    co_return co_await task;
  }

  bool resume(std::coroutine_handle<> handle) {
    if (handle == nullptr || handle.done()) {
      return false;
    }
    task_num_.fetch_add(1);
    if (shutdown_requested_.load()) {
      task_num_.fetch_sub(1);
      return false;
    }
    schedule_impl(handle);
    return true;
  }
  ScheduleAwaiter yield() { return schedule(); }

  bool is_shutdown() const { return shutdown_requested_.load(); }
  size_t size() { return task_num_.load(); }
  size_t queue_size() const { return task_queue_.size(); }

 private:
  Options opt_;
  std::vector<std::thread> threads_;
  std::mutex mutex_;
  std::condition_variable_any cv_;
  std::deque<std::coroutine_handle<>> task_queue_;
  std::atomic<size_t> task_num_{0};
  std::atomic<bool> shutdown_requested_{false};

  void schedule_impl(std::coroutine_handle<> handle) {
    if (handle == nullptr || handle.done()) {
      return;
    }
    std::lock_guard lk(mutex_);
    task_queue_.emplace_back(handle);
    cv_.notify_one();
  }

  void shutdown() {
    if (shutdown_requested_.exchange(true) == false) {
      {
        std::unique_lock<std::mutex> lk{mutex_};
        cv_.notify_all();
      }
      for (auto& thread : threads_) {
        if (thread.joinable()) {
          thread.join();
        }
      }
    }
  }

  void executor(size_t idx) {
    if (opt_.on_thread_start_functor != nullptr) {
      opt_.on_thread_start_functor(idx);
    }

    while (!shutdown_requested_.load()) {
      std::unique_lock lk{mutex_};
      cv_.wait(lk, [&] {
        return !task_queue_.empty() || shutdown_requested_.load();
      });

      if (shutdown_requested_.load()) {
        return;
      }

      auto t = task_queue_.front();
      task_queue_.pop_front();
      lk.unlock();

      t.resume();
      task_num_.fetch_sub(1);
    }

    // process left task
    while (task_num_.load() > 0) {
      std::unique_lock lk{mutex_};
      cv_.wait(lk, [&] { return !task_queue_.empty(); });
      if (task_queue_.empty()) {
        break;
      }
      auto handle = task_queue_.front();
      task_queue_.pop_front();
      lk.unlock();

      handle.resume();
      task_num_.fetch_sub(1);
    }

    if (opt_.on_thread_stop_functor) {
      opt_.on_thread_stop_functor(idx);
    }
  }
};