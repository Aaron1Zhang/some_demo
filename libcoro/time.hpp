#pragma once

#include <chrono>

namespace coro_time {

using clock = std::chrono::steady_clock;
using time_point = clock::time_point;

}  // namespace coro_time
