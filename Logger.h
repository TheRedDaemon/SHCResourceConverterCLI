#ifndef LOGGER_HEADER
#define LOGGER_HEADER

#include <iostream>
#include <array>
#include <chrono>

enum class LogLevel : int
{
  TRACE = 0,
  DEBUG = 1,
  INFO = 2,
  WARNING = 3,
  ERROR = 4,
};

inline constexpr std::array<const char*, 5> LOG_LEVEL_NAMES{ "TRACE", "DEBUG", "INFO", "WARNING", "ERROR" };

inline std::ostream& LOG_STREAM{ std::clog };

inline LogLevel currentLogLevel{ LogLevel::INFO };

template<class... Args>
void Log(const LogLevel level, const std::format_string<Args...> fmt, Args&&... args)
{
  if (level < currentLogLevel)
  {
    return;
  }
  const std::chrono::zoned_time currentTime{ std::chrono::current_zone(), std::chrono::system_clock::now() };
  std::print(LOG_STREAM, "{:%Y-%m-%d %T %Z} : {} : ", currentTime, LOG_LEVEL_NAMES.at(static_cast<int>(level)));
  std::println(LOG_STREAM, fmt, std::forward<Args>(args)...);
}

#endif //LOGGER_HEADER
