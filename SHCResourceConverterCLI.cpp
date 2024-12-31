
#include <string>
#include <memory>
#include <vector>
#include <fstream>
#include <filesystem>

#include "Console.h"
#include "Utility.h"
#include "CLI.h"

#include "SHCResourceConverter.h"
#include "TGXCoder.h"
#include "BinaryCFileReadHelper.h"

#include "TGXFile.h"
#include "GM1File.h"

// only test, TODO: clean
#include "ResourceMetaFormat.h"


/*
  GENERAL INFO:
  Certain parts use a C-like structure, since the it is also partially intended to experiment for
  potential UCP3 modules. Since the provided APIs by modules are C-like and it is not clear which parts
  will make it into a modules, the use of C-like structures is arbitrary.

  Note for modules:
  - C-like APIs should never throw
  - Memory control should be handled by one module, since passing around structures is error-prone
  - never mix malloc/free with new/delete and the later never with their array versions
*/

// TODO: C like APIs should never throw

// TODO: define constants for options


/* Constants */

// CLI COMMANDS:
namespace COMMAND
{
  inline const std::string TEST{ "test" };
  inline const std::string EXTRACT{ "extract" };
  inline const std::string PACK{ "pack" };
  inline const std::string HELP{ "help" };
}

// CLI OPTIONS:
namespace OPTION
{
  inline const std::string LOG{ "log" };
  inline const std::string TEST_TGX_TO_TEXT{ "test-tgx-to-text" };
  inline const std::string TGX_CODER_TRANSPARENT_PIXEL_TGX_COLOR{ "tgx-coder-transparent-pixel-tgx-color" };
  inline const std::string TGX_CODER_TRANSPARENT_PIXEL_RAW_COLOR{ "tgx-coder-transparent-pixel-raw-color" };
  inline const std::string TGX_CODER_PIXEL_REPEAT_THRESHOLD{ "tgx-coder-pixel-repeat-threshold" };
  inline const std::string TGX_CODER_PADDING_ALIGNMENT{ "tgx-coder-padding-alignment" };
}

enum class PathNameType
{
  TGX_FILE,
  GM1_FILE,
  FOLDER,
  UNKNOWN
};


/* Support functions */

static LogLevel logLevelFromStr(const std::string& str)
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

static void setLogLevelFromCliOption(const CLIArguments& cliArguments)
{
  const std::optional<LogLevel> logLevel{ cliArguments.getOptionAs<logLevelFromStr>(OPTION::LOG) };
  if (logLevel)
  {
    currentLogLevel = *logLevel;
    Log(LogLevel::DEBUG, "Setting log level to {}.", LOG_LEVEL_MAP.at(currentLogLevel));
  }
  else
  {
    Log(LogLevel::INFO, "No log level provided, defaulting to {}.", LOG_LEVEL_MAP.at(currentLogLevel));
  }
}


static TgxCoderInstruction getCoderInstructionFromCliOptionsWithFallback(const CLIArguments& cliArguments)
{
  TgxCoderInstruction coderInstruction{
    cliArguments.getOptionAs<uintFromStr<uint16_t>>(OPTION::TGX_CODER_TRANSPARENT_PIXEL_TGX_COLOR).value_or(TGX_FILE_DEFAULT_INSTRUCTION.transparentPixelTgxColor),
    cliArguments.getOptionAs<uintFromStr<uint16_t>>(OPTION::TGX_CODER_TRANSPARENT_PIXEL_RAW_COLOR).value_or(TGX_FILE_DEFAULT_INSTRUCTION.transparentPixelRawColor),
    cliArguments.getOptionAs<intFromStr<int>>(OPTION::TGX_CODER_PIXEL_REPEAT_THRESHOLD).value_or(TGX_FILE_DEFAULT_INSTRUCTION.pixelRepeatThreshold),
    cliArguments.getOptionAs<intFromStr<int>>(OPTION::TGX_CODER_PADDING_ALIGNMENT).value_or(TGX_FILE_DEFAULT_INSTRUCTION.paddingAlignment)
  };
  Log(LogLevel::DEBUG, "General TGX Coder Instructions:\n{}", coderInstruction);
  return coderInstruction;
}

