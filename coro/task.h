#include <iostream>
#include <thread>
#include <chrono>
#include <coroutine>
#include <queue>
#include <utility>
#include <exception>
#include <iostream>

#include <memory>

template<typename T>
struct manual_lifetime {
public:
    manual_lifetime() noexcept {}
    ~manual_lifetime() noexcept {}

    template<typename... Args>
    void construct(Args&&... args) {
        ::new (static_cast<void*>(std::addressof(value))) T(static_cast<Args&&>(args)...);
    }

    void destruct() {
        value.~T();
    }

    T& get() & { return value; }
    const T& get() const & { return value; }
    T&& get() && { return (T&&)value; }
    const T&& get() const && { return (const T&&)value; }

private:
  union { T value; };
};

template<typename T>
struct manual_lifetime<T&> {
    manual_lifetime() noexcept : ptr(nullptr) {}
    ~manual_lifetime() {}

    void construct(T& value) noexcept {
        ptr = std::addressof(value);
    }
    void destruct() noexcept {
        ptr = nullptr;
    }

    T& get() const noexcept { return *ptr; }

private:
    T* ptr;
};

template<typename T>
struct manual_lifetime<T&&> {
    manual_lifetime() noexcept : ptr(nullptr) {}
    ~manual_lifetime() {}

    void construct(T&& value) noexcept {
        ptr = std::addressof(value);
    }
    void destruct() noexcept {
        ptr = nullptr;
    }

    T&& get() const noexcept { return *ptr; }

private:
    T* ptr;
};

template<>
struct manual_lifetime<void> {
    void construct() noexcept {}
    void destruct() noexcept {}
    void get() const noexcept {}
};


template <typename T>
class task;

template <typename T>
class task_promise {
public:
    task_promise() {}
    ~task_promise() {
            clear();
    }

    std::suspend_always initial_suspend() {return {};}
    auto final_suspend() noexcept {
        struct final_awaiter {
            bool await_ready() {return false;}
            auto await_suspend(std::coroutine_handle<task_promise<T>> self) {
                auto cont =  self.promise().get_continuation();
                return cont;
            }
            void await_resume() {}
        };
        return final_awaiter{};
    }
    task<T> get_return_object();
    void unhandle_exception() {
        clear();
        ex_ptr_.construct(std::current_exception());
        state_ = state::error;
    }
    
    template <typename U, std::enable_if_t<std::is_convertible_v<U, T> ,int> = 0>
    void return_value(U&& val) {
        clear();
        val_.construct((U&&)val);
        state_ = state::value;
    }

    T get_value() {
       if (state_ == state::error) {
        std::rethrow_exception(std::move(ex_ptr_).get());
       }
       return std::move(val_).get();
    }

    void set_continuation(std::coroutine_handle<> cont) {
        continuation_ = cont;
    }

    auto get_continuation() {return continuation_;}
private:
    union {
        manual_lifetime<T> val_;
        manual_lifetime<std::exception_ptr> ex_ptr_;
    };
    enum class state: uint8_t {
        empty,
        value,
        error
    };
    void clear() {
       auto prev_state = std::exchange(state_, state::empty);
        switch(prev_state) {
            case state::empty:
                break;
            case state::value:
                val_.destruct();
                break;
            case state::error:
                ex_ptr_.destruct();
                break;
        }
    }
    state state_;
    std::coroutine_handle<> continuation_;
};

template <>
class task_promise<void> {
public:
    task_promise() {}
    ~task_promise() {clear();}
    task<void> get_return_object() noexcept;
    std::suspend_always initial_suspend() {return {};}
    auto final_suspend() {
        struct awaiter {
            bool await_ready() { return false; }
            auto await_suspend(std::coroutine_handle<task_promise> h) {
                return h.promise().continuation_;
            }
            void await_resume() {}
        };
        return awaiter{};
    }
   
    void return_value() {
        clear();
        val_.construct();
        state_ = state::value;
    }
    void unhandled_exception() {
        clear();
        ex_ptr_.construct(std::current_exception());
        state_ = state::error;
    }
    auto get_continuation() {return continuation_;}
private:
    void clear() {
        auto prev_state = std::exchange(state_, state::empty);
        switch(prev_state) {
            case state::empty:
                break;
            case state::value:
                val_.destruct();
                break;
            case state::error:
                ex_ptr_.destruct();
                break;
        }
    }
    enum class state : uint8_t {
        empty,
        value,
        error
    };
    union {
        manual_lifetime<void> val_;
        manual_lifetime<std::exception_ptr> ex_ptr_;
    };
    
    state state_;
    std::coroutine_handle<> continuation_;
};


template <typename T>
class task {
public:
    using promise_type = task_promise<T>;
    using coro_handle = std::coroutine_handle<promise_type>;
    explicit task(coro_handle h): handle(h) {}
    task(task&& t) noexcept: handle(std::exchange(t.handle, {}))
    {}

    ~task() {
        if (handle) {
            handle.destroy();
        }
    }


    struct awaiter {
        coro_handle self;
        bool await_ready() {return false;}
        auto await_suspend(std::coroutine_handle<> cont) {
            self.promise().set_continuation(cont);
            return self;
        }
        auto await_resume() {
            return self.promise().get_value();
        }
    };
    auto operator co_await() {
        return awaiter{handle};
    }
private:
    coro_handle handle;

};

template<typename T>
task<T> task_promise<T>::get_return_object() { return task<T>{std::coroutine_handle<task_promise<T>>::from_promise(*this)}; }

inline
task<void> task_promise<void>::get_return_object() noexcept {
    return task<void>{std::coroutine_handle<task_promise<void>>::from_promise(*this)};
}

int main() {
    return 0;
}
