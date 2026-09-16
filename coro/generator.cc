#include <iostream>
#include <coroutine>

class generator;
class gen_promise {
public:
    std::suspend_never initial_suspend() { return{}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    std::suspend_always yield_value(int t) {
        val = t;
        return {};
    }
    void return_void() {}
    void unhandled_exception() {}
    generator get_return_object();
    int get_val() { return val; }
private:
    int val;
};



class generator {
public:
    using promise_type = gen_promise;
    generator(std::coroutine_handle<promise_type> h) : handle(h) {}
    ~generator() {
        if (handle) {
            handle.destroy();
        }
    }
    bool finish() {
        if (handle) {
            return handle.done();
        }
        return true;
    }
    bool next() {
        if (handle) {
            handle.resume();
            if (handle.done()) {
                return false;
            }
            return true;
        }
        return false;
    }
    int value() {return handle.promise().get_val();}
private:
    std::coroutine_handle<promise_type> handle;
};

generator gen_promise::get_return_object() {return std::coroutine_handle<gen_promise>::from_promise(*this);}



generator Gen(int a, int b) {
    std::cout <<"enter Gen\n";
    for (int s = a; s <= b; ++s) {
        co_yield s;
    }
}


void Use(int a, int b) {
    auto generator = Gen(a,b);
    while (!generator.finish()) {
        std::cout << generator.value() << ',';
        generator.next();
    }
    std::cout << '\n';
}

int main() {
    std::cout <<"main start\n";
    Use(1,10);
    return 0;
}


