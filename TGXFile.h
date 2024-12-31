#pragma once

#include "SHCResourceConverter.h"

#include "Utility.h"
#include "TgxCoder.h"

#include <memory>
#include <filesystem>

namespace TGXFile
{
  using UniqueTgxResourcePointer = std::unique_ptr<TgxResource, ObjectWithAdditionalMemoryDeleter<TgxResource>>;

  inline constexpr std::string_view FILE_EXTENSION{ ".tgx" };
  inline constexpr std::uintmax_t MIN_FILE_SIZE{ sizeof(TgxHeader) }; // guess
  inline constexpr std::uintmax_t MAX_FILE_SIZE{ std::numeric_limits<uint32_t>::max() }; // setting limit

  inline constexpr std::string_view RAW_DATA_FILE_EXTENSION{ ".data" };

  namespace TgxResourceMeta
  {
    inline constexpr std::string_view RESOURCE_IDENTIFIER{ "TgxResource" };
    inline constexpr int CURRENT_VERSION{ 1 };
    inline constexpr int SUPPORTED_VERSIONS[]{ 1 };

    inline constexpr std::string_view RAW_DATA_PATH_KEY{ "data path" };
    inline constexpr std::string_view RAW_DATA_SIZE_KEY{ "data size" };
    inline constexpr std::string_view RAW_DATA_TRANSPARENT_PIXEL_KEY{ "transparent pixel" };
  }

  namespace TgxHeaderMeta
  {
    inline constexpr std::string_view RESOURCE_IDENTIFIER{ "TgxHeader" };
    inline constexpr int CURRENT_VERSION{ 1 };
    inline constexpr int SUPPORTED_VERSIONS[]{ 1 };

    inline constexpr std::string_view COMMENT_WIDTH{ "width" };
    inline constexpr std::string_view COMMENT_HEIGHT{ "height" };
  }

  void validateTgxResource(const TgxResource& resource, bool tgxAsText);

  UniqueTgxResourcePointer loadTgxResource(const std::filesystem::path& file);
  void saveTgxResource(const std::filesystem::path& file, const TgxResource& resource);

  UniqueTgxResourcePointer loadTgxResourceFromRaw(const std::filesystem::path& folder, const TgxCoderInstruction& instructions);
  void saveTgxResourceAsRaw(const std::filesystem::path& folder, const TgxResource& resource, const TgxCoderInstruction& instructions);
}
