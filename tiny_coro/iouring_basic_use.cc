#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <liburing.h>

#define BUFFER_SIZE 4096
#define QUEUE_DEPTH 1

int main() {
    struct io_uring ring;
    struct io_uring_sqe *sqe;   // Submission Queue Entry
    struct io_uring_cqe *cqe;   // Completion Queue Entry
    char buf[BUFFER_SIZE];
    int fd, ret;

    // 1. Spin up the ring — depth 1 means 1 in-flight op at a time
    ret = io_uring_queue_init(QUEUE_DEPTH, &ring, 0);
    if (ret < 0) {
        fprintf(stderr, "Queue init failed: %s\n", strerror(-ret));
        return 1;
    }

    // 2. Open file (still a regular syscall — totally fine here)
    fd = open("hello.txt", O_RDONLY);
    if (fd < 0) { perror("open"); return 1; }

    // 3. Grab a free slot in the SQ
    sqe = io_uring_get_sqe(&ring);

    // 4. Describe the operation — read from fd into buf at offset 0
    io_uring_prep_read(sqe, fd, buf, BUFFER_SIZE, 0);

    // 5. Tag it — this comes back in cqe->user_data so you know what finished
    io_uring_sqe_set_data(sqe, buf);

    // 6. Submit — THE one syscall. Everything queued goes in one shot.
    ret = io_uring_submit(&ring);
    if (ret < 0) {
        fprintf(stderr, "Submit failed: %s\n", strerror(-ret));
        return 1;
    }

    // 7. Block until at least one CQE is available
    ret = io_uring_wait_cqe(&ring, &cqe);
    if (ret < 0) {
        fprintf(stderr, "Wait failed: %s\n", strerror(-ret));
        return 1;
    }

    // 8. cqe->res = bytes read on success, negative errno on failure
    if (cqe->res < 0) {
        fprintf(stderr, "Read error: %s\n", strerror(-cqe->res));
    } else {
        buf[cqe->res] = '\0';
        printf("Read %d bytes:\n%s\n", cqe->res, buf);
    }

    // 9. Tell the kernel you've consumed this CQE — advances the CQ head
    io_uring_cqe_seen(&ring, cqe);

    close(fd);
    io_uring_queue_exit(&ring);
    return 0;
}
