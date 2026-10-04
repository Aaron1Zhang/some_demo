#pragma once

#include <atomic>
#include <coroutine>
#include <map>
#include <optional>

#include "poll.h"
#include "time.hpp"

struct PollInfo {
  using timed_events = std::multimap<coro_time::time_point, PollInfo*>;
  PollInfo() = default;
  ~PollInfo() = default;

  PollInfo(int fd, poll_op op) : fd_(fd), op_(op) {}
  PollInfo(const PollInfo&) = delete;
  PollInfo& operator=(const PollInfo&) = delete;
  PollInfo(PollInfo&&) = delete;
  PollInfo& operator=(PollInfo&&) = delete;

  struct Awaiter {
    PollInfo& self;
    bool await_ready() { return false; }
    auto await_suspend(std::coroutine_handle<> handle) {
      self.waiting_coro_ = handle;
      std::atomic_thread_fence(std::memory_order_release);
    }
    auto await_resume() { return self.status_; }
  };

  Awaiter operator co_await() { return Awaiter{*this}; }

  int fd_;
  poll_op op_;
  poll_status status_{poll_status::error};
  std::coroutine_handle<> waiting_coro_;
  bool processed_{false};
  std::optional<poll_stop_token> cancael_trigger_{std::nullopt};
};
