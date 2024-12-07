#include "TGXFile.h"

#include "Console.h"
#include "ResourceMetaFormat.h"

#include <fstream>

namespace TGXFile
{
  void validateTgxResource(const TgxResource& resource, const TgxCoderInstruction& instructions)
  {
    Log(LogLevel::INFO, "Try validating given resource.");
    
    const TgxCoderTgxInfo tgxInfo{
      .data{ resource.imageData },
      .dataSize{ resource.dataSize },
      .tgxWidth{ resource.header->width },
      .tgxHeight{ resource.header->height }
    };

    Out("### General TGX info ###\n{}\n\n", tgxInfo);
    TgxAnalysis tgxAnalysis{};
    const TgxCoderResult result{ analyzeTgxToRaw(&tgxInfo, &instructions, &tgxAnalysis) };
    if (result == TgxCoderResult::SUCCESS)
    {
      Out("### Structure meta data ###\n{}\n\n### TGX seems valid ###\n", tgxAnalysis);
      Log(LogLevel::INFO, "Validation completed successfully.");
      return;
    }

    Out("{}\n", std::string_view{ getTgxResultDescription(result) });
    Out("### TGX seems invalid. ###\n");
    Log(LogLevel::ERROR, "Validation completed. TGX is invalid.");
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

  UniqueTgxResourcePointer loadTgxResourceFromRaw(const std::filesystem::path& folder)
  {
    throw std::exception{ "Not yet implemented." };
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
