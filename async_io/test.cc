#include <iostream>
#include <string_view>

#include "eventloop.h"
#include "runner.h"
#include "task.h"

Task<std::string_view> hello() {
  // std::cout << "ready to run hello\n";
  co_return "hello";
}

Task<std::string_view> world() { co_return "world"; }

Task<std::string> hello_world() {
  auto ret1 = co_await hello();
  auto ret2 = co_await world();
  std::string ret = std::string(ret1) + std::string(ret2);
  co_return ret;
  // co_return fmt::format("{} {}", co_await hello(), co_await world());
}

int main() {
  std::cout << "ret: " << run(hello_world()) << '\n';
  return 0;
}
