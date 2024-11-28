#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <functional>

#include "Logger.h"

const std::string OPTION_PREFIX{ "--" };

class CLIArguments
{
private:
  const std::string executionPath{};
  const std::vector<std::string> arguments{};
  const std::unordered_map<std::string, std::string> options{};

  // args are always consumed and moved out of the provided objects
  CLIArguments();
  CLIArguments(std::string& executionPath);
  CLIArguments(std::string& executionPath, std::vector<std::string>& arguments,
    std::unordered_map<std::string, std::string>& options);
public:
  ~CLIArguments();

  static CLIArguments parse(int argc, char* argv[]);

  const std::string& getExecutionPath() const;
  const std::string* getArgument(int index) const;
  const std::string* getOption(const std::string& option) const;

  template<auto func>
  std::optional<std::invoke_result_t<decltype(func), std::string>> getArgumentAs(int index) const
  {
    const std::string* arg{ getArgument(index) };
    try {
      return arg ? std::optional{ func(*arg) } : std::nullopt;
    }
    catch (const std::exception& e)
    {
      Log(LogLevel::WARNING, "Failed to convert argument for index {} = '{}': {}", index, *arg, e.what());
      return std::nullopt;
    }
  }
  template<auto func>
  std::optional<std::invoke_result_t<decltype(func), std::string>> getOptionAs(const std::string& option) const
  {
    const std::string* arg{ getOption(option) };
    try
    {
      return arg ? std::optional{ func(*arg) } : std::nullopt;
    }
    catch (const std::exception& e)
    {
      Log(LogLevel::WARNING, "Failed to convert option for key '{}' = '{}': {}", option, *arg, e.what());
      return std::nullopt;
    }
  }

  CLIArguments(const CLIArguments&) = delete;
  CLIArguments& operator=(const CLIArguments&) = delete;
};
