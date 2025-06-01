#pragma once

#include <string_view> // For spdlog::format_string_t if not using C++20 std::format_string
#include <utility>     // For std::forward and std::exit
#include <source_location>

#include <spdlog/spdlog.h>

namespace aur {

// Call this once at the beginning of your application
void log_initialize(spdlog::level::level_enum default_level = spdlog::level::info,
                const std::string& pattern = "[%C%m%d %H%M:%S.%e] [%^%l%$] %v @%s:%# on T%t [%!]", // Matched previous default
                spdlog::level::level_enum flush_level = spdlog::level::warn);

namespace detail { 
    template <typename... Args>
    inline void log_at_loc(
        const std::source_location& loc,
        spdlog::level::level_enum level,
        std::format_string<Args...> fmt, Args &&...args) {
        spdlog::source_loc spdlog_source_loc{loc.file_name(), static_cast<int>(loc.line()), loc.function_name()};
        if (spdlog::default_logger_raw()->should_log(level)) {
            spdlog::default_logger_raw()->log(spdlog_source_loc, level, fmt, std::forward<Args>(args)...);
        }
    }

    template<typename... Args>
    [[noreturn]] inline void log_fatal_at_loc(
        const std::source_location& loc,
        std::format_string<Args...> fmt, Args&&... args) {
        spdlog::source_loc spdlog_source_loc{loc.file_name(), static_cast<int>(loc.line()), loc.function_name()};
        spdlog::default_logger_raw()->log(spdlog_source_loc, spdlog::level::critical, fmt, std::forward<Args>(args)...);
        spdlog::default_logger_raw()->flush(); 
        std::exit(EXIT_FAILURE);
    }

    // This class will hold the captured source_location and provide logging methods
    class LoggerProxy {
    public:
        // Constructor is explicit to avoid accidental conversions, though not strictly necessary here
        explicit LoggerProxy(const std::source_location& loc) : loc_(loc) {}

        template <typename... Args>
        void trace(std::format_string<Args...> fmt, Args&&... args) const {
            log_at_loc(loc_, spdlog::level::trace, fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        void debug(std::format_string<Args...> fmt, Args&&... args) const {
            log_at_loc(loc_, spdlog::level::debug, fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        void info(std::format_string<Args...> fmt, Args&&... args) const {
            log_at_loc(loc_, spdlog::level::info, fmt, std::forward<Args>(args)...);
        }
        
        template <typename... Args>
        void warn(std::format_string<Args...> fmt, Args&&... args) const {
            log_at_loc(loc_, spdlog::level::warn, fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        void error(std::format_string<Args...> fmt, Args&&... args) const {
            log_at_loc(loc_, fmt, spdlog::level::err, std::forward<Args>(args)...);
        }

        template <typename... Args>
        void critical(std::format_string<Args...> fmt, Args&&... args) const {
            log_at_loc(loc_, fmt, spdlog::level::critical, std::forward<Args>(args)...);
        }
        
        template<typename... Args>
        [[noreturn]] void fatal(std::format_string<Args...> fmt, Args&&... args) const {
            log_fatal_at_loc(loc_, fmt, std::forward<Args>(args)...);
        }

    private:
        std::source_location loc_; // Store the captured location
    };

} // namespace detail

// Public-facing logging function that returns the proxy object.
[[nodiscard]] inline detail::LoggerProxy log(const std::source_location& loc = std::source_location::current()) {
    return detail::LoggerProxy(loc);
}

} // namespace aur