static PathNameType determinePathNameType(const std::filesystem::path& path)
{
  const std::string extension{ path.extension().string() };
  if (extension == TGXFile::FILE_EXTENSION)
  {
    return PathNameType::TGX_FILE;
  }
  if (extension == GM1File::FILE_EXTENSION)
  {
    return PathNameType::GM1_FILE;
  }
  else if (extension == "")
  {
    return PathNameType::FOLDER;
  }
  else
  {
    return PathNameType::UNKNOWN;
  }
}


/* COMMAND FUNCTIONS */

static void printHelp()
{
  // TODO: create help text
}

static int executeTest(const CLIArguments& cliArguments)
{
  try
  {
    Log(LogLevel::INFO, "Try testing provided file.");
    const std::string* sourceStr{ cliArguments.getArgument(1) };
    const std::string* argNumCheck{ cliArguments.getArgument(2) };
    if (!sourceStr || argNumCheck)
    {
      Log(LogLevel::WARNING, "Argument missing or too many arguments provided. Printing help.");
      printHelp();
      return 1;
    }
    const std::filesystem::path source{ sourceStr->c_str() };

    switch (determinePathNameType(source))
    {
    case PathNameType::TGX_FILE:
    {
      Log(LogLevel::INFO, "Try testing provided TGX file path.");
      const TGXFile::UniqueTgxResourcePointer tgxResource{ TGXFile::loadTgxResource(source) };
      if (!tgxResource)
      {
        return 1;
      }
      TGXFile::validateTgxResource(*tgxResource, cliArguments.getOptionAs<boolFromStr>(OPTION::TEST_TGX_TO_TEXT).value_or(false));
    }
    break;
    case PathNameType::GM1_FILE:
    {
      Log(LogLevel::INFO, "Try testing provided GM1 file path.");
      const GM1File::UniqueGm1ResourcePointer gm1Resource{ GM1File::loadGm1Resource(source) };
      if (!gm1Resource)
      {
        return 1;
      }
      GM1File::validateGm1Resource(*gm1Resource, getCoderInstructionFromCliOptionsWithFallback(cliArguments),
        cliArguments.getOptionAs<boolFromStr>(OPTION::TEST_TGX_TO_TEXT).value_or(false));
    }
    break;
    default:
      Log(LogLevel::ERROR, "Provided file path has no supported file extension.");
      return 1;
    }
    Log(LogLevel::INFO, "Successfully tested provided file.");
    return 0;
  }
  catch (const std::exception& e)
  {
    Log(LogLevel::ERROR, "Encountered exception during file test: {}", e.what());
    return 1;
  }
}

static int executeExtract(const CLIArguments& cliArguments)
{
  try
  {
    Log(LogLevel::INFO, "Try extracting provided file.");
    const std::string* sourceStr{ cliArguments.getArgument(1) };
    const std::string* targetStr{ cliArguments.getArgument(2) };
    const std::string* argNumCheck{ cliArguments.getArgument(3) };
    if (!sourceStr || !targetStr || argNumCheck)
    {
      Log(LogLevel::WARNING, "Argument missing or too many arguments provided. Printing help.");
      printHelp();
      return 1;
    }
    const std::filesystem::path source{ sourceStr->c_str() };
    const std::filesystem::path target{ targetStr->c_str() };

    if (determinePathNameType(target) != PathNameType::FOLDER)
    {
      Log(LogLevel::ERROR, "Provided folder has an extension, which is not supported.");
      return 1;
    }

    switch (determinePathNameType(source))
    {
    case PathNameType::TGX_FILE:
    {
      Log(LogLevel::INFO, "Try extracting provided TGX file.");
      const TGXFile::UniqueTgxResourcePointer tgxResource{ TGXFile::loadTgxResource(source) };
      if (!tgxResource)
      {
        return 1;
      }
      TGXFile::saveTgxResourceAsRaw(target, *tgxResource, getCoderInstructionFromCliOptionsWithFallback(cliArguments));
    }
    break;
    case PathNameType::GM1_FILE:
    {
      Log(LogLevel::INFO, "Try extracting provided GM1 file.");
      const GM1File::UniqueGm1ResourcePointer gm1Resource{ GM1File::loadGm1Resource(source) };
      if (!gm1Resource)
      {
        return 1;
      }
      GM1File::saveGm1ResourceAsRaw(target, *gm1Resource);
    }
    break;
    default:
      Log(LogLevel::ERROR, "Provided file path has no supported file extension.");
      return 1;
    }
    Log(LogLevel::INFO, "Successfully extracted provided file.");
    return 0;
  }
  catch (const std::exception& e)
  {
    Log(LogLevel::ERROR, "Encountered exception during file extract: {}", e.what());
    return 1;
  }
}

