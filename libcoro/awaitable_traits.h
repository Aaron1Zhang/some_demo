

template <typename awaitable>
struct awaitable_traits {
  using awaiter_type = decltype(get_awaiter(std::declval<awaitable>()));
  using awaiter_return_type =
      decltype(std::declval<awaiter_type>().await_resume());
};

template <>
struct awaitable_traits<void> {};
