#pragma once

#include <stdint.h>

/* GAME STRUCTURES AND ENUMS */

enum PixeColorFormat : int32_t
{
  ARGB_1555 = 0x555,
  RGB_565 = 0x565,
};

enum TgxStreamMarker : uint8_t
{
  TGX_MARKER_STREAM_OF_PIXELS = 0x0,
  TGX_MARKER_TRANSPARENT_PIXELS = 0x20,
  TGX_MARKER_REPEATING_PIXELS = 0x40,
  TGX_MARKER_NEWLINE = 0x80,

  TGX_PIXEL_MARKER = 0xe0,
  TGX_PIXEL_NUMBER = 0x1f,
};

struct alignas(int32_t) TgxHeader
{
  int32_t width;
  int32_t height;
};

// source: https://github.com/PodeCaradox/Gm1KonverterCrossPlatform/blob/master/Gm1KonverterCrossPlatform/Files/GM1FileHeader.cs
enum class Gm1Type : int32_t
{
  GM1_TYPE_INTERFACE = 1,  // Interface items and some building animations. Images are stored similar to TGX images.
  GM1_TYPE_ANIMATIONS = 2,  // Animations
  GM1_TYPE_TILES_OBJECT = 3,  // Buildings. Images are stored similar to TGX images but with a Tile object.
  GM1_TYPE_FONT = 4,  // Font. TGX format.
  GM1_TYPE_NO_COMPRESSION_1 = 5,
  GM1_TYPE_TGX_CONST_SIZE = 6,
  GM1_TYPE_NO_COMPRESSION_2 = 7
};

// source: https://github.com/PodeCaradox/Gm1KonverterCrossPlatform/blob/master/Gm1KonverterCrossPlatform/Files/GM1FileHeader.cs
struct alignas(int32_t) Gm1Header
{
  uint32_t unknown_0x0;
  uint32_t unknown_0x4;
  uint32_t unknown_0x8;
  uint32_t numberOfPicturesInFile;
  uint32_t unknown_0x10;
  Gm1Type gm1Type;
  uint32_t unknown_0x18;
  uint32_t unknown_0x1C;
  uint32_t unknown_0x20;
  uint32_t unknown_0x24;
  uint32_t unknown_0x28;
  uint32_t unknown_0x2C;
  uint32_t width;
  uint32_t height;
  uint32_t unknown_0x38;
  uint32_t unknown_0x3C;
  uint32_t unknown_0x40;
  uint32_t unknown_0x44;
  uint32_t originX;
  uint32_t originY;
  uint32_t dataSize;
  uint32_t unknown_0x54;
  uint16_t colorPalette[10][256];
};

struct alignas(int32_t) Gm1TileObjectImageInfo
{
  uint8_t imagePart;
  uint8_t subParts;
  uint16_t tileOffset;
  uint8_t direction;
  uint8_t horizontalOffsetOfImage;
  uint8_t buildingWidth;
  uint8_t animatedColor; // if alpha 1?
};

struct alignas(int32_t) Gm1GeneralImageInfo
{
  int16_t relativeDataPos;  // seems to be used to point to data to use instead
  int16_t fontRelatedSize;
  uint8_t unknown_0x4;
  uint8_t unknown_0x5;
  uint8_t unknown_0x6;
  uint8_t flags; // used to indicate together with game flag, if certain animation frames are skipped
};

struct alignas(int32_t) Gm1UnknownImageInfo
{
  uint8_t unknown_0x0;
  uint8_t unknown_0x1;
  uint8_t unknown_0x2;
  uint8_t unknown_0x3;
  uint8_t unknown_0x4;
  uint8_t unknown_0x5;
  uint8_t unknown_0x6;
  uint8_t unknown_0x7;
};

union alignas(int32_t) Gm1ImageInfo
{
  Gm1GeneralImageInfo generalImageInfo;
  Gm1TileObjectImageInfo tileObjectImageInfo;
  Gm1UnknownImageInfo unknownImageInfo;
};

struct alignas(int32_t) Gm1ImageHeader
{
  uint16_t width;
  uint16_t height;
  uint16_t offsetX;
  uint16_t offsetY;
};

// source: https://github.com/PodeCaradox/Gm1KonverterCrossPlatform/blob/master/Gm1KonverterCrossPlatform/Files/TGXImageHeader.cs
struct alignas(int32_t) Gm1Image
{
  Gm1ImageHeader imageHeader;
  Gm1ImageInfo imageInfo;
};

/* API STRUCTURES AND ENUMS */

enum SHCResourceType : int
{
  SHC_RESOURCE_TGX = 0x0,
  SHC_RESOURCE_GM1 = 0x1
};

struct GenericResource
{
  SHCResourceType type;
  uint32_t resourceSize;
  PixeColorFormat colorFormat;
};

struct TgxResource
{
  GenericResource base;
  uint32_t dataSize;
  TgxHeader* header;
  uint8_t* imageData;
};

struct Gm1Resource
{
  GenericResource base;
  Gm1Header* gm1Header;
  uint32_t* imageOffsets;
  uint32_t* imageSizes;
  Gm1Image* imageHeaders;
  uint8_t* imageData;
};
