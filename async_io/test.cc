#include <iostream>
#include <string_view>

#include "eventloop.h"
#include "runner.h"
#include "task.h"

Task<std::string_view> hello() {
  // std::cout << "ready to run hello\n";
  co_return "hello";
}

// Task<std::string_view> world() { co_return "world"; }

int main() {
  std::cout << "ret: " << run(hello()) << '\n';
  return 0;
}
