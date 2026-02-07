#pragma once

#include <expected>
#include <jstm/types.hpp>

namespace jstm {

enum class error_code : u8 {
  ok,
  timeout,
  hardware_fault,
  invalid_argument,
  not_initialized,
  not_found,
  connection_failed,
  out_of_memory,
  io_error,
};

struct error {
  error_code code = error_code::ok;
  const char* message = "";

  constexpr explicit operator bool() const { return code != error_code::ok; }
};

template <typename T = void>
using result = std::expected<T, error>;

inline constexpr result<void> ok() { return {}; }

template <typename T>
inline constexpr result<std::decay_t<T>> ok(T&& val) {
  return std::forward<T>(val);
}

inline constexpr std::unexpected<error> fail(error_code code,
                                             const char* msg = "") {
  return std::unexpected{error{code, msg}};
}

}  // namespace jstm
