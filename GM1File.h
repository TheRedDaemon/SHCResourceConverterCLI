#pragma once

#include "SHCResourceConverter.h"

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
      args.unknown_0x0,
      args.unknown_0x4,
      args.unknown_0x8,
      args.numberOfPicturesInFile,
      args.unknown_0x10,
      args.gm1Type,
      args.unknown_0x18,
      args.unknown_0x1C,
      args.unknown_0x20,
      args.unknown_0x24,
      args.unknown_0x28,
      args.unknown_0x2C,
      args.width,
      args.height,
      args.unknown_0x38,
      args.unknown_0x3C,
      args.unknown_0x40,
      args.unknown_0x44,
      args.originX,
      args.originY,
      args.dataSize,
      args.unknown_0x54
    );
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
      "Image Direction: {}\n"
      "Horizontal Offset of Image: {}\n"
      "Building Width: {}\n"
      "Animated Color: {}",
      args.imagePart,
      args.subParts,
      args.tileOffset,
      args.direction,
      args.horizontalOffsetOfImage,
      args.buildingWidth,
      args.animatedColor
    );
  }
};

template<>
struct std::formatter<Gm1AnimationImageInfo> : public std::formatter<std::string>
{
  template<typename FormatContext>
  auto format(const Gm1AnimationImageInfo& args, FormatContext& ctx) const
  {
    return format_to(ctx.out(),
      "Alternative Data Relative Position: {}\n"
      "Unknown 0x2: {}\n"
      "Unknown 0x3: {}\n"
      "Unknown 0x4: {}\n"
      "Unknown 0x5: {}\n"
      "Unknown 0x6: {}\n"
      "Flags: {:b}",
      args.relativeDataPos,
      args.unknown_0x2,
      args.unknown_0x3,
      args.unknown_0x4,
      args.unknown_0x5,
      args.unknown_0x6,
      args.flags
    );
  }
};

template<>
struct std::formatter<Gm1FontImageInfo> : public std::formatter<std::string>
{
  template<typename FormatContext>
  auto format(const Gm1FontImageInfo& args, FormatContext& ctx) const
  {
    return format_to(ctx.out(),
      "Unknown 0x0: {}\n"
      "Unknown 0x1: {}\n"
      "Font Related Size: {}\n"
      "Unknown 0x4: {}\n"
      "Unknown 0x5: {}\n"
      "Unknown 0x6: {}\n"
      "Unknown 0x7: {}\n",
      args.unknown_0x0,
      args.unknown_0x1,
      args.fontRelatedSize,
      args.unknown_0x4,
      args.unknown_0x5,
      args.unknown_0x6,
      args.unknown_0x7
    );
  }
};

template<>
struct std::formatter<Gm1UnknownImageInfo> : public std::formatter<std::string>
{
  template<typename FormatContext>
  auto format(const Gm1UnknownImageInfo& args, FormatContext& ctx) const
  {
    return format_to(ctx.out(),
      "Unknown 0x0: {}\n"
      "Unknown 0x1: {}\n"
      "Unknown 0x2: {}\n"
      "Unknown 0x3: {}\n"
      "Unknown 0x4: {}\n"
      "Unknown 0x5: {}\n"
      "Unknown 0x6: {}\n"
      "Unknown 0x7: {}\n",
      args.unknown_0x0,
      args.unknown_0x1,
      args.unknown_0x2,
      args.unknown_0x3,
      args.unknown_0x4,
      args.unknown_0x5,
      args.unknown_0x6,
      args.unknown_0x7
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

  void validateGm1Resource(const Gm1Resource& resource);

  UniqueGm1ResourcePointer loadGm1Resource(const std::filesystem::path& file);
  void saveGm1Resource(const std::filesystem::path& file, const Gm1Resource& resource);

  UniqueGm1ResourcePointer loadGm1ResourceFromRaw(const std::filesystem::path& folder);
  void saveGm1ResourceAsRaw(const std::filesystem::path& folder, const Gm1Resource& resource);
}
