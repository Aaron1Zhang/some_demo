#pragma once

#include <fcntl.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>
#include <utility>

class Pipe {
 public:
  Pipe() {
    if (pipe(fds_.data()) != 0) {
      const std::string msg =
          "Failed to create pipe, error=" + std::string(strerror(errno));
      throw std::runtime_error(msg);
    }
    for (auto& fd : fds_) {
      int flags = fcntl(fd, F_GETFL);
      flags |= O_NONBLOCK;
      fcntl(fd, F_SETFL, flags);
    }
  }
  ~Pipe() { close(); }

  Pipe(const Pipe& other) {
    fds_[0] = dup(other.fds_[0]);
    fds_[1] = dup(other.fds_[1]);
  }
  Pipe(Pipe&& other) { fds_ = std::exchange(other.fds_, {-1}); }
  Pipe& operator=(const Pipe& other) {
    if (std::addressof(other) != this) {
      fds_[0] = dup(other.fds_[0]);
      fds_[1] = dup(other.fds_[1]);
    }
    return *this;
  }

  Pipe& operator=(Pipe&& other) {
    if (std::addressof(other) != this) {
      fds_ = std::exchange(other.fds_, {-1});
    }
    return *this;
  }

  long write(const void* bytes, size_t n) {
    return ::write(write_fd(), bytes, n);
  }
  long read(void* buffer, size_t n) { return ::read(read_fd(), buffer, n); }

  int read_fd() const { return fds_[0]; }
  int write_fd() const { return fds_[1]; }
  void close() {
    if (fds_[0] != -1) {
      ::close(fds_[0]);
      fds_[0] = -1;
    }
    if (fds_[1] != -1) {
      ::close(fds_[1]);
      fds_[1] = -1;
    }
  }

 private:
  std::array<int, 2> fds_{-1};
};
