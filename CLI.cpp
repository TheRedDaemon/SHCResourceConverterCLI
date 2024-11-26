#include "CLI.h"

#include "Logger.h"

CLIArguments::CLIArguments(int argc, char* argv[])
{
  std::vector<std::string> args{ argv, argv + argc };
  std::string* pendingOption{ nullptr };
  for (std::string& arg : args)
  {
    if (arg.starts_with(OPTION_PREFIX))
    {
      if (pendingOption)
      {
        Log(LogLevel::WARNING, "Incomplete options param '{}'. Ignored.", *pendingOption);
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
    Log(LogLevel::WARNING, "Incomplete options param '{}'. Ignored.", *pendingOption);
  }
}

CLIArguments::~CLIArguments()
{
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
