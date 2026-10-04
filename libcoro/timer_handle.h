#pragma once

#include "io_epoll.hpp"

class timer_handle {
 public:
  timer_handle(const void* ptr, IoEpoll& notifier);
  int get_fd() const;
  const void* get_inner() const;

 private:
  int fd_;
  const void* timer_handle_ = nullptr;
};
