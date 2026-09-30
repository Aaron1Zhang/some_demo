#include <coroutine>
#include <iostream>
#include <utility>
#include "result.h"

struct NoWaitAtInitialSuspend {};
inline constexpr NoWaitAtInitialSuspend no_wait_at_initial_suspend;

template <typename R = void>
class Task {
public:
    struct promise_type;
    using coro_handle = std::coroutine_handle<promise_type>;
    struct promise_type : Result<R> {
        promise_type() = default;
        template <typename...Args>
        promise_type(NoWaitAtInitialSuspend, Args&&...): no_wait_at_initial_suspend_{true} {}
        template <typename Obj, typename...Args>
        promise_type(NoWaitAtInitialSuspend, Obj&&, Args&&...): no_wait_at_initial_suspend_{true} {}


        Task get_return_object() {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        auto initial_suspend() {
            if (no_wait_at_initial_suspend_) {
                return std::suspend_never{};
            }
            return std::suspend_always{};
        }

        struct final_awaiter {
            bool await_ready() noexcept { return false; }
            template <typename Promise>
            auto await_suspend(std::coroutine_handle<Promise> handle) noexcept {
                if (auto cont = handle.promise().continuation_) {
                    //todo eventloop
                }
            }
            auto await_resume() noexcept {}
        };
        auto final_suspend() noexcept  {
            return final_awaiter{};
        }
        
        bool no_wait_at_initial_suspend_{false};
        std::coroutine_handle<> continuation_;
    };

    explicit Task(coro_handle h) noexcept : handle_(h) {}
    Task(Task&& other) noexcept : handle_{std::exchange(other.handle_, nullptr)} {}
    ~Task() { destroy(); }
    bool valid() { return handle_ != nullptr; }
    bool done() { return handle_.done(); }
    decltype(auto) get_result() & {
        return handle_.promise().result();
    }
    decltype(auto) get_result() && {
        return std::move(handle_.promise()).result();
    }

    struct AwaiterBase {
        coro_handle self;
        bool await_ready() { 
            if (self) {
                return self.done();
            }
            return true;
        }
        void await_suspend(std::coroutine_handle<> cont) {
            self.promise().continuation_ = cont;
            // todo eventloop
        }
    };
    auto operator co_await() & {
        struct Awaiter : AwaiterBase {
            auto await_resume() {
                return AwaiterBase::self.promise().result();
            }
        };
        return Awaiter{handle_};
    }
    auto operator co_await() && {
        struct Awaiter : AwaiterBase {
            auto await_resume() {
                return std::move(AwaiterBase::self.promise()).result();
            }
        };
        return Awaiter{handle_};
    }



private:
    void destroy() {
        if (auto handle = std::exchange(handle_, nullptr)) {
            handle.destroy();
        }
    }
    coro_handle handle_;


};

