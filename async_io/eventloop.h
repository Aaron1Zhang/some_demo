#pragma once

#include <algorithm>
#include <chrono>
#include <coroutine>
#include <optional>
#include <queue>
#include <ranges>
#include <thread>
#include <unordered_map>
#include <vector>

// #include "epoll.h"

class EventLoop {
 public:
  using MSDuration = std::chrono::milliseconds;
  EventLoop() {
    auto now = std::chrono::steady_clock::now();
    start_time_ = duration_cast<MSDuration>(now.time_since_epoch());
  }

  MSDuration time() {
    auto now = std::chrono::steady_clock::now();
    return duration_cast<MSDuration>(now.time_since_epoch()) - start_time_;
  }

  void call_soon(std::coroutine_handle<> handle) { ready_tasks_.push(handle); }
  template <typename Rep, typename Period>
  void call_after(std::chrono::duration<Rep, Period> delay,
                  std::coroutine_handle<> h) {
    call_at(time() + delay, h);
  }

  void run_all() {
    while (!is_stopped()) {
      run_once();
    }
  }

 private:
  using TimerHandle = std::pair<MSDuration, std::coroutine_handle<>>;
  // Epoll poller_;
  std::queue<std::coroutine_handle<>> ready_tasks_;
  std::vector<TimerHandle> schedule_tasks_;
  MSDuration start_time_;

  template <typename Rep, typename Period>
  void call_at(std::chrono::duration<Rep, Period> when,
               std::coroutine_handle<> h) {
    schedule_tasks_.emplace_back(duration_cast<MSDuration>(when), h);
    std::ranges::push_heap(schedule_tasks_, std::ranges::greater{},
                           &TimerHandle::first);
  }

  bool is_stopped() { return ready_tasks_.empty() && schedule_tasks_.empty(); }
  void run_once() {
    std::optional<MSDuration> timeout;

    if (!ready_tasks_.empty()) {
      timeout.emplace(0);
    } else if (!schedule_tasks_.empty()) {
      auto&& [when, _] = schedule_tasks_[0];
      timeout = std::max(when - time(), MSDuration(0));
    }

    // auto event_lists =
    //     poller_.get_event(timeout.has_value() ? timeout->count() : -1);
    // std::cout << "event_list size: " << event_lists.size() << '\n';
    // for (auto&& event : event_lists) {
    //   ready_tasks_.push(event.handle);
    // }

    auto end_time = time();
    while (!schedule_tasks_.empty()) {
      auto&& [when, handle] = schedule_tasks_[0];
      if (when >= end_time) {
        break;
      }
      ready_tasks_.push(handle);
      std::ranges::pop_heap(schedule_tasks_, std::ranges::greater{},
                            &TimerHandle::first);
      schedule_tasks_.pop_back();
    }

    for (size_t i = 0; i < ready_tasks_.size(); ++i) {
      auto handle = ready_tasks_.front();
      ready_tasks_.pop();
      handle.resume();
    }
  }
};

EventLoop& get_event_loop() {
  static EventLoop loop;
  return loop;
}
