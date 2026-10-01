#pragma once

#include <coroutine>
#include <exception>
#include <iostream>
#include <optional>
#include <variant>

template <typename T>
class Result {
 public:
  constexpr bool has_value() const {
    if (std::holds_alternative<std::monostate>(ret_)) {
      return false;
    }
    return true;
  }

  void set_exception(std::exception_ptr ex_ptr) { ret_ = ex_ptr; }
  void unhandled_exception() { ret_ = std::current_exception(); }

  template <typename R>
  void set_value(R&& r) {
    ret_.template emplace<T>(std::forward<R>(r));
  }
  template <typename R>
  void return_value(R&& r) {
    set_value(std::forward<R>(r));
  }

  decltype(auto) result() & {
    if (auto exception = std::get_if<std::exception_ptr>(&ret_)) {
      std::rethrow_exception(*exception);
    }
    if (auto res = std::get_if<T>(&ret_)) {
      return *res;
    }
    throw std::runtime_error("No value");
  }
  decltype(auto) result() && {
    if (auto exception = std::get_if<std::exception_ptr>(&ret_)) {
      std::rethrow_exception(*exception);
    }
    if (auto res = std::get_if<T>(&ret_)) {
      // std::cout << "result &&: " << *res << '\n';
      return std::move(*res);
    }
    throw std::runtime_error("No value");
  }

 private:
  std::variant<std::monostate, T, std::exception_ptr> ret_;
};

template <>
class Result<void> {
 public:
  constexpr bool has_value() const { return ret_.has_value(); }

  void return_void() { ret_.emplace(nullptr); }

  void set_exception(std::exception_ptr cur_ex) { ret_.emplace(cur_ex); }

  auto result() {
    if (ret_.has_value() && ret_.value() != nullptr) {
      std::rethrow_exception(ret_.value());
    }
  }

  void unhandled_exception() { ret_ = std::current_exception(); }

 private:
  std::optional<std::exception_ptr> ret_;
};
