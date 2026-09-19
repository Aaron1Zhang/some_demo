class sync_wait_task {
public:
    struct promise_type {
        public:
            sync_wait_task get_return_object() {return std::coroutine_handle<promise_type>::from_promise(*this);}
            std::suspend_always initial_suspend() {return {};}
            auto final_suspend() noexcept {
                struct awaiter {
                    bool await_ready() noexcept {return false;}
                    void await_suspend(std::coroutine_handle<promise_type> handle) noexcept {
                        auto& promise = handle.promise();
                        std::lock_guard<std::mutex> lk(promise.mutex_);
                        promise.done_ = true;
                        promise.cv_.notify_one();
                    }
                    void await_resume() noexcept {}
                };
                return awaiter{};
            }
            void return_void() {}
            void unhandled_exception() {
                err_ = std::current_exception();
            }
            void wait() {
                std::unique_lock<std::mutex> lg(mutex_);
                cv_.wait(lg, [this]{
                    return done_;
                });

                if (err_) {
                    std::rethrow_exception(err_);
                }
            }
            std::mutex mutex_;
            std::condition_variable cv_;
            bool done_{false};
            std::exception_ptr err_;

    };
    using coro_handle = std::coroutine_handle<promise_type>;

    sync_wait_task(coro_handle h) : handle(h) {}
    sync_wait_task(sync_wait_task&& other) : handle(std::exchange(other.handle, {})) {
    }

    ~sync_wait_task() {
        if (handle) {
            handle.destroy();
        }
    }

    void wait() {
        handle.resume();
        handle.promise().wait();
    }
private:
    coro_handle handle;

};

inline void sync_wait(task<void>&& t) {
    [&]() -> sync_wait_task {
        co_await std::move(t);
    }().wait();
}
