#include <iostream>
#include <coroutine>
#include <queue>

class task {
public:
    struct promise_type;
    using coro_handle = std::coroutine_handle<promise_type>;
    struct promise_type {
        std::suspend_always initial_suspend() {return {};}
        std::suspend_always final_suspend() noexcept { return {};}
        task get_return_object() { return coro_handle::from_promise(*this);}
        void return_void() {}
        void unhandled_exception() {}
    };
    task(coro_handle h) : handle(h) {}
    coro_handle get_handle() {return handle;}

private:
    coro_handle handle;
};

class scheduler {
public:
    auto suspend() {
        return std::suspend_always{};
    }
    void run() {
        while (!tasks.empty()) {
            auto t = tasks.front();
            tasks.pop();
            t.resume();
            if (!t.done()) {
                tasks.push(t);
            } else {
                t.destroy();
            }
        }
    }
    void enqueue(std::coroutine_handle<> h) {
        tasks.push(h);
    }
private:
    std::queue<std::coroutine_handle<>> tasks;
};

task task1(scheduler& s) {
    std::cout << "task1 ready to run\n";
    co_await s.suspend();
    std::cout << "task1 continue to run\n";
    co_await s.suspend();
    std::cout << "task1 end\n";
}


task task2(scheduler& s) {
    std::cout << "task2 ready to run\n";
    co_await s.suspend();
    std::cout << "task2 continue to run\n";
    co_await s.suspend();
    std::cout << "task2 end\n";
}

int main() {
    scheduler s;
    s.enqueue(task1(s).get_handle());
    s.enqueue(task2(s).get_handle());
    s.run();
    std::cout << "main exit\n";
    return 0;
}

