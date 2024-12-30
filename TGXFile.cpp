#include "TGXFile.h"

#include "Console.h"
#include "ResourceMetaFormat.h"

#include <fstream>
#include <span>

namespace TGXFile
{
  void validateTgxResource(const TgxResource& resource, const TgxCoderInstruction& instructions, bool tgxAsText)
  {
    Log(LogLevel::INFO, "Try validating given resource.");
    
    const TgxCoderTgxInfo tgxInfo{
      .colorType{ TgxColorType::DEFAULT },
      .data{ resource.imageData },
      .dataSize{ resource.dataSize },
      .tgxWidth{ resource.header->width },
      .tgxHeight{ resource.header->height }
    };

    Out("### General TGX info ###\n{}\n\n", tgxInfo);
    TgxAnalysis tgxAnalysis{};
    const TgxCoderResult result{ analyzeTgxToRaw(&tgxInfo, &instructions, &tgxAnalysis) };
    if (result != TgxCoderResult::SUCCESS)
    {
      Out("{}\n", std::string_view{ getTgxResultDescription(result) });
      Out("### TGX seems invalid. ###\n");
      Log(LogLevel::ERROR, "Validation completed. TGX is invalid.");
    }

    Out("### Structure meta data ###\n{}\n\n### TGX seems valid ###\n", tgxAnalysis);
    Log(LogLevel::INFO, "Validation completed successfully.");
    if (!tgxAsText)
    {
      return;
    }
    Log(LogLevel::INFO, "Printing TGX as text to stdout.");
    Out("\n");
    const TgxCoderResult toTextResult{ decodeTgxToText(tgxInfo, instructions, STD_OUT) };
    if (toTextResult != TgxCoderResult::SUCCESS)
    {
      Out("{}\n", std::string_view{ getTgxResultDescription(toTextResult) });
      Log(LogLevel::ERROR, "Failed to print TGX as text.");
    }
    Log(LogLevel::INFO, "Completed to print TGX as text.");
  }

  UniqueTgxResourcePointer loadTgxResource(const std::filesystem::path& file)
  {
    Log(LogLevel::INFO, "Try loading TGX file.");
    if (!std::filesystem::is_regular_file(file))
    {
      Log(LogLevel::ERROR, "Provided TGX file is not a regular file.");
      return {};
    }
    const uintmax_t fileSize{ std::filesystem::file_size(file) };
    if (fileSize < MIN_FILE_SIZE)
    {
      Log(LogLevel::ERROR, "Provided file is too small for a TGX file.");
      return {};
    }
    if (fileSize > MAX_FILE_SIZE)
    {
      Log(LogLevel::ERROR, "Provided TGX file is too big to be handled by this implementation.");
      return {};
    }
    const uint32_t size{ static_cast<uint32_t>(fileSize) };

    std::ifstream in;
    in.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    in.open(file, std::ios::in | std::ios::binary);

    auto resource{ createWithAdditionalMemory<TgxResource>(size) };
    in.read(reinterpret_cast<char*>(resource.get()) + sizeof(TgxResource), size);
    resource->base.type = SHCResourceType::SHC_RESOURCE_TGX;
    resource->base.resourceSize = size;
    resource->base.colorFormat = PixeColorFormat::ARGB_1555;
    resource->dataSize = size - sizeof(TgxHeader);
    resource->header = reinterpret_cast<TgxHeader*>(reinterpret_cast<uint8_t*>(resource.get()) + sizeof(TgxResource));
    resource->imageData = reinterpret_cast<uint8_t*>(resource->header) + sizeof(TgxHeader);

    // no further loading check, since TGX files do not rely on internal data

    Log(LogLevel::INFO, "Loaded TGX resource.");
    return resource;
  }

  void saveTgxResource(const std::filesystem::path& file, const TgxResource& resource)
  {
    Log(LogLevel::INFO, "Try saving TGX resource as TGX file.");

    std::filesystem::create_directories(file.parent_path());
    Log(LogLevel::DEBUG, "Created directories.");

    // inner block, to wrap file action
    {
      std::ofstream out;
      out.exceptions(std::ofstream::failbit | std::ofstream::badbit);
      out.open(file, std::ios::out | std::ios::trunc | std::ios::binary);

      out.write(reinterpret_cast<char*>(&resource.header->width), sizeof(uint32_t));
      out.write(reinterpret_cast<char*>(&resource.header->height), sizeof(uint32_t));
      out.write(reinterpret_cast<char*>(resource.imageData), resource.dataSize);
    }

    // small validation
    if(std::filesystem::file_size(file) != resource.base.resourceSize)
    {
      Log(LogLevel::ERROR, "Saved TGX resource as TGX file, but saved file has not expected size. Might be corrupted.");
      return;
    }
    else
    {
      Log(LogLevel::INFO, "Saved TGX resource as TGX file.");
    }
  }

