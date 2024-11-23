#pragma once

#include <stdint.h>

enum class TgxCoderResult
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

struct TgxCoderRawInfo
{
  uint16_t* data; // it is assumed the buffer is big enough to contain the data indicated by this and the tgy struct
  int rawWidth;
  int rawHeight; // unused in coder, but might be handy
  int rawX;
  int rawY;
};

struct TgxCoderTgxInfo
{
  uint8_t* data;
  int dataSize;
  int tgxWidth;
  int tgxHeight;
};

struct TgxCoderInstruction
{
  uint16_t transparentPixelColorIndicator; // the game uses a certain color to also indicate transparency 
  uint16_t transparentPixel; // used to decode pixels that are indicated by the transparency header
  uint16_t transparentPixelFlag;
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
};

// defined for transformation
constexpr uint16_t GAME_TRANSPARENT_COLOR{ 0b1111100000011111 }; // used by game for some cases (repeating pixels seem excluded?)
constexpr uint16_t TGX_FILE_TRANSPARENT_PIXEL{ 0 }; // for placing them
constexpr uint16_t TGX_FILE_TRANSPARENT_PIXEL_FLAG{ 0x8000 }; // for checking if pixel should be transparent in tgx
constexpr int TGX_FILE_PIXEL_REPEAT_THRESHOLD{ 3 }; // requires testing with other files
constexpr int TGX_FILE_PADDING_ALIGNMENT{ 4 }; // requires testing with other files
constexpr TgxCoderInstruction TGX_FILE_DEFAULT_INSTRUCTION{ GAME_TRANSPARENT_COLOR, TGX_FILE_TRANSPARENT_PIXEL, TGX_FILE_TRANSPARENT_PIXEL_FLAG, TGX_FILE_PIXEL_REPEAT_THRESHOLD, TGX_FILE_PADDING_ALIGNMENT };

extern "C" __declspec(dllexport) TgxCoderResult analyzeTgxToRaw(const TgxCoderTgxInfo* tgxData, const TgxCoderInstruction* instruction, TgxAnalysis* tgxAnalysis);
extern "C" __declspec(dllexport) TgxCoderResult decodeTgxToRaw(const TgxCoderTgxInfo* tgxData, TgxCoderRawInfo* rawData, const TgxCoderInstruction* instruction, TgxAnalysis* tgxAnalysis);

// fills the dataSize in TgxCoderTgxInfo if data ptr is nullptr and return different result, else it will fill the buffer, but stop if dataSize indicates that the buffer is too small
// resulting in a broken result; if the buffer size indicated by dataSize is big enough, the dataSize will be set to the actual size
extern "C" __declspec(dllexport) TgxCoderResult encodeRawToTgx(const TgxCoderRawInfo* rawData, TgxCoderTgxInfo* tgxData, const TgxCoderInstruction* instruction);
