#pragma once

#include <coroutine>
#include <utility>
#include <vector>

#include "result.h"

template <typename R = void>
class Task {
 public:
  struct promise_type;
  using coro_handle = std::coroutine_handle<promise_type>;
  struct FinalAwaiter {
    bool await_ready() noexcept { return false; }
    template <typename Promise>
    auto await_suspend(std::coroutine_handle<Promise> self) noexcept {
      if (auto cont = self.promise().continuation) {
        return cont;
      } else {
        return std::noop_coroutine();
      }
    }
    void await_resume() noexcept {}
  };

  struct promise_type : Result<R> {
    auto initial_suspend() { return std::suspend_always{}; }
    auto final_suspend() { return FinalAwaiter{}; }
    Task get_return_object() { return Task{coro_handle::from_promise(*this)}; }
    std::coroutine_handle<> continuation{nullptr};
  };

  explicit Task(coro_handle handle) : handle_{handle} {}
  ~Task() {
    if (handle_) {
      handle_.destroy();
    }
  }
  Task(const Task&) = delete;

  Task(Task&& other) : handle_{std::exchange(other.handle_, nullptr)} {}

  struct AwaiterBase {
    coro_handle self;
    bool await_ready() noexcept {
      if (self) {
        return self.done();
      }
      return true;
    }
    std::coroutine_handle<> await_suspend(
        std::coroutine_handle<> cont) noexcept {
      self.promise().continuation = cont;
      return self;
    }
  };
  auto operator co_await() & {
    struct Awaiter : AwaiterBase {
      decltype(auto) await_resume() & {
        return AwaiterBase::self.promise().get_value();
      }
    };
    return Awaiter{handle_};
  }

  auto operator co_await() && {
    struct Awaiter : AwaiterBase {
      decltype(auto) await_resume() && {
        return std::move(AwaiterBase::self.promise()).get_value();
      }
    };
    return Awaiter{};
  }

  decltype(auto) value() & { return handle_.promise().get_value(); }
  decltype(auto) value() && { return std::move(handle_.promise()).get_value(); }

  bool ready() const { return handle_ == nullptr || handle_.done(); }

  bool resume() {
    if (handle_ && !handle_.done()) {
      handle_.resume();
      return !handle_.done();
    }
    return false;
  }
  bool destroy() {
    if (handle_ != nullptr) {
      handle_.destroy();
      handle_ = nullptr;
      return true;
    }

    return false;
  }

  promise_type& promise() & { return handle_.promise(); }
  const promise_type& promise() const& { return handle_.promise(); }
  promise_type&& promise() && { return std::move(handle_.promise()); }
  coro_handle handle() { return handle_; }

 private:
  coro_handle handle_;
};