  static ResourceMetaFormat::ResourceMetaFileReader readResourceMetaFile(const std::filesystem::path& folder, std::string_view resourceName)
  {
    try
    {
      std::filesystem::path file{ folder / resourceName };
      file.replace_extension(ResourceMetaFormat::FILE::EXTENSION);

      std::ifstream in;
      in.exceptions(std::ifstream::failbit | std::ifstream::badbit);
      in.open(file, std::ios::in); // text handling

      return ResourceMetaFormat::ResourceMetaFileReader::parseFrom(in);
    }
    catch (...)
    {
      Log(LogLevel::ERROR, "Failed to read resource meta file.");
      throw;
    }
  }

  static bool isVersionSupported(int version, std::span<const int> supportedVersions)
  {
    const auto it{ std::find(supportedVersions.begin(), supportedVersions.end(), version) };
    return it != supportedVersions.end();
  }

  UniqueTgxResourcePointer loadTgxResourceFromRaw(const std::filesystem::path& folder, const TgxCoderInstruction& instructions)
  {
    Log(LogLevel::INFO, "Try loading TGX resource from raw data.");
    if (!std::filesystem::is_directory(folder))
    {
      Log(LogLevel::ERROR, "Provided raw data folder path is not a directory.");
      return {};
    }

    const std::string resourceName{ folder.filename().string() };
    Log(LogLevel::DEBUG, "Using folder name '{}' as resource name.", resourceName);

    Log(LogLevel::DEBUG, "Loading resource meta file.");
    ResourceMetaFormat::ResourceMetaFileReader resourceMetaFile{ readResourceMetaFile(folder, resourceName) };
    Log(LogLevel::DEBUG, "Loaded resource meta file.");

    auto& resourceMetaObjects{ resourceMetaFile.getObjects() };
    if (resourceMetaObjects.size() != 2)
    {
      Log(LogLevel::ERROR, "Resource meta file has not expected number of objects.");
      return {};
    }


    Log(LogLevel::DEBUG, "Read TgxResource object from meta file.");
    const ResourceMetaFormat::ResourceMetaObjectReader&  tgxResourceMeta{ resourceMetaObjects.at(0) };
    if (tgxResourceMeta.getIdentifier() != TgxResourceMeta::RESOURCE_IDENTIFIER)
    {
      Log(LogLevel::ERROR, "Resource meta file does not have a {} object after the header.", TgxResourceMeta::RESOURCE_IDENTIFIER);
      return {};
    }
    if (!isVersionSupported(tgxResourceMeta.getVersion(), TgxResourceMeta::SUPPORTED_VERSIONS))
    {
      Log(LogLevel::ERROR, "{} object has no supported version.", TgxResourceMeta::RESOURCE_IDENTIFIER);
      return {};
    }
    // version currently ignored, since only one available

    const auto& tgxResourceEntries{ tgxResourceMeta.getMapEntries() };
    if (tgxResourceEntries.size() != 3)
    {
      Log(LogLevel::ERROR, "{} object has not expected number of entries.", TgxResourceMeta::RESOURCE_IDENTIFIER);
      return {};
    }

    auto it{ tgxResourceEntries.find(TgxResourceMeta::RAW_DATA_PATH_KEY) };
    if (it == tgxResourceEntries.end())
    {
      Log(LogLevel::ERROR, "{} object has not expected entry '{}'.", TgxResourceMeta::RESOURCE_IDENTIFIER, TgxResourceMeta::RAW_DATA_PATH_KEY);
      return {};
    }
    const std::string& relativeDataPath{ it->second };

    it = tgxResourceEntries.find(TgxResourceMeta::RAW_DATA_SIZE_KEY);
    if (it == tgxResourceEntries.end())
    {
      Log(LogLevel::ERROR, "{} object has not expected entry '{}'.", TgxResourceMeta::RESOURCE_IDENTIFIER, TgxResourceMeta::RAW_DATA_SIZE_KEY);
      return {};
    }
    const size_t rawDataSize{ uintFromStr<size_t>(it->second) };

    it = tgxResourceEntries.find(TgxResourceMeta::RAW_DATA_TRANSPARENT_PIXEL_KEY);
    if (it == tgxResourceEntries.end())
    {
      Log(LogLevel::ERROR, "{} object has not expected entry '{}'.", TgxResourceMeta::RESOURCE_IDENTIFIER, TgxResourceMeta::RAW_DATA_TRANSPARENT_PIXEL_KEY);
      return {};
    }
    const uint16_t transparentPixel{ uintFromStr<uint16_t>(it->second) };


    Log(LogLevel::DEBUG, "Read TgxHeader object from meta file.");
    const ResourceMetaFormat::ResourceMetaObjectReader& tgxHeaderMeta{ resourceMetaObjects.at(1) };
    if (tgxHeaderMeta.getIdentifier() != TgxHeaderMeta::RESOURCE_IDENTIFIER)
    {
      Log(LogLevel::ERROR, "Resource meta file does not have a {} object at last position.", TgxHeaderMeta::RESOURCE_IDENTIFIER);
      return {};
    }
    if (!isVersionSupported(tgxHeaderMeta.getVersion(), TgxHeaderMeta::SUPPORTED_VERSIONS))
    {
      Log(LogLevel::ERROR, "{} object has no supported version.", TgxHeaderMeta::RESOURCE_IDENTIFIER);
      return {};
    }
    // version currently ignored, since only one available

    const auto& tgxHeaderEntries{ tgxHeaderMeta.getListEntries() };
    if (tgxHeaderEntries.size() != 2)
    {
      Log(LogLevel::ERROR, "{} object has not expected number of entries.", TgxHeaderMeta::RESOURCE_IDENTIFIER);
      return {};
    }
    const int32_t width{ intFromStr<int32_t>(tgxHeaderEntries.at(0)) };
    const int32_t height{ intFromStr<int32_t>(tgxHeaderEntries.at(1)) };

    const std::filesystem::path fullDataPath{ folder / relativeDataPath };

    Log(LogLevel::DEBUG, "Validating certain values.");
    if (static_cast<size_t>(width) * height * sizeof(uint16_t) != rawDataSize)
    {
      Log(LogLevel::ERROR, "Dimensions in header do not match raw data size in meta file.");
      return {};
    }
    if (std::filesystem::file_size(fullDataPath) != rawDataSize)
    {
      Log(LogLevel::ERROR, "Size of raw data file do not match raw data size in meta file.");
      return {};
    }
    if (transparentPixel != instructions.transparentPixelRawColor)
    {
      Log(LogLevel::WARNING, "Transparent pixel in meta file does not match transparent pixel in coder instructions." 
        "This is valid, but might produce unexpected results. Set the coder options in the CLI if this is not wanted.");
    }

    auto rawData{ std::make_unique<uint16_t[]>(static_cast<size_t>(width) * height) };
    Log(LogLevel::DEBUG, "Loading raw data.");
    try {
      std::ifstream in;
      in.exceptions(std::ifstream::failbit | std::ifstream::badbit);
      in.open(fullDataPath, std::ios::in | std::ios::binary);
      in.read(reinterpret_cast<char*>(rawData.get()), rawDataSize);
    }
    catch (...)
    {
      Log(LogLevel::ERROR, "Failed to load raw data.");
      throw;
    }
    Log(LogLevel::DEBUG, "Loaded raw data.");

    const TgxCoderRawInfo rawInfo{
      .data{ rawData.get() },
      .rawWidth{ width },
      .rawHeight{ height },
      .rawX{ 0 },
      .rawY{ 0 }
    };
    TgxCoderTgxInfo tgxInfo{
      .colorType{ TgxColorType::DEFAULT },
      .data{ nullptr },
      .dataSize{ 0 },
      .tgxWidth{ width },
      .tgxHeight{ height }
    };

    Log(LogLevel::DEBUG, "Determine encoded size.");
    const TgxCoderResult dataSizeCheckResult{ encodeRawToTgx(&rawInfo, &tgxInfo, &instructions) };
    if (dataSizeCheckResult != TgxCoderResult::FILLED_ENCODING_SIZE)
    {
      Log(LogLevel::ERROR, "{}", std::string_view{ getTgxResultDescription(dataSizeCheckResult) });
      return {};
    }

    Log(LogLevel::DEBUG, "Create TGX resource.");
    const uint32_t resourceSize{ sizeof(TgxHeader) + tgxInfo.dataSize };
    UniqueTgxResourcePointer resource{ createWithAdditionalMemory<TgxResource>(resourceSize) };
    resource->base.type = SHCResourceType::SHC_RESOURCE_TGX;
    resource->base.resourceSize = resourceSize;
    resource->base.colorFormat = PixeColorFormat::ARGB_1555;
    resource->dataSize = tgxInfo.dataSize;
    resource->header = reinterpret_cast<TgxHeader*>(reinterpret_cast<uint8_t*>(resource.get()) + sizeof(TgxResource));
    resource->imageData = reinterpret_cast<uint8_t*>(resource->header) + sizeof(TgxHeader);
    resource->header->width = width;
    resource->header->height = height;

    Log(LogLevel::DEBUG, "Encode into TGX resource.");
    tgxInfo.data = resource->imageData;
    const TgxCoderResult encodingResult{ encodeRawToTgx(&rawInfo, &tgxInfo, &instructions) };
    if (encodingResult != TgxCoderResult::SUCCESS)
    {
      Log(LogLevel::ERROR, "{}", std::string_view{ getTgxResultDescription(encodingResult) });
      return {};
    }

    Log(LogLevel::INFO, "Loaded TGX resource from raw data.");
    return resource;
  }

