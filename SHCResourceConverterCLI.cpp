
#include <string>
#include <memory>
#include <vector>
#include <fstream>
#include <filesystem>

#include "Logger.h"
#include "Utility.h"
#include "CLI.h"

#include "SHCResourceConverter.h"
#include "TGXCoder.h"
#include "BinaryCFileReadHelper.h"

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
  inline const std::string TGX_CODER_TRANSPARENT_PIXEL_TGX_COLOR{ "tgx-coder-transparent-pixel-tgx-color" };
  inline const std::string TGX_CODER_TRANSPARENT_PIXEL_RAW_COLOR{ "tgx-coder-transparent-pixel-raw-color" };
  inline const std::string TGX_CODER_PIXEL_REPEAT_THRESHOLD{ "tgx-coder-pixel-repeat-threshold" };
  inline const std::string TGX_CODER_PADDING_ALIGNMENT{ "tgx-coder-padding-alignment" };
}

static int intFromStr(const std::string& str)
{
  return std::stoi(str);
}
static uint16_t uint16FromStr(const std::string& str)
{
  unsigned long result{ std::stoul(str) };
  if (result > std::numeric_limits<uint16_t>::max())
  {
    throw std::out_of_range("Value is out of range.");
  }
  return static_cast<uint16_t>(result);
}

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
  return TgxCoderInstruction{
    cliArguments.getOptionAs<uint16FromStr>(OPTION::TGX_CODER_TRANSPARENT_PIXEL_TGX_COLOR).value_or(TGX_FILE_DEFAULT_INSTRUCTION.transparentPixelTgxColor),
    cliArguments.getOptionAs<uint16FromStr>(OPTION::TGX_CODER_TRANSPARENT_PIXEL_RAW_COLOR).value_or(TGX_FILE_DEFAULT_INSTRUCTION.transparentPixelRawColor),
    cliArguments.getOptionAs<intFromStr>(OPTION::TGX_CODER_PIXEL_REPEAT_THRESHOLD).value_or(TGX_FILE_DEFAULT_INSTRUCTION.pixelRepeatThreshold),
    cliArguments.getOptionAs<intFromStr>(OPTION::TGX_CODER_PADDING_ALIGNMENT).value_or(TGX_FILE_DEFAULT_INSTRUCTION.paddingAlignment)
  };
}

