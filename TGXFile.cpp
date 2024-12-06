#include "TGXFile.h"

#include "Console.h"

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

    switch (result)
    {
    case TgxCoderResult::WIDTH_TOO_BIG:
      Out("Decoding had line with bigger width than said in header.\n");
      break;
    case TgxCoderResult::HEIGHT_TOO_BIG:
      Out("Decoding had bigger height than said in header.\n");
      break;
    case TgxCoderResult::UNKNOWN_MARKER:
      Out("Encountered an unknown marker in the encoded data.\n");
      break;
    case TgxCoderResult::INVALID_TGX_DATA_SIZE:
      Out("Decoder attempted to run beyond the given TGX data. Data likely invalid or incomplete.\n");
      break;
    case TgxCoderResult::TGX_HAS_NOT_ENOUGH_PIXELS:
      Out("Decoder produced an image with less pixels than required by the dimensions in the header.\n");
      break;
    default:
      Out("Encountered unknown decoder analysis result. Please report this as bug.\n");
      break;
    }
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
    Log(LogLevel::INFO, "Created directories.");

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

  void saveTgxResourceAsRaw(const std::filesystem::path& folder, const TgxResource& resource)
  {
    throw std::exception{ "Not yet implemented." };
  }
}
