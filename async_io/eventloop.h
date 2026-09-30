#include <thread>
#include <vector>
#include <unordered_map>
#include <coroutine>
#include <queue>
#include <algorithm>

class eventloop {
public:
    using MSDuration = std::chrono::milliseconds;
    eventloop() {
        auto now = std::chrono::steady_clock::now();
        start_time_ = duration_cast<MSDuration>(now.time_since_epoch());
    }

     MSDuration time() {
        auto now = std::chrono::steady_clock::now();
        return duration_cast<MSDuration>(now.time_since_epoch()) - start_time_;
    }

    void call_soon(std::coroutine_handle<> handle) {
        ready_tasks_.push(handle);
    }
    template <typename Req, typename Period>
    void call_after(std::chrono::duration<Rep, Period> delay, std::coroutine_handle<> h) {
        call_at(time() + delay, h);
    }

private:
    using TimeHandle = std::pair<MSDuration, std::coroutine_handle<>>;
    std::queue<std::coroutine_handle<>> ready_tasks_;
    std::vector<TimeHandle> schedule_tasks_;
    MSDuration start_time_;

    template<typename Rep, typename Period>
    void call_at(std::chrono::duration<Rep, Period> when, std::coroutine_handle<> h) {
        schedule_.emplace_back(duration_cast<MSDuration>(when), h);
        std::ranges::push_heap(schedule_, std::ranges::greater{}, &TimerHandle::first);
    }

};

