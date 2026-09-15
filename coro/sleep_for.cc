#include <iostream>
#include <thread>
#include <chrono>
#include <coroutine>
using namespace std::chrono_literals;
struct sleep_awaiter {
    sleep_awaiter(int64_t dur_ms): dur(dur_ms) {}
    bool await_ready() {return false;}
    void await_suspend(std::coroutine_handle<> h) {
        std::thread([h, d = this->dur]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(d));
            std::cout <<"timeout, will resume\n";
            h.resume();
        }).detach();
    }
    void await_resume() {}
    int64_t dur;
};

class timer_task {
public:
    struct promise_type;
    using coro_handle = std::coroutine_handle<promise_type>;
    struct promise_type {
        template <typename Rep, typename Period>
        sleep_awaiter await_transform(std::chrono::duration<Rep, Period> d) {
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(d).count();
            return sleep_awaiter{ms};
        }
        std::suspend_never initial_suspend() {return {};}
        std::suspend_always final_suspend() noexcept {return {};}
        timer_task get_return_object() {return coro_handle::from_promise(*this);}
        void return_void() {}
        void unhandled_exception() {}
    };
    

    timer_task(coro_handle h) : handle(h) {} 
    ~timer_task() {
        if (handle) {
            handle.destroy();
        }
    }
private:
    coro_handle handle;
};

timer_task sleep_for() {
    co_await 1s;
    //std::this_thread::thread_id();
}

int main() {
    auto t = sleep_for();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    return 0;
}
