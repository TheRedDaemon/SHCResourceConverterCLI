#pragma once

#include <stdint.h>
#include <format>

enum class TgxCoderResult : int32_t
{
  SUCCESS,
  FILLED_ENCODING_SIZE,
  MISSING_REQUIRED_STRUCTS,
  UNKNOWN_MARKER,
  WIDTH_TOO_BIG,
  HEIGHT_TOO_BIG,
  INVALID_TGX_DATA_SIZE,
  TGX_HAS_NOT_ENOUGH_PIXELS,
  RAW_WIDTH_TOO_SMALL,
};

enum class TgxColorType : int32_t
{
  DEFAULT,
  INDEXED, // uses a color table to actually store the colors, leading to using 8 bit per actual pixel
};

struct TgxCoderRawInfo
{
  uint16_t* data; // it is assumed the buffer is big enough to contain the data indicated by this and the tgx struct
  int rawWidth;
  int rawHeight; // unused in coder, but might be handy
  int rawX;
  int rawY;
};

struct TgxCoderTgxInfo
{
  TgxColorType colorType;
  uint8_t* data;
  uint32_t dataSize;
  int32_t tgxWidth;
  int32_t tgxHeight;
};

struct TgxCoderInstruction
{
  uint16_t transparentPixelTgxColor; // the game uses a certain color to also indicate transparency 
  uint16_t transparentPixelRawColor; // used to decode pixels that are indicated by the transparency header
  int pixelRepeatThreshold;
  int paddingAlignment;
};

struct TgxAnalysis
{
  int markerCountPixelStream{ 0 };
  int pixelStreamPixelCount{ 0 };
  int markerCountTransparent{ 0 };
  int transparentPixelCount{ 0 };
  int markerCountRepeatingPixels{ 0 };
  int repeatingPixelsPixelCount{ 0 };
  int markerCountNewline{ 0 };
  int unfinishedWidthPixelCount{ 0 };
  int newlineWithoutMarkerCount{ 0 };
  int paddingNewlineMarkerCount{ 0 };
};

template<>
struct std::formatter<TgxCoderRawInfo> : public std::formatter<std::string>
{
  template<typename FormatContext>
  auto format(const TgxCoderRawInfo& args, FormatContext& ctx) const
  {
    return format_to(ctx.out(), "Raw Width: {}\nRaw Height: {}\nRaw X: {}\nRaw Y: {}",
      args.rawWidth, args.rawHeight, args.rawX, args.rawY);
  }
};

template<>
struct std::formatter<TgxColorType> : public std::formatter<std::string>
{
  template<typename FormatContext>
  auto format(const TgxColorType& args, FormatContext& ctx) const
  {
    switch (args)
    {
    case TgxColorType::DEFAULT:
      return format_to(ctx.out(), "DEFAULT");
    case TgxColorType::INDEXED:
      return format_to(ctx.out(), "INDEXED");

    default:
      return format_to(ctx.out(), "UNKNOWN");
    }
  }
};

template<>
struct std::formatter<TgxCoderTgxInfo> : public std::formatter<std::string>
{
  template<typename FormatContext>
  auto format(const TgxCoderTgxInfo& args, FormatContext& ctx) const
  {
    return format_to(ctx.out(), "Color Type: {}\nData Size: {}\nTGX Width: {}\nTGX Height: {}",
      args.colorType, args.dataSize, args.tgxWidth, args.tgxHeight);
  }
};

template<>
struct std::formatter<TgxCoderInstruction> : public std::formatter<std::string>
{
  template<typename FormatContext>
  auto format(const TgxCoderInstruction& args, FormatContext& ctx) const
  {
    return format_to(ctx.out(),
      "Transparent Pixel TGX Color: {:#06x}\n"
      "Transparent Pixel Raw Color: {:#06x}\n"
      "Pixel Repeat Threshold: {}\n"
      "Padding Alignment: {}",
      args.transparentPixelTgxColor,
      args.transparentPixelRawColor,
      args.pixelRepeatThreshold,
      args.paddingAlignment);
  }
};

template<>
struct std::formatter<TgxAnalysis> : public std::formatter<std::string>
{
  template<typename FormatContext>
  auto format(const TgxAnalysis& args, FormatContext& ctx) const
  {
    return format_to(ctx.out(),
      "Marker Count Pixel Stream: {}\n"
      "Pixel Stream Pixel Count: {}\n"
      "Marker Count Transparent: {}\n"
      "Transparent Pixel Count: {}\n"
      "Marker Count Repeating Pixels: {}\n"
      "Repeating Pixels Pixel Count: {}\n"
      "Marker Count Newline: {}\n"
      "Unfinished Width Pixel Count: {}\n"
      "Newline Without Marker Count: {}\n"
      "Padding Newline Marker Count: {}",
      args.markerCountPixelStream,
      args.pixelStreamPixelCount,
      args.markerCountTransparent,
      args.transparentPixelCount,
      args.markerCountRepeatingPixels,
      args.repeatingPixelsPixelCount,
      args.markerCountNewline,
      args.unfinishedWidthPixelCount,
      args.newlineWithoutMarkerCount,
      args.paddingNewlineMarkerCount);
  }
};

// defined for transformation
constexpr uint16_t GAME_TRANSPARENT_COLOR{ 0b1111100000011111 }; // used by game for some cases (repeating pixels seem excluded?)
constexpr uint16_t TGX_FILE_TRANSPARENT{ 0 }; // for placing transparency and identification of it
constexpr int TGX_FILE_PIXEL_REPEAT_THRESHOLD{ 3 }; // requires testing with other files
constexpr int TGX_FILE_PADDING_ALIGNMENT{ 4 }; // requires testing with other files
inline constexpr TgxCoderInstruction TGX_FILE_DEFAULT_INSTRUCTION{
  .transparentPixelTgxColor{ GAME_TRANSPARENT_COLOR },
  .transparentPixelRawColor{ TGX_FILE_TRANSPARENT },
  .pixelRepeatThreshold{ TGX_FILE_PIXEL_REPEAT_THRESHOLD },
  .paddingAlignment{ TGX_FILE_PADDING_ALIGNMENT },
};

extern "C" __declspec(dllexport) TgxCoderResult analyzeTgxToRaw(const TgxCoderTgxInfo* tgxData, TgxAnalysis* tgxAnalysis);
extern "C" __declspec(dllexport) TgxCoderResult decodeTgxToRaw(const TgxCoderTgxInfo* tgxData, TgxCoderRawInfo* rawData, TgxAnalysis* tgxAnalysis);

// fills the dataSize in TgxCoderTgxInfo if data ptr is nullptr and return different result, else it will fill the buffer, but stop if dataSize indicates that the buffer is too small
// resulting in a broken result; if the buffer size indicated by dataSize is big enough, the dataSize will be set to the actual size
extern "C" __declspec(dllexport) TgxCoderResult encodeRawToTgx(const TgxCoderRawInfo* rawData, TgxCoderTgxInfo* tgxData, const TgxCoderInstruction* instruction);

// get a string description of the result, never returns nullptr
extern "C" __declspec(dllexport) const char* getTgxResultDescription(const TgxCoderResult result);


// analysis function that decodes the raw TGX data to a readable text stream
// only intended for analysis
TgxCoderResult decodeTgxToText(const TgxCoderTgxInfo& tgxData, std::ostream& outStream);
