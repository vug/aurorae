#pragma once

#include <spdlog/spdlog.h>
#include <cstdlib>

namespace aur {

template<typename... Args> // Note: FormatString template parameter is removed
void fatal(spdlog::format_string_t<Args...> fmt, Args&&... args) {
  spdlog::critical(fmt, std::forward<Args>(args)...);
  std::exit(EXIT_FAILURE);
}

} // namespace aur