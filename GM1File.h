#pragma once

#include "SHCResourceConverter.h"

#include "TGXCoder.h"
#include "Utility.h"

#include <memory>
#include <filesystem>

#include <stdint.h>
#include <format>

// adding formatter

template<>
struct std::formatter<Gm1Type> : public std::formatter<std::string>
{
  template<typename FormatContext>
  auto format(const Gm1Type& args, FormatContext& ctx) const
  {
    switch (args)
    {
    case Gm1Type::GM1_TYPE_INTERFACE:
      return format_to(ctx.out(), "INTERFACE");
    case Gm1Type::GM1_TYPE_ANIMATIONS:
      return format_to(ctx.out(), "ANIMATION");
    case Gm1Type::GM1_TYPE_TILES_OBJECT:
      return format_to(ctx.out(), "TILE_OBJECT");
    case Gm1Type::GM1_TYPE_FONT:
      return format_to(ctx.out(), "FONT");
    case Gm1Type::GM1_TYPE_NO_COMPRESSION_1:
      return format_to(ctx.out(), "UNCOMMPRESSED_1");
    case Gm1Type::GM1_TYPE_TGX_CONST_SIZE:
      return format_to(ctx.out(), "TGX");
    case Gm1Type::GM1_TYPE_NO_COMPRESSION_2:
      return format_to(ctx.out(), "NO_COMPRESSION_2");

    default:
      return format_to(ctx.out(), "UNKNOWN");
    }
  }
};

template<>
struct std::formatter<Gm1Header> : public std::formatter<std::string>
{
  template<typename FormatContext>
  auto format(const Gm1Header& args, FormatContext& ctx) const
  {
    return format_to(ctx.out(),
      "Unknown 0x0: {}\n"
      "Unknown 0x4: {}\n"
      "Unknown 0x8: {}\n"
      "Number of Pictures: {}\n"
      "Unknown 0x10: {}\n"
      "GM1 Type: {}\n"
      "Unknown 0x18: {}\n"
      "Unknown 0x1C: {}\n"
      "Unknown 0x20: {}\n"
      "Unknown 0x24: {}\n"
      "Unknown 0x28: {}\n"
      "Unknown 0x2C: {}\n"
      "Width: {}\n"
      "Height: {}\n"
      "Unknown 0x38: {}\n"
      "Unknown 0x3C: {}\n"
      "Unknown 0x40: {}\n"
      "Unknown 0x44: {}\n"
      "Origin X: {}\n"
      "Origin Y: {}\n"
      "Data Size: {}\n"
      "Unknown 0x54: {}\n"
      "Omitting Palettes",
      args.info.unknown_0x0,
      args.info.unknown_0x4,
      args.info.unknown_0x8,
      args.info.numberOfPicturesInFile,
      args.info.unknown_0x10,
      args.info.gm1Type,
      args.info.unknown_0x18,
      args.info.unknown_0x1C,
      args.info.unknown_0x20,
      args.info.unknown_0x24,
      args.info.unknown_0x28,
      args.info.unknown_0x2C,
      args.info.width,
      args.info.height,
      args.info.unknown_0x38,
      args.info.unknown_0x3C,
      args.info.unknown_0x40,
      args.info.unknown_0x44,
      args.info.originX,
      args.info.originY,
      args.info.dataSize,
      args.info.unknown_0x54
    );
  }
};

template<>
struct std::formatter<Gm1ImageHeader> : public std::formatter<std::string>
{
  template<typename FormatContext>
  auto format(const Gm1ImageHeader& args, FormatContext& ctx) const
  {
    return format_to(ctx.out(),
      "Width: {}\n"
      "Height: {}\n"
      "Offset X: {}\n"
      "Offset Y: {}",
      args.width,
      args.height,
      args.offsetX,
      args.offsetY
    );
  }
};

template<>
struct std::formatter<Gm1TileObjectImagePosition> : public std::formatter<std::string>
{
  template<typename FormatContext>
  auto format(const Gm1TileObjectImagePosition& args, FormatContext& ctx) const
  {
    switch (args)
    {
    case Gm1TileObjectImagePosition::NONE:
      return format_to(ctx.out(), "NONE");
    case Gm1TileObjectImagePosition::TOP:
      return format_to(ctx.out(), "TOP");
    case Gm1TileObjectImagePosition::UPPER_LEFT:
      return format_to(ctx.out(), "UPPER_LEFT");
    case Gm1TileObjectImagePosition::UPPER_RIGHT:
      return format_to(ctx.out(), "UPPER_RIGHT");

    default:
      return format_to(ctx.out(), "UNKNOWN");
    }
  }
};

