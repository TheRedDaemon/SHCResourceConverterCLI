#pragma once

#include <stdint.h>
#include <format>


enum class Gm1CoderResult : int32_t
{
  SUCCESS,
  CHECKED_PARAMETER,
  FILLED_ENCODING_SIZE,
  MISSING_REQUIRED_PARAMETER,
  CANVAS_CAN_NOT_CONTAIN_IMAGE,
  EXPECTED_TRANSPARENT_PIXEL,
  INVALID_DATA_SIZE,
};

struct Gm1CoderRawInfo
{
  uint16_t* raw; // it is assumed the buffer is big enough to contain the data indicated by this and the data struct
  int rawWidth;
  int rawHeight; // unused in coder, but might be handy
  int rawX;
  int rawY;
};

struct Gm1CoderDataInfo
{
  uint8_t* data;
  uint32_t dataSize;
  int32_t dataWidth;
  int32_t dataHeight;
};

template<>
struct std::formatter<Gm1CoderRawInfo> : public std::formatter<std::string>
{
  template<typename FormatContext>
  auto format(const Gm1CoderRawInfo& args, FormatContext& ctx) const
  {
    return format_to(ctx.out(), "Raw Width: {}\nRaw Height: {}\nRaw X: {}\nRaw Y: {}", args.rawWidth, args.rawHeight, args.rawX, args.rawY);
  }
};

template<>
struct std::formatter<Gm1CoderDataInfo> : public std::formatter<std::string>
{
  template<typename FormatContext>
  auto format(const Gm1CoderDataInfo& args, FormatContext& ctx) const
  {
    return format_to(ctx.out(), "Data Size: {}\nData Width: {}\nData Height: {}", args.dataSize, args.dataWidth, args.dataHeight);
  }
};

constexpr uint16_t RAW_TRANSPARENT{ 0 }; // for placing transparency and identification of it

constexpr int TILE_BYTE_SIZE{ 0x200 };
constexpr int TILE_WIDTH{ 30 };
constexpr int TILE_HEIGHT{ 16 };

// The following functions provide a check run that will not necessarily run the logic, but will check the parameters for validity
// In all these cases, the Ptr to be written too needs to be a nullptr, and the result will be CHECKED_PARAMETER

// tiles have a stable size of 512 bytes, so the tile ptr always assumes to provide this size, the assumed canvas is also always 30x16 pixels
extern "C" __declspec(dllexport) Gm1CoderResult decodeTileToRaw(const uint16_t* tile, Gm1CoderRawInfo* raw, uint16_t transparentColor);
extern "C" __declspec(dllexport) Gm1CoderResult encodeRawToTile(const Gm1CoderRawInfo* raw, uint16_t* tile, uint16_t transparentColor);

// simply copies the given uncompressed data to raw, not much checks are done
extern "C" __declspec(dllexport) Gm1CoderResult copyUncompressedToRaw(const Gm1CoderDataInfo* uncompressed, Gm1CoderRawInfo* raw, uint16_t transparentColor);
extern "C" __declspec(dllexport) Gm1CoderResult copyRawToUncompressed(const Gm1CoderRawInfo* raw, Gm1CoderDataInfo* uncompressed, uint16_t transparentColor);

// get a string description of the result, never returns nullptr
extern "C" __declspec(dllexport) const char* getGm1ResultDescription(const Gm1CoderResult result);
