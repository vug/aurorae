#include "Logger.h"

#include <spdlog/sinks/stdout_color_sinks.h>  // For console logging
#include <spdlog/spdlog.h>

#include <memory>
#include <source_location>
#include <string>

namespace aur {

namespace detail {
// bool shouldLog(spdlog::level::level_enum level) {
//   return spdlog::default_logger_raw()->should_log(level);
// }
inline spdlog::level::level_enum toSpdLogLevel(LogLevel level) {
  return static_cast<spdlog::level::level_enum>(static_cast<int32_t>(level));
}

void logWithSpd(const std::source_location& loc, LogLevel level,
                std::string_view msg) {
  const spdlog::source_loc spdLoc{loc.file_name(), static_cast<int>(loc.line()),
                                  loc.function_name()};
  const spdlog::level::level_enum spdLevel = toSpdLogLevel(level);
  spdlog::default_logger_raw()->log(spdLoc, spdLevel, msg);
}

void logWithSpdFatal(const std::source_location& loc, std::string_view msg) {
  logWithSpd(loc, LogLevel::Critical, msg);
  spdlog::default_logger_raw()->flush();
}
}  // namespace detail

void logInitialize(LogLevel defaultLevel, const std::string& pattern,
                   LogLevel flushLevel) {
  // Create a color console sink.
  // You can customize this to add file sinks, multiple sinks, etc.
  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  console_sink->set_level(spdlog::level::trace);  // Log all levels to the sink

  // You could also create a file sink
  // auto file_sink =
  // std::make_shared<spdlog::sinks::basic_file_sink_mt>("app_log.txt", true);
  // file_sink->set_level(spdlog::level::trace);

  // Create a logger with the sink(s)
  // std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
  // auto logger = std::make_shared<spdlog::logger>("app", sinks.begin(),
  // sinks.end());
  auto logger = std::make_shared<spdlog::logger>("app", console_sink);

  // Set the logger as the default spdlog logger
  spdlog::set_default_logger(logger);

  // Set the global logging level for spdlog (and our default logger)
  spdlog::set_level(detail::toSpdLogLevel(defaultLevel));

  // Set the pattern for all loggers (or just the default one if you prefer)
  spdlog::set_pattern(pattern);

  // Set the flush level
  spdlog::flush_on(detail::toSpdLogLevel(flushLevel));

  log().trace("Logging initialized.");
}

}  // namespace aur