#pragma once

#include "SHCResourceConverter.h"

#include "Utility.h"

#include <memory>
#include <filesystem>

namespace TGXFile
{
  using UniqueTgxResourcePointer = std::unique_ptr<TgxResource, ObjectWithAdditionalMemoryDeleter<TgxResource>>;

  inline constexpr std::string_view FILE_EXTENSION{ ".tgx" };
  inline constexpr std::uintmax_t MIN_FILE_SIZE{ sizeof(TgxHeader) }; // guess
  inline constexpr std::uintmax_t MAX_FILE_SIZE{ std::numeric_limits<uint32_t>::max() }; // setting limit

  void validateTgxResource(const TgxResource& resource);

  UniqueTgxResourcePointer loadTgxResource(const std::filesystem::path& file);
  void saveTgxResource(const std::filesystem::path& file, const TgxResource& resource);

  UniqueTgxResourcePointer loadTgxResourceFromRaw(const std::filesystem::path& folder);
  void saveTgxResourceAsRaw(const std::filesystem::path& folder, const TgxResource& resource);
}
