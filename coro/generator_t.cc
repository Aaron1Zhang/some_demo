#include <iostream>
#include <coroutine>
#include <cassert>

template <typename T>
class generator;


template <typename T>
class gen_promise {
public:
    std::suspend_always initial_suspend() { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    generator<T> get_return_object();
    void unhandled_exception() {}
    void return_void() {}
    std::suspend_always yield_value(T val) {
        val_ = std::move(val);
        return {};
    }
    T get_val() {return val_;}
private:
    T val_;

};

template <typename T>
class generator {
public:
    using promise_type = gen_promise<T>;
    using Handle = std::coroutine_handle<promise_type>;
    generator(Handle h): handle(h) {}
    struct iterator {
        Handle handle{nullptr};
        iterator(Handle h) : handle(h) {}
        void get_next() {
            if (handle) {
                handle.resume();
                if (handle.done()) {
                    handle = nullptr;
                }
            }
        }
        T operator*() {
            assert(handle != nullptr);
            return handle.promise().get_val();
        }
        iterator operator++() {
            get_next();
            return *this;
        }
        bool operator== (const iterator&) const = default;
    };

    iterator begin() {
        if (!handle || handle.done()) {
            return iterator{nullptr};
        }
        iterator it{handle};
        it.get_next();
        return it;

    }
    iterator end() {
        return iterator{nullptr};
    }

private:
    std::coroutine_handle<promise_type> handle;
};

template <typename T>
generator<T> gen_promise<T>::get_return_object() { return std::coroutine_handle<gen_promise>::from_promise(*this);}


generator<int> gen_int(int max) {
    for (int i = 0; i < max; ++i) {
        co_yield i;
    }

}

int main() {
    std::cout <<"main s\n";
    for (auto i : gen_int(5)) {
        std::cout << i << ',';
    }
    std::cout << '\n';
    return 0;
}
