#include <iostream>

template <typename T>
class manual_lifetime {
public:
  manual_lifetime() noexcept {}
  ~manual_lifetime() noexcept {}

  template <typename...Args>
  void construct(Args&&... args) {
    new (static_cast<void*>(std::addressof(val))) T(static_cast<Args&&>(args)...);
  }
  void destruct() {
    val.~T();
  }
  T& get() & {return val;}
  T&& get() && {return (T&&)val;}
  const T& get() const & {return val;}

private:
  union {
    T val;
  };
};

template <typename T>
class manual_lifetime<T&> {
public:
  manual_lifetime() : ptr{nullptr} {}
  ~manual_lifetime() {}

  void construct(const T& val) {
    ptr = std::addressof(val);
  }

  void destruct() {ptr = nullptr;}
  T& get() & {return *ptr;}

private:
  T* ptr;
};

//需要注意的是这个类只是保存的引用，不负责对象的生命周期，一定不能传入临时对象
// 也就是由调用者确保被引用对象的生命周期
template <typename T>
class manual_lifetime<T&&> {
public:
  manual_lifetime() : ptr{nullptr} {}
  ~manual_lifetime() {}

  void construct(T&& val) {
    ptr = std::addressof(val);
  }

  void destruct() {ptr = nullptr;}
  T&& get() const {return *ptr;}


private:
  T* ptr;
};


template <>
class manual_lifetime<void> {
public:
  void get() {}
  void construct() {}
  void destruct() {}
};