static void printHelp()
{
  // TODO: create help text
}

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

    const TgxCoderInstruction coderInstruction{ getCoderInstructionFromCliOptionsWithFallback(cliArguments) };
    Log(LogLevel::DEBUG, "General TGX Coder Instructions:\n{}", coderInstruction);
    if (COMMAND::TEST == *command)
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

      // TODO: test
    } else if (COMMAND::EXTRACT == *command)
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

      // TODO: extract
    } else if (COMMAND::PACK == *command)
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

      // TODO: pack
    }
    else
    {
      Log(LogLevel::ERROR, "Unknown command provided. Printing help.");
      printHelp();
      return 1;
    }

    // TODO: remove, debug space

    std::istringstream testStream{ R"(
      # this is a comment

      RESOURCE_META_HEADER 1 # this is a comment
      : key 1 = value 1# this is a comment
      :=# this is a comment
      - list entry 1
      - list entry 2  # this is a comment
      -# this is a comment

      # this is a comment

      OBJECT    1
      :   key 1=value   1
      :key 2=value 2
      -    list entry 1
      -list entry 2
      -list entry 3)" 
    };

    const ResourceMetaFileReader reader{ ResourceMetaFileReader::parseFrom(testStream) };

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

    // test
    auto fileReader{ createWithAdditionalMemory<BinaryCFileReadHelper>(100, source.string().c_str() ) };
    if (fileReader->hasInvalidState())
    {
      return 1;
    }
    const int size{ fileReader->getSize() };
    if (size < 0)
    {
      return 1;
    }

    if (!source.has_extension())
    {
      return 1;
    }

    const std::string extension{ source.extension().string() };
    if (extension == ".tgx")
    {
      auto resource{ createWithAdditionalMemory<TgxResource>(size) };
      fileReader->read(reinterpret_cast<uint8_t*>(resource.get()) + sizeof(TgxResource), sizeof(uint8_t), size);
      if (fileReader->hasInvalidState())
      {
        return 1;
      }
      resource->base.type = SHCResourceType::SHC_RESOURCE_TGX;
      resource->base.resourceSize = size;
      resource->base.colorFormat = PixeColorFormat::ARGB_1555;
      resource->dataSize = size - sizeof(TgxHeader);
      resource->header = reinterpret_cast<TgxHeader*>(reinterpret_cast<uint8_t*>(resource.get()) + sizeof(TgxResource));
      resource->imageData = reinterpret_cast<uint8_t*>(resource->header) + sizeof(TgxHeader);

      std::ofstream outRawFile{ target, std::ios::binary };
      if (!outRawFile.is_open())
      {
        return 1;
      }

      TgxCoderTgxInfo tgxInfo{ resource->imageData, resource->dataSize, resource->header->width, resource->header->height };
      TgxCoderRawInfo rawInfo{ nullptr, resource->header->width * 2, resource->header->height * 2, resource->header->width, resource->header->height };
      std::unique_ptr<uint16_t[]> rawPixels{ std::make_unique<uint16_t[]>(static_cast<size_t>(rawInfo.rawWidth * rawInfo.rawHeight)) };
      rawInfo.data = rawPixels.get();

      decodeTgxToRaw(&tgxInfo, &rawInfo, &coderInstruction, nullptr);
      outRawFile.write((char*) rawInfo.data, rawInfo.rawWidth * rawInfo.rawHeight * sizeof(uint16_t));

      // TODO: test why size is bigger, test in general if valid result
      tgxInfo.data = nullptr;
      tgxInfo.dataSize = 0;
      encodeRawToTgx(&rawInfo, &tgxInfo, &coderInstruction);
      std::unique_ptr<uint8_t[]> tgxRaw{ std::make_unique<uint8_t[]>(tgxInfo.dataSize) };
      tgxInfo.data = tgxRaw.get();
      encodeRawToTgx(&rawInfo, &tgxInfo, &coderInstruction);

      TgxAnalysis tgxAnalyses{};
      if (analyzeTgxToRaw(&tgxInfo, &coderInstruction, &tgxAnalyses) == TgxCoderResult::SUCCESS)
      {
        std::ofstream outTgxFile{ source.string().append(".test.tgx").c_str(), std::ios::binary };
        if (!outTgxFile.is_open())
        {
          return 1;
        }
        outTgxFile.write(reinterpret_cast<char*>(&resource->header->width), sizeof(uint32_t));
        outTgxFile.write(reinterpret_cast<char*>(&resource->header->height), sizeof(uint32_t));
        outTgxFile.write(reinterpret_cast<char*>(tgxInfo.data), tgxInfo.dataSize);
      }
    }
    else if (extension == ".gm1")
    {
      auto resource{ createWithAdditionalMemory<Gm1Resource>(size) };
      fileReader->read(reinterpret_cast<uint8_t*>(resource.get()) + sizeof(Gm1Resource), sizeof(uint8_t), size);
      if (fileReader->hasInvalidState())
      {
        return 1;
      }
      resource->base.type = SHCResourceType::SHC_RESOURCE_GM1;
      resource->base.resourceSize = size;
      resource->base.colorFormat = PixeColorFormat::ARGB_1555;
      resource->gm1Header = reinterpret_cast<Gm1Header*>(reinterpret_cast<uint8_t*>(resource.get()) + sizeof(Gm1Resource));
      resource->imageOffsets = reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(resource->gm1Header) + sizeof(Gm1Header));
      resource->imageSizes = reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(resource->imageOffsets) + sizeof(uint32_t) * resource->gm1Header->numberOfPicturesInFile);
      resource->imageHeaders = reinterpret_cast<Gm1ImageHeader*>(reinterpret_cast<uint8_t*>(resource->imageSizes) + sizeof(uint32_t) * resource->gm1Header->numberOfPicturesInFile);
      resource->imageData = reinterpret_cast<uint8_t*>(resource->imageHeaders) + sizeof(Gm1ImageHeader) * resource->gm1Header->numberOfPicturesInFile;

      uint32_t offsetOne{ resource->imageOffsets[1] };
      uint32_t sizeOne{ resource->imageSizes[1] };
      Gm1ImageHeader* headerOne{ resource->imageHeaders + 1 };

      uint32_t sizeOfDataBasedOnHeader{ resource->gm1Header->dataSize };
      uint32_t sizeOfDataBasedOnSize{ size - sizeof(Gm1Header) - (2 * sizeof(uint32_t) + sizeof(Gm1ImageHeader)) * resource->gm1Header->numberOfPicturesInFile };

      // validation required? -> Maybe partial read?
    }
    else
    {
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
