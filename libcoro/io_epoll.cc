#include "io_epoll.hpp"

#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <sys/types.h>
#include <unistd.h>

#include "timer_handle.h"

using namespace std::chrono_literals;
namespace {

uint64_t encode_udata(bool keep_registered, bool is_cancel_event, void* udata) {
  // Pointers on 64 bit unix machines take up to 48 bit right now. So we have
  // some bits left to encode the boolean to indicate if this is a cancellation
  // event descriptor at the highest bit.
  return (((uint64_t)keep_registered) << 63) |
         (((uint64_t)is_cancel_event) << 62) |
         (reinterpret_cast<uintptr_t>(udata) & 0x3FFFFFFFFFFFFFFFULL);
}

/**
 * Decode the state that was encoded into the epoll events user data field.
 *
 * For details see documentation of `coro::detail::encode_udata(bool, bool,
 * void*)` above.
 */
std::tuple<bool, bool, void*> decode_udata(uint64_t encoded) {
  bool keep_registered = (bool)(encoded >> 63);
  bool is_cancel_event = (bool)((encoded >> 62) & 0x1);
  void* udata = reinterpret_cast<void*>(encoded & 0xFFFFFFFFFFFFULL);
  return std::make_tuple(keep_registered, is_cancel_event, udata);
}

}  // namespace

poll_status IoEpoll::event_to_poll_status(const Event& event) {
  if (event.events & static_cast<uint32_t>(poll_op::read)) {
    return poll_status::read;
  }
  if (event.events & static_cast<uint32_t>(poll_op::write)) {
    return poll_status::write;
  } else if (event.events & EPOLLERR) {
    return poll_status::error;
  } else if (event.events & EPOLLRDHUP || event.events & EPOLLHUP) {
    return poll_status::closed;
  }
  throw std::runtime_error{"invalid epoll state"};
}

IoEpoll::IoEpoll() : epoll_fd_(epoll_create1(EPOLL_CLOEXEC)) {}
IoEpoll::~IoEpoll() {
  if (epoll_fd_) {
    ::close(epoll_fd_);
    epoll_fd_ = -1;
  }
}

bool IoEpoll::watch_timer(const timer_handle& timer,
                          std::chrono::nanoseconds dur) {
  auto sec = std::chrono::duration_cast<std::chrono::seconds>(dur);
  dur -= sec;

  auto nano = std::chrono::duration_cast<std::chrono::nanoseconds>(dur);
  if (sec <= 0s) {
    sec = 0s;
    if (nano <= 0ns) {
      nano = 1ns;
    }
  }

  itimerspec ts{};
  ts.it_value.tv_sec = sec.count();
  ts.it_value.tv_nsec = nano.count();
  return timerfd_settime(timer.get_fd(), 0, &ts, nullptr) != -1;
}

bool IoEpoll::unwatch_timer(const timer_handle& timer) {
  itimerspec ts{};
  ts.it_value.tv_sec = 0;
  ts.it_value.tv_nsec = 0;
  return ::timerfd_settime(timer.get_fd(), 0, &ts, nullptr) != -1;
}

bool IoEpoll::watch(int fd, poll_op op, void* data, bool keep,
                    bool is_cancel_event) {
  Event event{};
  event.events = static_cast<uint32_t>(op) | EPOLLRDHUP;
  event.data.u64 = encode_udata(keep, is_cancel_event, data);
  if (!keep) {
    event.events |= EPOLLONESHOT;
  } else {
    event.events |= EPOLLET;
  }
  return epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &event) != -1;
}

bool IoEpoll::watch(PollInfo& pi) {
  watch(pi.fd_, pi.op_, static_cast<void*>(&pi), false, false);
  if (pi.cancael_trigger_.has_value()) {
    watch(pi.cancael_trigger_.value().native_handle(), poll_op::read,
          static_cast<void*>(&pi), false, true);
  }
  return true;
}

bool IoEpoll::unwatch(int fd, poll_op op) {
  return epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr) != -1;
}

bool IoEpoll::unwatch(PollInfo& pi) { return unwatch(pi.fd_, pi.op_); }

void IoEpoll::next_event(
    std::vector<std::pair<PollInfo*, poll_status>>& ready_events,
    std::chrono::milliseconds timeout) {
  auto ready_set = std::array<Event, max_events>{};
  int num = ::epoll_wait(epoll_fd_, ready_set.data(), ready_set.size(),
                         timeout.count());
  for (int i = 0; i < num; ++i) {
    auto [keep_reg, is_cancel_event, udata] =
        decode_udata(ready_set[i].data.u64);
    auto* pi = static_cast<PollInfo*>(udata);

    if (is_cancel_event) {
      ready_events.emplace_back(pi, poll_status::cancelled);
      if (!keep_reg) {
        unwatch(*pi);
      }
    } else {
      ready_events.emplace_back(pi,
                                IoEpoll::event_to_poll_status(ready_set[i]));
      if (pi->cancael_trigger_.has_value() && !keep_reg) {
        unwatch(pi->cancael_trigger_.value().native_handle(), poll_op::read);
      }
    }
  }
}
