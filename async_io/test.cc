#include <iostream>
#include <string_view>

#include "eventloop.h"
#include "runner.h"
#include "task.h"

Task<std::string_view> hello() { co_return "hello"; }

// Task<std::string_view> world() { co_return "world"; }

int main() {
  std::cout << "test\n";
  std::cout << "ret: " << run(hello());
  return 0;
}
