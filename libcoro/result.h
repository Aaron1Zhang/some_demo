#pragma once

#include <exception>
#include <optional>
#include <stdexcept>
#include <variant>

template <typename R = void>
class Result {
 public:
  Result() = default;

  bool has_value() {
    if (std::holds_alternative<std::monostate>(val_)) {
      return false;
    }
    return true;
  }

  void unhandled_exception() { val_ = std::current_exception(); }

  template <typename T>
  void return_value(T&& t) {
    val_.template emplace<R>(std::forward<T>(t));
  }

  decltype(auto) get_value() & {
    if (auto ret = std::get_if<R>(&val_)) {
      return *ret;
    } else if (auto exception = std::get_if<std::exception_ptr>(&val_)) {
      std::rethrow_exception(*exception);
    } else {
      throw std::runtime_error("No result");
    }
  }

  decltype(auto) get_value() && {
    if (auto ret = std::get_if<R>(&val_)) {
      return std::move(*ret);
    } else if (auto exception = std::get_if<std::exception_ptr>(&val_)) {
      std::rethrow_exception(std::move(*exception));
    } else {
      throw std::runtime_error("No result");
    }
  }

 private:
  std::variant<std::monostate, R, std::exception_ptr> val_;
};

template <>
class Result<void> {
 public:
  Result() = default;
  bool has_value() const { return val_.has_value(); }

  void unhandled_exception() noexcept { val_ = std::current_exception(); }
  void return_void() { val_.emplace(nullptr); }

  auto get_value() {
    if (val_.has_value() && val_.value() != nullptr) {
      std::rethrow_exception(val_.value());
    }
  }

 private:
  std::optional<std::exception_ptr> val_;
};
