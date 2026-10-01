#include <sys/epoll.h>
#include <unistd.h>

#include <coroutine>
#include <exception>
#include <stdexcept>
#include <vector>

struct Event {
  int fd;
  uint32_t flags;
  std::coroutine_handle<void> handle;
};

class Epoll {
 public:
  Epoll() {
    epoll_fd_ = epoll_create1(0);
    if (epoll_fd_ < 0) {
      throw std::runtime_error("epoll_create1 failed ");
    }
  }
  ~Epoll() {
    if (epoll_fd_ > 0) {
      close(epoll_fd_);
    }
  }

  bool is_stop() const { return register_event_count_ == 1; }

  void add_event(const Event& event) {
    struct epoll_event ev;
    ev.events = event.flags;
    ev.data.fd = event.fd;
    ev.data.ptr = const_cast<void*>(&event.handle);
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, event.fd, &ev) == 0) {
      ++register_event_count_;
    }
  }

  void remove_event(const Event& event) {
    epoll_event ev{.events = event.flags};
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, event.fd, &ev) == 0) {
      --register_event_count_;
    }
  }

  std::vector<Event> get_event(int timeout /*ms*/) {
    std::vector<epoll_event> events;
    events.resize(register_event_count_);
    int num =
        epoll_wait(epoll_fd_, events.data(), register_event_count_, timeout);
    std::vector<Event> ret;

    for (size_t i = 0; i < num; ++i) {
    }

    return ret;
  }

 private:
  int epoll_fd_{0};
  uint32_t register_event_count_{0};
};
