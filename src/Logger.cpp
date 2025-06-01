#include "Logger.h"
#include <spdlog/sinks/stdout_color_sinks.h> // For console logging
#include <memory>
#include <string>

namespace aur {

void log_initialize(spdlog::level::level_enum default_level,
                const std::string& pattern,
                spdlog::level::level_enum flush_level) {
    // Create a color console sink.
    // You can customize this to add file sinks, multiple sinks, etc.
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::trace); // Log all levels to the sink

    // You could also create a file sink
    // auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("app_log.txt", true);
    // file_sink->set_level(spdlog::level::trace);

    // Create a logger with the sink(s)
    // std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
    // auto logger = std::make_shared<spdlog::logger>("app", sinks.begin(), sinks.end());
    auto logger = std::make_shared<spdlog::logger>("app", console_sink);
    
    // Set the logger as the default spdlog logger
    spdlog::set_default_logger(logger);

    // Set the global logging level for spdlog (and our default logger)
    spdlog::set_level(default_level);

    // Set the pattern for all loggers (or just the default one if you prefer)
    spdlog::set_pattern(pattern);

    // Set the flush level
    spdlog::flush_on(flush_level);
}

} // namespace aur