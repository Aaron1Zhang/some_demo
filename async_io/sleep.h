#include <iostream>
#include "task.h"

template <typename Dur>
struct SleepAwaiter {
    SleepAwaiter(Dur dur) : delay_{dur} {}
    bool await_ready() {
        return false;
    }
    void await_suspend(std::coroutine_handle<> handle) {
        //eventloop().call_after(ms, handle);

    }   

    void await_resume() {}

private:
    Dur delay_;
};

template <typename Rep, typename Period>
Task<void> sleep_for(NoWaitAtInitialSuspend, std::chrono::duration<Rep, Period> delay) {
    co_await SleepAwaiter{delay};
}


template <typename Rep, typename Period>
Task<void> sleep_for(std::chrono::duration<Rep, Period> delay) {
    return sleep_for(no_wait_at_initial_suspend, delay);
}
