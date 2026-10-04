#include "timer_handle.h"

#include "io_epoll.hpp"

timer_handle::timer_handle(const void* ptr, IoEpoll& notifier)
    : fd_(timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC)),
      timer_handle_(ptr) {}

int timer_handle::get_fd() const { return fd_; }

const void* timer_handle::get_inner() const { return timer_handle_; }
