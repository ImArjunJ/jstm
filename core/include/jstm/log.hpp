#pragma once

#include <cstdio>
#include <cstring>
#include <jstm/types.hpp>

namespace jstm::hal {
extern void log_uart_transmit(const char* data, u32 len) __attribute__((weak));
extern void log_lock() __attribute__((weak));
extern void log_unlock() __attribute__((weak));
}  // namespace jstm::hal

namespace jstm::log {

enum class level : u8 {
  trace = 0,
  debug = 1,
  info = 2,
  warn = 3,
  error = 4,
  off = 5,
};

#ifndef JSTM_LOG_LEVEL
#define JSTM_LOG_LEVEL 2
#endif

inline constexpr level min_level = static_cast<level>(JSTM_LOG_LEVEL);

namespace detail {

inline constexpr const char* level_tag(level l) {
  switch (l) {
    case level::trace:
      return "[TRC]";
    case level::debug:
      return "[DBG]";
    case level::info:
      return "[INF]";
    case level::warn:
      return "[WRN]";
    case level::error:
      return "[ERR]";
    default:
      return "[???]";
  }
}

template <level L, typename... Args>
inline void emit(const char* fmt, Args... args) {
  if constexpr (L >= min_level) {
    char buf[256];
    int off = snprintf(buf, sizeof(buf), "%s ", level_tag(L));
    off += snprintf(buf + off, sizeof(buf) - off, fmt, args...);
    if (off < static_cast<int>(sizeof(buf)) - 2) {
      buf[off++] = '\r';
      buf[off++] = '\n';
    }
    buf[off] = '\0';

    if (jstm::hal::log_lock) jstm::hal::log_lock();

    if (jstm::hal::log_uart_transmit) {
      jstm::hal::log_uart_transmit(buf, static_cast<u32>(off));
    } else {
      std::printf("%s", buf);
    }

    if (jstm::hal::log_unlock) jstm::hal::log_unlock();
  }
}

}  // namespace detail

template <typename... Args>
inline void trace(const char* fmt, Args... args) {
  detail::emit<level::trace>(fmt, args...);
}

template <typename... Args>
inline void debug(const char* fmt, Args... args) {
  detail::emit<level::debug>(fmt, args...);
}

template <typename... Args>
inline void info(const char* fmt, Args... args) {
  detail::emit<level::info>(fmt, args...);
}

template <typename... Args>
inline void warn(const char* fmt, Args... args) {
  detail::emit<level::warn>(fmt, args...);
}

template <typename... Args>
inline void error(const char* fmt, Args... args) {
  detail::emit<level::error>(fmt, args...);
}

}  // namespace jstm::log
