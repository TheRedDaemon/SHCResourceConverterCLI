#include "TGXFile.h"

#include "Logger.h"

#include <fstream>

namespace TGXFile
{
  void validateTgxResource(const TgxResource& resource)
  {
    throw std::exception{ "Not yet implemented." };
  }

  UniqueTgxResourcePointer loadTgxResource(const std::filesystem::path& file)
  {
    Log(LogLevel::INFO, "Try loading provided file.");
    if (!std::filesystem::is_regular_file(file))
    {
      Log(LogLevel::ERROR, "Provided file is not a regular file.");
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
      Log(LogLevel::ERROR, "Provided file is too big to be handled by this implementation.");
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
    throw std::exception{ "Not yet implemented." };
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
