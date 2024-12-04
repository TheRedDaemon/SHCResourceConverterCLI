#pragma once

#include "SHCResourceConverter.h"

#include "Utility.h"

#include <memory>
#include <filesystem>

namespace GM1File
{
  using UniqueGm1ResourcePointer = std::unique_ptr<Gm1Resource, ObjectWithAdditionalMemoryDeleter<Gm1Resource>>;

  inline constexpr std::string_view FILE_EXTENSION{ ".gm1" };
  inline constexpr std::uintmax_t MIN_FILE_SIZE{ sizeof(Gm1Header) }; // guess
  inline constexpr std::uintmax_t MAX_FILE_SIZE{ std::numeric_limits<uint32_t>::max() }; // setting limit

  void validateGm1Resource(const Gm1Resource& resource);

  UniqueGm1ResourcePointer loadGm1Resource(const std::filesystem::path& file);
  void saveGm1Resource(const std::filesystem::path& file, const Gm1Resource& resource);

  UniqueGm1ResourcePointer loadGm1ResourceFromRaw(const std::filesystem::path& folder);
  void saveGm1ResourceAsRaw(const std::filesystem::path& folder, const Gm1Resource& resource);
}