template<>
struct std::formatter<Gm1TileObjectImageInfo> : public std::formatter<std::string>
{
  template<typename FormatContext>
  auto format(const Gm1TileObjectImageInfo& args, FormatContext& ctx) const
  {
    return format_to(ctx.out(),
      "Image Part Index: {}\n"
      "Number of Image Parts: {}\n"
      "Tile Offset: {}\n"
      "Image Position: {}\n"
      "Image Offset X: {}\n"
      "Image Width: {}\n"
      "Flags: {:08b}",
      args.imagePart,
      args.subParts,
      args.tileOffset,
      args.imagePosition,
      args.imageOffsetX,
      args.imageWidth,
      args.flags
    );
  }
};

template<>
struct std::formatter<Gm1GeneralImageInfo> : public std::formatter<std::string>
{
  template<typename FormatContext>
  auto format(const Gm1GeneralImageInfo& args, FormatContext& ctx) const
  {
    return format_to(ctx.out(),
      "Alternative Data Relative Position: {}\n"
      "Font Related Size: {}\n"
      "Unknown 0x4: {}\n"
      "Unknown 0x5: {}\n"
      "Unknown 0x6: {}\n"
      "Flags: {:08b}",
      args.relativeDataPos,
      args.fontRelatedSize,
      args.unknown_0x4,
      args.unknown_0x5,
      args.unknown_0x6,
      args.flags
    );
  }
};


// actual file data

namespace GM1File
{
  using UniqueGm1ResourcePointer = std::unique_ptr<Gm1Resource, ObjectWithAdditionalMemoryDeleter<Gm1Resource>>;

  inline constexpr std::string_view FILE_EXTENSION{ ".gm1" };
  inline constexpr std::uintmax_t MIN_FILE_SIZE{ sizeof(Gm1Header) }; // guess
  inline constexpr std::uintmax_t MAX_FILE_SIZE{ std::numeric_limits<uint32_t>::max() }; // setting limit

  inline constexpr std::string_view RAW_DATA_FILE_EXTENSION{ ".data" };

  inline constexpr std::string_view PALETTE_FILE_EXTENSION{ ".palette" };
  inline constexpr int PALETTE_COUNT{ 10 };
  inline constexpr int PALETTE_SIZE{ 512 };

  namespace Gm1ResourceMeta
  {
    inline constexpr std::string_view RESOURCE_IDENTIFIER{ "Gm1Resource" };
    inline constexpr int CURRENT_VERSION{ 1 };
    inline constexpr int SUPPORTED_VERSIONS[]{ 1 };

    inline constexpr int MAP_ENTRIES{ 5 };
    inline constexpr int LIST_ENTRIES{ 0 };

    inline constexpr std::string_view RAW_DATA_PATH_KEY{ "data path" };
    inline constexpr std::string_view RAW_DATA_SIZE_KEY{ "data size" };
    inline constexpr std::string_view RAW_DATA_TRANSPARENT_PIXEL_KEY{ "transparent pixel" };
    inline constexpr std::string_view RAW_DATA_WIDTH_KEY{ "data width" };
    inline constexpr std::string_view RAW_DATA_HEIGHT_KEY{ "data height" };
  }

  namespace Gm1HeaderMeta
  {
    inline constexpr std::string_view RESOURCE_IDENTIFIER{ "Gm1HeaderMeta" };
    inline constexpr int CURRENT_VERSION{ 1 };
    inline constexpr int SUPPORTED_VERSIONS[]{ 1 };

    inline constexpr int MAP_ENTRIES{ 0 };
    inline constexpr int LIST_ENTRIES{ 22 };

    inline constexpr std::string_view COMMENT_UNKNOWN_0x0{ "unknown 0x0" };
    inline constexpr std::string_view COMMENT_UNKNOWN_0x4{ "unknown 0x4" };
    inline constexpr std::string_view COMMENT_UNKNOWN_0x8{ "unknown 0x8" };
    inline constexpr std::string_view COMMENT_NUMBER_OF_PICTURES_IN_FILE{ "number of pictures in file" };
    inline constexpr std::string_view COMMENT_UNKNOWN_0x10{ "unknown 0x10" };
    inline constexpr std::string_view COMMENT_GM1_TYPE{ "gm1 type" };
    inline constexpr std::string_view COMMENT_UNKNOWN_0x18{ "unknown 0x18" };
    inline constexpr std::string_view COMMENT_UNKNOWN_0x1C{ "unknown 0x1C" };
    inline constexpr std::string_view COMMENT_UNKNOWN_0x20{ "unknown 0x20" };
    inline constexpr std::string_view COMMENT_UNKNOWN_0x24{ "unknown 0x24" };
    inline constexpr std::string_view COMMENT_UNKNOWN_0x28{ "unknown 0x28" };
    inline constexpr std::string_view COMMENT_UNKNOWN_0x2C{ "unknown 0x2C" };
    inline constexpr std::string_view COMMENT_WIDTH{ "width" };
    inline constexpr std::string_view COMMENT_HEIGHT{ "height" };
    inline constexpr std::string_view COMMENT_UNKNOWN_0x38{ "unknown 0x38" };
    inline constexpr std::string_view COMMENT_UNKNOWN_0x3C{ "unknown 0x3C" };
    inline constexpr std::string_view COMMENT_UNKNOWN_0x40{ "unknown 0x40" };
    inline constexpr std::string_view COMMENT_UNKNOWN_0x44{ "unknown 0x44" };
    inline constexpr std::string_view COMMENT_ORIGIN_X{ "origin x" };
    inline constexpr std::string_view COMMENT_ORIGIN_Y{ "origin y" };
    inline constexpr std::string_view COMMENT_DATA_SIZE{ "data size" };
    inline constexpr std::string_view COMMENT_UNKNOWN_0x54{ "unknown 0x54" };
  }

