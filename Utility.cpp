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

/* Value from string helper */

bool boolFromStr(const std::string& str)
{
  if (str == "true" || str == "1")
  {
    return true;
  }
  else if (str == "false" || str == "0")
  {
    return false;
  }
  throw std::invalid_argument("Unable to convert string to bool.");
}
