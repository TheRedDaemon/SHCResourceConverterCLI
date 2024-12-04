#pragma once

#include "SHCResourceConverter.h"

#include <memory>
#include <filesystem>

namespace GM1File
{
  inline constexpr std::string_view FILE_EXTENSION{ ".gm1" };

  void validateGm1Resource(const Gm1Resource& resource);

  std::unique_ptr<Gm1Resource> loadGm1Resource(const std::filesystem::path& file);
  void saveGm1Resource(const std::filesystem::path& file, const Gm1Resource& resource);

  std::unique_ptr<Gm1Resource> loadGm1ResourceFromRaw(const std::filesystem::path& folder);
  void saveGm1ResourceAsRaw(const std::filesystem::path& folder, const Gm1Resource& resource);
}