  namespace Gm1ImageHeaderMeta
  {
    inline constexpr std::string_view RESOURCE_IDENTIFIER{ "Gm1ImageHeader" };
    inline constexpr int CURRENT_VERSION{ 1 };
    inline constexpr int SUPPORTED_VERSIONS[]{ 1 };

    inline constexpr int MAP_ENTRIES{ 2 };
    inline constexpr int LIST_ENTRIES{ 4 };

    inline constexpr std::string_view OFFSET_KEY{ "data offset" };
    inline constexpr std::string_view SIZE_KEY{ "data size" };

    inline constexpr std::string_view COMMENT_WIDTH{ "width" };
    inline constexpr std::string_view COMMENT_HEIGHT{ "height" };
    inline constexpr std::string_view COMMENT_OFFSET_X{ "offset x" };
    inline constexpr std::string_view COMMENT_OFFSET_Y{ "offset y" };
  }

  namespace Gm1TileObjectImageInfoMeta
  {
    inline constexpr std::string_view RESOURCE_IDENTIFIER{ "Gm1TileObjectImageInfo" };
    inline constexpr int CURRENT_VERSION{ 1 };
    inline constexpr int SUPPORTED_VERSIONS[]{ 1 };

    inline constexpr int MAP_ENTRIES{ 0 };
    inline constexpr int LIST_ENTRIES{ 7 };

    inline constexpr std::string_view COMMENT_IMAGE_PART{ "image part" };
    inline constexpr std::string_view COMMENT_SUB_PARTS{ "sub parts" };
    inline constexpr std::string_view COMMENT_TILE_OFFSET{ "tile offset" };
    inline constexpr std::string_view COMMENT_IMAGE_POSITION{ "image position" };
    inline constexpr std::string_view COMMENT_IMAGE_OFFSET_X{ "image offset x" };
    inline constexpr std::string_view COMMENT_IMAGE_WIDTH{ "image width" };
    inline constexpr std::string_view COMMENT_FLAGS{ "flags" };
  }

  namespace Gm1GeneralImageInfoMeta
  {
    inline constexpr std::string_view RESOURCE_IDENTIFIER{ "Gm1GeneralImageInfo" };
    inline constexpr int CURRENT_VERSION{ 1 };
    inline constexpr int SUPPORTED_VERSIONS[]{ 1 };

    inline constexpr int MAP_ENTRIES{ 0 };
    inline constexpr int LIST_ENTRIES{ 6 };

    inline constexpr std::string_view COMMENT_RELATIVE_DATA_POS{ "relative data position" };
    inline constexpr std::string_view COMMENT_FONT_RELATED_SIZE{ "font related size" };
    inline constexpr std::string_view COMMENT_UNKNOWN_0x4{ "unknown 0x4" };
    inline constexpr std::string_view COMMENT_UNKNOWN_0x5{ "unknown 0x5" };
    inline constexpr std::string_view COMMENT_UNKNOWN_0x6{ "unknown 0x6" };
    inline constexpr std::string_view COMMENT_FLAGS{ "flags" };
  }

  // basically the hight the image is "sunk" into the tile, and it seems to be a constant in the game
  inline constexpr int TILE_IMAGE_HEIGHT_OFFSET{ 7 };

  void validateGm1Resource(const Gm1Resource& resource, const TgxCoderInstruction& instructions, bool tgxAsText);

  UniqueGm1ResourcePointer loadGm1Resource(const std::filesystem::path& file);
  void saveGm1Resource(const std::filesystem::path& file, const Gm1Resource& resource);

  UniqueGm1ResourcePointer loadGm1ResourceFromRaw(const std::filesystem::path& folder);
  void saveGm1ResourceAsRaw(const std::filesystem::path& folder, const Gm1Resource& resource, const TgxCoderInstruction& instructions);
}
