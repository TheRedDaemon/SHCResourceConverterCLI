#include "CLI.h"

#include "Logger.h"

CLIArguments::CLIArguments()
{
}

CLIArguments::CLIArguments(std::string& executionPath) : executionPath{ std::move(executionPath) }
{
}

CLIArguments::CLIArguments(std::string& executionPath, std::vector<std::string>& arguments,
  std::unordered_map<std::string, std::string>& options)
  : executionPath{ std::move(executionPath) }, arguments{ std::move(arguments) }, options{ std::move(options) }
{
}

CLIArguments CLIArguments::parse(int argc, char* argv[])
{
  if (argc < 1)
  {
    return {};
  }

  std::string executionPath{ argv[0] };
  if (argc < 2)
  {
    return CLIArguments{ executionPath };
  }

  std::vector<std::string> args{ argv + 1, argv + argc };

  std::vector<std::string> arguments;
  std::unordered_map<std::string, std::string> options;

  std::string* pendingOption{ nullptr };
  for (std::string& arg : args)
  {
    // first value should be the executable

    if (arg.starts_with(OPTION_PREFIX))
    {
      if (pendingOption)
      {
        Log(LogLevel::WARNING, "CLIArguments: Incomplete options param '{}'. Ignored.", *pendingOption);
      }
      pendingOption = &arg;
      continue;
    }

    if (pendingOption)
    {
      options.insert_or_assign(std::move(pendingOption->erase(0, OPTION_PREFIX.length())), std::move(arg));
      pendingOption = nullptr;
    }
    else
    {
      arguments.emplace_back(std::move(arg));
    }
  }
  if (pendingOption)
  {
    Log(LogLevel::WARNING, "CLIArguments: Incomplete options param '{}'. Ignored.", *pendingOption);
  }

  return CLIArguments{ executionPath, arguments, options };
}

CLIArguments::~CLIArguments()
{
}

const std::string& CLIArguments::getExecutionPath() const
{
  return executionPath;
}

const std::string* CLIArguments::getArgument(int index) const
{
  return 0 > index || index >= arguments.size() ? nullptr : &arguments.at(index);
}

const std::string* CLIArguments::getOption(const std::string& option) const
{
  auto it{ options.find(option) };
  return it == options.end() ? nullptr : &it->second;
}
