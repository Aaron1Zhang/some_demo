#include <liburing.h>

#include <cstdio>
#include <cstring>
#include <iostream>

int main()
{
    io_uring ring{};

    // 初始化 io_uring
    int ret = io_uring_queue_init(8, &ring, 0);
    if (ret < 0) {
        std::cerr << "io_uring_queue_init failed: "
                  << strerror(-ret) << '\n';
        return 1;
    }

    std::cout << "liburing works!\n";

    io_uring_queue_exit(&ring);

    return 0;
}
