#pragma once

#include <format>
#include <source_location>
#include <string_view>
#include <utility>

#include "Utils.h"

namespace aur {

enum class LogLevel : i32 {
  Trace = 0,
  Debug = 1,
  Info = 2,
  Warning = 3,
  Error = 4,
  Critical = 5,
  Off = 6,
};

// Call this once at the beginning of your application
void logInitialize(LogLevel defaultLevel = LogLevel::Info,
                   const std::string& pattern = "[%C%m%d %H%M:%S.%e] [%^%l%$] %v @%s:%# on T%t [%!]",
                   LogLevel flushLevel = LogLevel::Warning);

namespace detail {

void logWithSpd(const std::source_location& loc, LogLevel level, std::string_view msg);
void logWithSpdFatal(const std::source_location& loc, std::string_view msg);
;

template <typename... Args>
inline void log_at_loc(const std::source_location& loc, LogLevel level, std::format_string<Args...> fmt,
                       Args&&... args) {
  auto formattedMessage = std::format(fmt, std::forward<Args>(args)...);
  logWithSpd(loc, level, formattedMessage);
}

template <typename... Args>
[[noreturn]] inline void log_fatal_at_loc(const std::source_location& loc, std::format_string<Args...> fmt,
                                          Args&&... args) {
  auto formattedMessage = std::format(fmt, std::forward<Args>(args)...);
  logWithSpdFatal(loc, formattedMessage);
  if constexpr (kBuildType != BuildType::Release)
    std::abort();
  else
    std::exit(EXIT_FAILURE);
}

// This class will hold the captured source_location and provide logging methods
class LoggerProxy {
public:
  // Constructor is explicit to avoid accidental conversions, though not
  // strictly necessary here
  explicit LoggerProxy(const std::source_location& loc)
      : loc_(loc) {}

  template <typename... Args>
  void trace(std::format_string<Args...> fmt, Args&&... args) const {
    log_at_loc(loc_, LogLevel::Trace, fmt, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void debug(std::format_string<Args...> fmt, Args&&... args) const {
    log_at_loc(loc_, LogLevel::Debug, fmt, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void info(std::format_string<Args...> fmt, Args&&... args) const {
    log_at_loc(loc_, LogLevel::Info, fmt, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void warn(std::format_string<Args...> fmt, Args&&... args) const {
    log_at_loc(loc_, LogLevel::Warning, fmt, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void error(std::format_string<Args...> fmt, Args&&... args) const {
    log_at_loc(loc_, LogLevel::Error, fmt, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void critical(std::format_string<Args...> fmt, Args&&... args) const {
    log_at_loc(loc_, LogLevel::Critical, fmt, std::forward<Args>(args)...);
  }

  template <typename... Args>
  [[noreturn]] void fatal(std::format_string<Args...> fmt, Args&&... args) const {
    log_fatal_at_loc(loc_, fmt, std::forward<Args>(args)...);
  }

private:
  std::source_location loc_; // Store the captured location
};

} // namespace detail

// Public-facing logging function that returns the proxy object.
[[nodiscard]] inline detail::LoggerProxy
log(const std::source_location& loc = std::source_location::current()) {
  return detail::LoggerProxy(loc);
}

} // namespace aur