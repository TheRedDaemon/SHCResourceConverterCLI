#pragma once

#include "SHCResourceConverter.h"

#include <memory>
#include <filesystem>

namespace TGXFile
{
  inline constexpr std::string_view FILE_EXTENSION{ ".tgx" };

  void validateTgxResource(const TgxResource& resource);

  std::unique_ptr<TgxResource> loadTgxResource(const std::filesystem::path& file);
  void saveTgxResource(const std::filesystem::path& file, const TgxResource& resource);

  std::unique_ptr<TgxResource> loadTgxResourceFromRaw(const std::filesystem::path& folder);
  void saveTgxResourceAsRaw(const std::filesystem::path& folder, const TgxResource& resource);
}
