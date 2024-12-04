#include "Utility.h"

/* string trim */
// source for trim: https://stackoverflow.com/a/217605

void trimLeadingWhitespaceInPlace(std::string& str)
{
  str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](unsigned char ch) { return !std::isspace(ch); }));
}

void trimTrailingWhitespaceInPlace(std::string& str)
{
  str.erase(std::find_if(str.rbegin(), str.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), str.end());
}

void trimLeadingAndTrailingWhitespaceInPlace(std::string& str)
{
  trimTrailingWhitespaceInPlace(str);
  trimLeadingWhitespaceInPlace(str);
}

/* Value from String helper */

LogLevel logLevelFromStr(const std::string& str)
{
  for (const auto& [level, name] : LOG_LEVEL_MAP)
  {
    if (str == name)
    {
      return level;
    }
  }
  throw std::invalid_argument("Unable to find fitting log level for string.");
}