  void saveTgxResourceAsRaw(const std::filesystem::path& folder, const TgxResource& resource, const TgxCoderInstruction& instructions)
  {
    Log(LogLevel::INFO, "Try saving TGX resource as raw data.");

    std::filesystem::create_directories(folder);
    Log(LogLevel::DEBUG, "Created directory.");

    auto rawData{ std::make_unique<uint16_t[]>(static_cast<size_t>(resource.header->width) * resource.header->height) };

    const TgxCoderTgxInfo tgxInfo{
      .data{ resource.imageData },
      .dataSize{ resource.dataSize },
      .tgxWidth{ resource.header->width },
      .tgxHeight{ resource.header->height }
    };
    TgxCoderRawInfo rawInfo{
      .data{ rawData.get() },
      .rawWidth{ resource.header->width },
      .rawHeight{ resource.header->height },
      .rawX{ 0 },
      .rawY{ 0 }
    };
    const TgxCoderResult result{ decodeTgxToRaw(&tgxInfo, &rawInfo, &instructions, nullptr) };
    if (result != TgxCoderResult::SUCCESS)
    {
      throw std::exception{ getTgxResultDescription(result) };
    }
    Log(LogLevel::DEBUG, "Decoded TGX to raw data.");

    const std::string resourceName{ folder.filename().string() };
    Log(LogLevel::DEBUG, "Using folder name '{}' as resource name.", resourceName);

    std::filesystem::path relativeDataPath{ resourceName };
    relativeDataPath.replace_extension(RAW_DATA_FILE_EXTENSION);

    const size_t rawDataSize{ static_cast<size_t>(resource.header->width) * resource.header->height * sizeof(uint16_t) };

    Log(LogLevel::DEBUG, "Creating resource meta file.");
    {
      std::filesystem::path file{ folder / resourceName };
      file.replace_extension(ResourceMetaFormat::FILE::EXTENSION);

      try {
        std::ofstream out;
        out.exceptions(std::ofstream::failbit | std::ofstream::badbit);
        out.open(file, std::ios::out | std::ios::trunc); // text handling

        ResourceMetaFormat::ResourceMetaFileWriter::startFile(out, ResourceMetaFormat::VERSION::CURRENT)
          .startHeader()
          .endObject()

          .startObject(TgxResourceMeta::RESOURCE_IDENTIFIER, TgxResourceMeta::CURRENT_VERSION)
          .writeMapEntry(TgxResourceMeta::RAW_DATA_PATH_KEY, relativeDataPath.string())
          .writeMapEntry(TgxResourceMeta::RAW_DATA_SIZE_KEY, std::to_string(rawDataSize))
          .writeMapEntry(TgxResourceMeta::RAW_DATA_TRANSPARENT_PIXEL_KEY, std::format("{:#06x}", instructions.transparentPixelRawColor),
            "Color used for transparent pixel during extract. Not automatically used during packing.")
          .endObject()

          .startObject(TgxHeaderMeta::RESOURCE_IDENTIFIER, TgxHeaderMeta::CURRENT_VERSION)
          .writeListEntry(std::to_string(resource.header->width), TgxHeaderMeta::COMMENT_WIDTH)
          .writeListEntry(std::to_string(resource.header->height), TgxHeaderMeta::COMMENT_HEIGHT)
          .endObject()

          .endFile();
      }
      catch (...) {
        Log(LogLevel::ERROR, "Encountered error while writing TGX resource meta file. File is likely corrupted.");
        throw;
      }
    }
    Log(LogLevel::DEBUG, "Created resource meta file.");

    Log(LogLevel::DEBUG, "Creating resource data file.");
    {
      const std::filesystem::path file{ folder / relativeDataPath };
      try
      {
        std::ofstream out;
        out.exceptions(std::ofstream::failbit | std::ofstream::badbit);
        out.open(file, std::ios::out | std::ios::trunc | std::ios::binary);
        out.write(reinterpret_cast<const char*>(rawData.get()), rawDataSize);
      }
      catch (...)
      {
        Log(LogLevel::ERROR, "Encountered error while writing TGX resource data file. File is likely corrupted.");
        throw;
      }
    }
    Log(LogLevel::DEBUG, "Created resource data file.");

    Log(LogLevel::INFO, "Saved TGX resource as raw data.");
  }
}
