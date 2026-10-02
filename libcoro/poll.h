#pragma once

#include <sys/epoll.h>
#include <unistd.h>

#include <array>
#include <iostream>
#include <string>

enum class poll_op : uint64_t {
  /// Poll for read operations.
  read = EPOLLIN,
  /// Poll for write operations.
  write = EPOLLOUT,
  /// Poll for read and write operations.
  read_write = EPOLLIN | EPOLLOUT
};

inline auto poll_op_readable(poll_op op) -> bool {
  return (static_cast<int64_t>(op) & static_cast<int64_t>(poll_op::read));
}

inline auto poll_op_writeable(poll_op op) -> bool {
  return (static_cast<int64_t>(op) & static_cast<int64_t>(poll_op::write));
}

enum class poll_status {
  /// The poll operation was was successful with a read-event.
  read,
  /// The poll operation was was successful with a write-event.
  write,
  /// The poll operation timed out.
  timeout,
  /// The file descriptor had an error while polling.
  error,
  /// The file descriptor has been closed by the remote or an internal
  /// error/close.
  closed,
  /// The poll operation was cancelled by a 'poll_stop_source'.
  cancelled,
};

static const std::string poll_unknown{"unknown"};

static const std::string poll_op_read{"read"};
static const std::string poll_op_write{"write"};
static const std::string poll_op_read_write{"read_write"};

const std::string& to_string(poll_op op) {
  switch (op) {
    case poll_op::read:
      return poll_op_read;
    case poll_op::write:
      return poll_op_write;
    case poll_op::read_write:
      return poll_op_read_write;
    default:
      return poll_unknown;
  }
}

static const std::string poll_status_read{"read"};
static const std::string poll_status_write{"write"};
static const std::string poll_status_timeout{"timeout"};
static const std::string poll_status_error{"error"};
static const std::string poll_status_closed{"closed"};

const std::string& to_string(poll_status status) {
  switch (status) {
    case poll_status::read:
      return poll_status_read;
    case poll_status::write:
      return poll_status_write;
    case poll_status::timeout:
      return poll_status_timeout;
    case poll_status::error:
      return poll_status_error;
    case poll_status::closed:
      return poll_status_closed;
    default:
      return poll_unknown;
  }
}
