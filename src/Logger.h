#pragma once

#include <string_view> // For spdlog::format_string_t if not using C++20 std::format_string
#include <utility>     // For std::forward and std::exit
#include <source_location>

#include <spdlog/spdlog.h>

namespace aur::log { // Changed namespace from app_logger

// Call this once at the beginning of your application
void initialize(spdlog::level::level_enum default_level = spdlog::level::info,
                const std::string& pattern = "[%C%m%d %H:%M:%S.%e] [%^%l%$] [T%t] [%s:%# @ %!] %v", // Matched previous default
                spdlog::level::level_enum flush_level = spdlog::level::warn);

namespace detail { // Internal implementation details

    inline spdlog::source_loc make_spdlog_source_loc(const std::source_location& loc) {
        return {loc.file_name(), static_cast<int>(loc.line()), loc.function_name()};
    }

    // Renamed _impl to _log_at_loc for clarity with the proxy
    template <typename... Args>
    inline void log_trace_at_loc(
        const std::source_location& loc,
        spdlog::format_string_t<Args...> fmt, Args &&...args) {
        if (spdlog::default_logger_raw()->should_log(spdlog::level::trace)) {
            spdlog::default_logger_raw()->log(make_spdlog_source_loc(loc), spdlog::level::trace, fmt, std::forward<Args>(args)...);
        }
    }
    template <typename... Args>
    inline void log_debug_at_loc(
        const std::source_location& loc,
        spdlog::format_string_t<Args...> fmt, Args &&...args) {
        if (spdlog::default_logger_raw()->should_log(spdlog::level::debug)) {
            spdlog::default_logger_raw()->log(make_spdlog_source_loc(loc), spdlog::level::debug, fmt, std::forward<Args>(args)...);
        }
    }
    template <typename... Args>
    inline void log_info_at_loc(
        const std::source_location& loc,
        spdlog::format_string_t<Args...> fmt, Args &&...args) {
        if (spdlog::default_logger_raw()->should_log(spdlog::level::info)) {
            spdlog::default_logger_raw()->log(make_spdlog_source_loc(loc), spdlog::level::info, fmt, std::forward<Args>(args)...);
        }
    }
    template <typename... Args>
    inline void log_warn_at_loc(
        const std::source_location& loc,
        spdlog::format_string_t<Args...> fmt, Args &&...args) {
        if (spdlog::default_logger_raw()->should_log(spdlog::level::warn)) {
            spdlog::default_logger_raw()->log(make_spdlog_source_loc(loc), spdlog::level::warn, fmt, std::forward<Args>(args)...);
        }
    }
    template <typename... Args>
    inline void log_error_at_loc(
        const std::source_location& loc,
        spdlog::format_string_t<Args...> fmt, Args &&...args) {
        if (spdlog::default_logger_raw()->should_log(spdlog::level::err)) {
            spdlog::default_logger_raw()->log(make_spdlog_source_loc(loc), spdlog::level::err, fmt, std::forward<Args>(args)...);
        }
    }
    template <typename... Args>
    inline void log_critical_at_loc(
        const std::source_location& loc,
        spdlog::format_string_t<Args...> fmt, Args &&...args) {
        if (spdlog::default_logger_raw()->should_log(spdlog::level::critical)) {
            spdlog::default_logger_raw()->log(make_spdlog_source_loc(loc), spdlog::level::critical, fmt, std::forward<Args>(args)...);
        }
    }
    template<typename... Args>
    [[noreturn]] inline void log_fatal_at_loc(
        const std::source_location& loc,
        spdlog::format_string_t<Args...> fmt, Args&&... args) {
        spdlog::default_logger_raw()->log(make_spdlog_source_loc(loc), spdlog::level::critical, fmt, std::forward<Args>(args)...);
        spdlog::default_logger_raw()->flush(); 
        std::exit(EXIT_FAILURE);
    }

    // This class will hold the captured source_location and provide logging methods
    class LoggerProxy {
    public:
        // Constructor is explicit to avoid accidental conversions, though not strictly necessary here
        explicit LoggerProxy(const std::source_location& loc) : loc_(loc) {}

        template <typename... Args>
        void trace(spdlog::format_string_t<Args...> fmt, Args&&... args) const {
            log_trace_at_loc(loc_, fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        void debug(spdlog::format_string_t<Args...> fmt, Args&&... args) const {
            log_debug_at_loc(loc_, fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        void info(spdlog::format_string_t<Args...> fmt, Args&&... args) const {
            log_info_at_loc(loc_, fmt, std::forward<Args>(args)...);
        }
        
        template <typename... Args>
        void warn(spdlog::format_string_t<Args...> fmt, Args&&... args) const {
            log_warn_at_loc(loc_, fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        void error(spdlog::format_string_t<Args...> fmt, Args&&... args) const {
            log_error_at_loc(loc_, fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        void critical(spdlog::format_string_t<Args...> fmt, Args&&... args) const {
            log_critical_at_loc(loc_, fmt, std::forward<Args>(args)...);
        }
        
        template<typename... Args>
        [[noreturn]] void fatal(spdlog::format_string_t<Args...> fmt, Args&&... args) const {
            log_fatal_at_loc(loc_, fmt, std::forward<Args>(args)...);
        }

    private:
        std::source_location loc_; // Store the captured location
    };

} // namespace detail

// Public-facing logging functions
// Public API: a function that returns the proxy object.
// [[nodiscard]] encourages the user to actually call a logging method on the returned proxy.
[[nodiscard]] inline detail::LoggerProxy at(const std::source_location& loc = std::source_location::current()) {
    return detail::LoggerProxy(loc);
}

} // namespace aur::log