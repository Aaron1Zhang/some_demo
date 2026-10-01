#pragma once

#include <iostream>

#include "eventloop.h"
#include "schedule_task.h"

template <typename Fut>
decltype(auto) run(Fut&& main) {
  auto t = schedule_task(std::forward<Fut>(main));
  get_event_loop().run_all();
  if constexpr (std::is_lvalue_reference_v<Fut>) {
    std::cout << "main is lvalue\n";
    return t.get_result();
  } else {
    // std::cout << "main is rvalue\n";
    return std::move(t).get_result();
  }
}
