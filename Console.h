#pragma once

#include <iostream>
#include <array>
#include <chrono>
#include <unordered_map>

enum class LogLevel : int
{
  TRACE = 0,
  DEBUG = 1,
  INFO = 2,
  WARNING = 3,
  ERROR = 4,
};

inline const std::unordered_map<LogLevel, const char*> LOG_LEVEL_MAP{
  { LogLevel::TRACE, "TRACE" },
  { LogLevel::DEBUG, "DEBUG" },
  { LogLevel::INFO, "INFO" },
  { LogLevel::WARNING, "WARNING" },
  { LogLevel::ERROR, "ERROR" }
};

inline std::ostream& LOG_OUT{ std::clog };
inline std::ostream& STD_OUT{ std::cout };

inline LogLevel currentLogLevel{ LogLevel::INFO };

template<class... Args>
void Log(const LogLevel level, const std::format_string<Args...> fmt, Args&&... args)
{
  if (level < currentLogLevel)
  {
    return;
  }
  const std::chrono::zoned_time currentTime{ std::chrono::current_zone(), std::chrono::system_clock::now() };
  std::print(LOG_OUT, "{:%Y-%m-%d %T %Z} : {} : ", currentTime, LOG_LEVEL_MAP.at(level));
  std::println(LOG_OUT, fmt, std::forward<Args>(args)...);
}

template<class... Args>
void Out(const std::format_string<Args...> fmt, Args&&... args)
{
  std::print(STD_OUT, fmt, std::forward<Args>(args)...);
}
