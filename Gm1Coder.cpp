#include "Gm1Coder.h"


static constexpr int HALF_TILE_WIDTH{ TILE_WIDTH / 2 };
static constexpr int QUARTER_TILE_WIDTH{ HALF_TILE_WIDTH / 2 };
static constexpr int HALF_TILE_HEIGHT{ TILE_HEIGHT / 2 };

static bool isRectContainedInCanvas(int x, int y,
  unsigned int innerWidth, unsigned int innerHeight,
  unsigned int canvasWidth, unsigned int canvasHeight)
{
  return x >= 0 && y >= 0 && x + innerWidth <= canvasWidth && y + innerHeight <= canvasHeight;
}

extern "C" __declspec(dllexport) Gm1CoderResult decodeTileToRaw(const uint16_t* tile, Gm1CoderRawInfo* raw, uint16_t transparentColor)
{
  if (!(tile && raw))
  {
    return Gm1CoderResult::MISSING_REQUIRED_PARAMETER;
  }
  if (!isRectContainedInCanvas(raw->rawX, raw->rawY, TILE_WIDTH, TILE_HEIGHT, raw->rawWidth, raw->rawHeight))
  {
    return Gm1CoderResult::CANVAS_CAN_NOT_CONTAIN_IMAGE;
  }
  if (!raw->raw)
  {
    return Gm1CoderResult::CHECKED_PARAMETER;
  }

  const int lineJump{ raw->rawWidth - TILE_WIDTH };
  size_t sourceIndex{ 0 };
  size_t targetIndex{ raw->rawX + static_cast<uint64_t>(raw->rawWidth) * raw->rawY };
  for (int y{ -HALF_TILE_HEIGHT }; y <= HALF_TILE_HEIGHT; ++y)
  {
    for (int x{ -QUARTER_TILE_WIDTH }; x <= QUARTER_TILE_WIDTH; ++x)
    {
      if (y == 0)
      {
        continue;
      }
      const int xAbs{ x < 0 ? -x : x };
      const int yAbs{ y < 0 ? -y : y };
      if (xAbs + yAbs <= HALF_TILE_WIDTH)
      {
        raw->raw[targetIndex++] = tile[sourceIndex++];
        raw->raw[targetIndex++] = tile[sourceIndex++];
      }
      else
      {
        raw->raw[targetIndex++] = transparentColor;
        raw->raw[targetIndex++] = transparentColor;
      }
    }

    targetIndex += lineJump;
  }
  return Gm1CoderResult::SUCCESS;
}

extern "C" __declspec(dllexport) Gm1CoderResult encodeRawToTile(const Gm1CoderRawInfo* raw, uint16_t* tile, uint16_t transparentColor)
{
  if (!(raw && tile))
  {
    return Gm1CoderResult::MISSING_REQUIRED_PARAMETER;
  }
  if (!isRectContainedInCanvas(raw->rawX, raw->rawY, TILE_WIDTH, TILE_HEIGHT, raw->rawWidth, raw->rawHeight))
  {
    return Gm1CoderResult::CANVAS_CAN_NOT_CONTAIN_IMAGE;
  }

  const int lineJump{ raw->rawWidth - TILE_WIDTH };
  size_t sourceIndex{ raw->rawX + static_cast<uint64_t>(raw->rawWidth) * raw->rawY };
  size_t targetIndex{ 0 };
  for (int y{ -HALF_TILE_HEIGHT }; y <= HALF_TILE_HEIGHT; ++y)
  {
    for (int x{ -QUARTER_TILE_WIDTH }; x <= QUARTER_TILE_WIDTH; ++x)
    {
      if (y == 0)
      {
        continue;
      }
      const int xAbs{ x < 0 ? -x : x };
      const int yAbs{ y < 0 ? -y : y };
      if (xAbs + yAbs <= HALF_TILE_WIDTH)
      {
        if (tile)
        {
          tile[targetIndex++] = raw->raw[sourceIndex++];
          tile[targetIndex++] = raw->raw[sourceIndex++];
        }
        else
        {
          sourceIndex += 2;
          targetIndex += 2;
        }
      }
      else
      {
        const bool firstTransparent{ raw->raw[sourceIndex++] == transparentColor };
        const bool secondTransparent{ raw->raw[sourceIndex++] == transparentColor };
        if (!firstTransparent || !secondTransparent)
        {
          return Gm1CoderResult::EXPECTED_TRANSPARENT_PIXEL;
        }
      }
    }

    sourceIndex += lineJump;
  }
  return tile ? Gm1CoderResult::SUCCESS : Gm1CoderResult::CHECKED_PARAMETER;
}

extern "C" __declspec(dllexport) Gm1CoderResult copyUncompressedToRaw(const Gm1CoderDataInfo* uncompressed, Gm1CoderRawInfo* raw)
{
  if (!(uncompressed && raw))
  {
    return Gm1CoderResult::MISSING_REQUIRED_PARAMETER;
  }
  if (uncompressed->dataSize != uncompressed->dataWidth * uncompressed->dataHeight)
  {
    return Gm1CoderResult::INVALID_DATA_SIZE;
  }
  if (!isRectContainedInCanvas(raw->rawX, raw->rawY, TILE_WIDTH, TILE_HEIGHT, raw->rawWidth, raw->rawHeight))
  {
    return Gm1CoderResult::CANVAS_CAN_NOT_CONTAIN_IMAGE;
  }
  if (!raw->raw)
  {
    return Gm1CoderResult::CHECKED_PARAMETER;
  }

  size_t sourceIndex{ 0 };
  size_t targetIndex{ raw->rawX + static_cast<uint64_t>(raw->rawWidth) * raw->rawY };
  for (int y{ 0 }; y < uncompressed->dataHeight; ++y)
  {
    std::memcpy(raw->raw + targetIndex, uncompressed->data + sourceIndex, uncompressed->dataWidth * sizeof(uint16_t));
    sourceIndex += uncompressed->dataWidth * 2;
    targetIndex += raw->rawWidth;
  }
  return Gm1CoderResult::SUCCESS;
}

extern "C" __declspec(dllexport) Gm1CoderResult copyRawToUncompressed(const Gm1CoderRawInfo* raw, Gm1CoderDataInfo* uncompressed)
{
  if (!(raw && uncompressed))
  {
    return Gm1CoderResult::MISSING_REQUIRED_PARAMETER;
  }
  if (uncompressed->dataSize != uncompressed->dataWidth * uncompressed->dataHeight)
  {
    return Gm1CoderResult::INVALID_DATA_SIZE;
  }
  if (!isRectContainedInCanvas(raw->rawX, raw->rawY, TILE_WIDTH, TILE_HEIGHT, raw->rawWidth, raw->rawHeight))
  {
    return Gm1CoderResult::CANVAS_CAN_NOT_CONTAIN_IMAGE;
  }
  if (!uncompressed->data)
  {
    return Gm1CoderResult::CHECKED_PARAMETER;
  }

  size_t sourceIndex{ raw->rawX + static_cast<uint64_t>(raw->rawWidth) * raw->rawY };
  size_t targetIndex{ 0 };
  for (int y{ 0 }; y < uncompressed->dataHeight; ++y)
  {
    std::memcpy(uncompressed->data + targetIndex, raw->raw + sourceIndex, uncompressed->dataWidth * sizeof(uint16_t));
    sourceIndex += raw->rawWidth;
    targetIndex += uncompressed->dataWidth * 2;
  }
  return Gm1CoderResult::SUCCESS;
}

// TODO implement TGX coder for 8bit variant -> how to do? mix with file?
