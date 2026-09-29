template <typename R = void>
class Task {
public:
    struct promise_type;
    using coro_handle = std::coroutine_handle<promise_type>;


    struct promise_type : Result<R> {
        auto initial_suspend() {
            if (no_wait_at_initial_suspend_) {
                return std::suspend_never{};
            }
            return std::suspend_always{};
        }

        struct final_awaiter {
            bool await_ready() {return false;}
            template <typename Promise>
            auto await_suspend(std::coroutine_handle<Promise> handle) {
                if (auto cont = handle.promise().continuation_) {
                    //todo eventloop
                }
            }
            auto await_resume() {}
        };
        auto final_suspend() {
            return final_awaiter{};
        }
        void unhandle_exception() {}

        template <typename...Args>
        promise_type(NoWaitAtInitialSuspend, Args&&...): no_wait_at_initial_suspend_{true} {}

        template <typename Obj, typename...Args>
        promise_type(NoWaitAtInitialSuspend, Obj&&, Args&&...): no_wait_at_initial_suspend_{true} {}
        
        bool no_wait_at_initial_suspend_{false};
    };


    explicit Task(coro_handle h) : handle_(h) {}

private:
    coro_handle handle_;
    

};

