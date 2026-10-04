#pragma once

#include <sys/timerfd.h>

#include <chrono>
#include <ctime>
#include <vector>

#include "poll.h"
#include "poll_info.h"

class timer_handle;

class IoEpoll {
  using Event = struct ::epoll_event;

  static constexpr size_t max_events = 16;
  int epoll_fd_;
  static poll_status event_to_poll_status(const Event& event);

 public:
  IoEpoll();
  ~IoEpoll();

  bool watch_timer(const timer_handle& timer, std::chrono::nanoseconds dur);
  bool unwatch_timer(const timer_handle& timer);
  bool watch(int fd, poll_op op, void* data, bool keep = false,
             bool is_cancel_event = false);
  bool watch(PollInfo& pi);
  bool unwatch(int fd, poll_op op);
  bool unwatch(PollInfo& pi);

  void next_event(std::vector<std::pair<PollInfo*, poll_status>>& ready_events,
                  std::chrono::milliseconds timeout);
};
