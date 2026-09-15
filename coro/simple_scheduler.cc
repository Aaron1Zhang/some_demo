#include <coroutine>
#include <iostream>
#include <queue>
#include <utility>

class task {
public:
    struct promise_type;
    using coro_handle = std::coroutine_handle<promise_type>;

    struct promise_type {
        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        task get_return_object() {return coro_handle::from_promise(*this); }
        void unhandled_exception() {}
        void return_void() {}

    };

    task(coro_handle h) : handle(h) {}
    ~task() {
        if (handle) {
            std::cout << "~task\n";
            handle.destroy();
        }
    }
    task(task&& other) : handle{std::exchange(other.handle, nullptr)} {
        //std::cout << "move cons\n";
    }
    task& operator=(task&& other) {
        std::cout << "move assign cons\n";              
        if (handle) {
            handle.destroy();
        }
        handle = other.handle;
        other.handle = nullptr;
        return *this;
    }
    task(const task&) = delete;
    task& operator=(const task&) = delete;

    bool resume() {
        if (handle && !handle.done()) {
            handle.resume();
            if (!handle.done()) {
                return true;
            }
            return false;
        }
        return false;
    }
    coro_handle get_handle() {
        return handle;
    }
    void destroy() {
        handle.destroy();
    }

private:
    coro_handle handle;
  
};

class scheduler {
public:
    scheduler() = default;
    void run() {
        while (!tasks.empty()) {
            auto t = std::move(tasks.front());
            tasks.pop();
            auto ret = t.resume();
            if (ret) {
                tasks.push(std::move(t));
            } 
        }
    }

    void enqueue(task&& t) {
        tasks.push(std::move(t));
    }

    auto schedule() {
        struct awaiter {
            scheduler* s;
            bool await_ready() {return false;}
            void await_suspend(task::coro_handle h) {
                // s->tasks.push(h);
            }
            void await_resume() {}
        };
        return awaiter{this};
    }
private:
  std::queue<task> tasks;
};


void spawn_task(task t, scheduler& s) {
    s.enqueue(std::move(t));
}

task task1(scheduler& s) {
    std::cout << "task1 ready to run\n";
    co_await std::suspend_always{};
    std::cout << "task1 continue to run\n";
    co_await std::suspend_always{};
    std::cout << "task1 end\n";
}

task task2(scheduler& s) {
    std::cout << "task2 ready to run\n";
    co_await std::suspend_always{};
    std::cout << "task2 continue to run\n";
    co_await std::suspend_always{};
    std::cout << "task2 end\n";
}

int main() {
    scheduler s;
    spawn_task(task1(s), s);
    spawn_task(task2(s), s);
    s.run();
    return 0;
}