static int executePack(const CLIArguments& cliArguments)
{
  try
  {
    Log(LogLevel::INFO, "Try packing provided file structure.");
    const std::string* sourceStr{ cliArguments.getArgument(1) };
    const std::string* targetStr{ cliArguments.getArgument(2) };
    const std::string* argNumCheck{ cliArguments.getArgument(3) };
    if (!sourceStr || !targetStr || argNumCheck)
    {
      Log(LogLevel::WARNING, "Argument missing or too many arguments provided. Printing help.");
      printHelp();
      return 1;
    }
    const std::filesystem::path source{ sourceStr->c_str() };
    const std::filesystem::path target{ targetStr->c_str() };

    if (determinePathNameType(source) != PathNameType::FOLDER)
    {
      Log(LogLevel::ERROR, "Provided folder has an extension, which is not supported.");
      return 1;
    }

    switch (determinePathNameType(target))
    {
    case PathNameType::TGX_FILE:
    {
      Log(LogLevel::INFO, "Try packing provided TGX folder.");
      const TGXFile::UniqueTgxResourcePointer tgxResource{ TGXFile::loadTgxResourceFromRaw(source, getCoderInstructionFromCliOptionsWithFallback(cliArguments)) };
      if (!tgxResource)
      {
        return 1;
      }
      TGXFile::saveTgxResource(target, *tgxResource);
    }
    break;
    case PathNameType::GM1_FILE:
    {
      Log(LogLevel::INFO, "Try packing provided GM1 folder.");
      const GM1File::UniqueGm1ResourcePointer gm1Resource{ GM1File::loadGm1ResourceFromRaw(source) };
      if (!gm1Resource)
      {
        return 1;
      }
      GM1File::saveGm1Resource(target, *gm1Resource);
    }
    break;
    default:
      Log(LogLevel::ERROR, "Provided result file path has no supported file extension.");
      return 1;
    }
    Log(LogLevel::INFO, "Successfully packed provided file.");
    return 0;
  }
  catch (const std::exception& e)
  {
    Log(LogLevel::ERROR, "Encountered exception during file pack: {}", e.what());
    return 1;
  }
}


/* MAIN */

int main(int argc, char* argv[])
{
  try
  {
    const CLIArguments cliArguments{ CLIArguments::parse(argc, argv) };
    setLogLevelFromCliOption(cliArguments);
    Log(LogLevel::DEBUG, "CLIArguments:\n{}", cliArguments);
   
    const std::string* command{ cliArguments.getArgument(0) };
    if (!command)
    {
      Log(LogLevel::WARNING, "No command provided. Printing help.");
      printHelp();
      return 1;
    }

    if (COMMAND::HELP == *command)
    {
      printHelp();
      return 0;
    }

    if (COMMAND::TEST == *command)
    {
      const int result{ executeTest(cliArguments) };
      if (result != 0)
      {
        return result;
      }
    } else if (COMMAND::EXTRACT == *command)
    {
      const int result{ executeExtract(cliArguments) };
      if (result != 0)
      {
        return result;
      }
    } else if (COMMAND::PACK == *command)
    {
      const int result{ executePack(cliArguments) };
      if (result != 0)
      {
        return result;
      }
    }
    else
    {
      Log(LogLevel::ERROR, "Unknown command provided. Printing help.");
      printHelp();
      return 1;
    }

    return 0;
  }
  catch (const std::exception& e)
  {
    Log(LogLevel::ERROR, "Unexpected error occurred: {}", e.what());
    return 1;
  }
}
