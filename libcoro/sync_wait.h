#pragma once

#include <atomic>
#include <condition_variable>
#include <coroutine>
#include <mutex>
#include <utility>

#include "awaitable_traits.h"
#include "result.h"

class sync_wait_event {
 public:
  sync_wait_event(bool initially_set = false) : set_{initially_set} {}
  sync_wait_event(const sync_wait_event&) = delete;
  sync_wait_event(sync_wait_event&&) = delete;
  auto operator=(const sync_wait_event&) -> sync_wait_event& = delete;
  auto operator=(sync_wait_event&&) -> sync_wait_event& = delete;
  ~sync_wait_event() = default;

  void set() noexcept {
    std::lock_guard lk(mutex_);
    set_.exchange(true);
    cv_.notify_one();
  }
  void reset() noexcept { set_.exchange(false); }
  void wait() noexcept {
    std::unique_lock lk(mutex_);
    cv_.wait(lk, [this]() { return set_.load(); });
  }

 private:
  std::mutex mutex_;
  std::condition_variable cv_;
  std::atomic<bool> set_{false};
};

template <typename R = void>
class SyncWaitTask {
 public:
  struct promise_type;
  using coro_handle = std::coroutine_handle<promise_type>;
  SyncWaitTask(coro_handle handle) : handle_{handle} {}
  ~SyncWaitTask() {
    if (handle_) {
      handle_.destroy();
    }
  }
  SyncWaitTask(const SyncWaitTask&) = delete;
  SyncWaitTask(SyncWaitTask&& other)
      : handle_{std::exchange(other.handle_, nullptr)} {}

  struct promise_type : Result<R> {
    sync_wait_event* ev_;
    auto initial_suspend() noexcept { return std::suspend_always{}; }
    auto final_suspend() {
      struct final_awaiter {
        bool await_ready() { return false; }
        void await_suspend(coro_handle handle) { handle.promise().ev_->set(); }
        void await_resume() {}
      };
      return final_awaiter{};
    }
    auto get_return_object() { return coro_handle::from_promise(*this); }
    void start(sync_wait_event* event) {
      ev_ = event;
      coro_handle::from_promise(*this).resume();
    }
  };

  promise_type& promise() & { return handle_.promise(); }
  promise_type&& promise() && { return std::move(handle_.promise()); }
  const promise_type& promise() const& { return handle_.promise(); }

  decltype(auto) value() & { return handle_.promise().get_value(); }
  decltype(auto) value() && { return std::move(handle_.promise()).get_value(); }

 private:
  coro_handle handle_;
};

template <typename Awaitable, typename R = typename awaitable_traits<
                                  Awaitable>::awaiter_return_type>
static SyncWaitTask<R> make_sync_wait_task(Awaitable&& awaitable) {
  if constexpr (std::is_void_v<R>) {
    co_await std::forward<Awaitable>(awaitable);
    co_return;
  } else {
    co_return co_await std::forward<Awaitable>(awaitable);
  }
}

template <typename Awaitable, typename R = typename awaitable_traits<
                                  Awaitable>::awaiter_return_type>
decltype(auto) sync_wait(Awaitable&& awaitable) {
  sync_wait_event event{};
  auto task = make_sync_wait_task(std::forward<Awaitable>(awaitable));
  task.promise().start(&event);
  event.wait();
  if constexpr (std::is_void_v<R>) {
    task.value();
    return;
  } else if constexpr (std::is_move_assignable_v<R>) {
    return std::move(task).value();
  } else {
    return task.value();
  }
}