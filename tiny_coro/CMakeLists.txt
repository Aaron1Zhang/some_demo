cmake_minimum_required(VERSION 3.10)

project(io_uring_demo)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(io_uring_demo iouring_test.cc)
target_link_libraries(io_uring_demo PRIVATE uring)

add_executable(file_demo iouring_basic_use.cc)
target_link_libraries(file_demo PRIVATE uring)

add_executable(eventfd_test iouring_eventfd.cc)
target_link_libraries(eventfd_test PRIVATE uring)