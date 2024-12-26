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
    if (y == 0)
    {
      continue;
    }
    const int yAbs{ y < 0 ? -y : y };
    for (int x{ -QUARTER_TILE_WIDTH }; x <= QUARTER_TILE_WIDTH; ++x)
    {
      const int xAbs{ x < 0 ? -x : x };
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
    if (y == 0)
    {
      continue;
    }
    const int yAbs{ y < 0 ? -y : y };
    for (int x{ -QUARTER_TILE_WIDTH }; x <= QUARTER_TILE_WIDTH; ++x)
    {
      
      const int xAbs{ x < 0 ? -x : x };
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

Gm1CoderResult copyUncompressedToRaw(const Gm1CoderDataInfo* uncompressed, Gm1CoderRawInfo* raw, uint16_t transparentColor)
{
  if (!(uncompressed && raw))
  {
    return Gm1CoderResult::MISSING_REQUIRED_PARAMETER;
  }
  if (!isRectContainedInCanvas(raw->rawX, raw->rawY, uncompressed->dataWidth, uncompressed->dataHeight, raw->rawWidth, raw->rawHeight))
  {
    return Gm1CoderResult::CANVAS_CAN_NOT_CONTAIN_IMAGE;
  }
  // the data might not fill every horizontal line
  const size_t lineSize{ uncompressed->dataWidth * sizeof(uint16_t) };
  if (uncompressed->dataSize > lineSize * uncompressed->dataHeight || uncompressed->dataSize % lineSize != 0)
  {
    return Gm1CoderResult::INVALID_DATA_SIZE;
  }
  if (!raw->raw)
  {
    return Gm1CoderResult::CHECKED_PARAMETER;
  }

  size_t sourceIndex{ 0 };
  size_t targetIndex{ raw->rawX + static_cast<uint64_t>(raw->rawWidth) * raw->rawY };
  const size_t linesWithData{ uncompressed->dataSize / lineSize };
  for (int y{ 0 }; y < linesWithData; ++y)
  {
    std::memcpy(raw->raw + targetIndex, uncompressed->data + sourceIndex, uncompressed->dataWidth * sizeof(uint16_t));
    sourceIndex += uncompressed->dataWidth * sizeof(uint16_t);
    targetIndex += raw->rawWidth;
  }
  // fill remaining lines with transparent color
  const int lineJump{ raw->rawWidth - uncompressed->dataWidth };
  for (int y{ 0 }; y < uncompressed->dataHeight - linesWithData; ++y)
  {
    for (int x{ 0 }; x < uncompressed->dataWidth; ++x)
    {
      raw->raw[targetIndex++] = transparentColor;
    }
    targetIndex += lineJump;
  }
  return Gm1CoderResult::SUCCESS;
}

Gm1CoderResult copyRawToUncompressed(const Gm1CoderRawInfo* raw, Gm1CoderDataInfo* uncompressed, uint16_t transparentColor)
{
  if (!(raw && uncompressed))
  {
    return Gm1CoderResult::MISSING_REQUIRED_PARAMETER;
  }
  if (!isRectContainedInCanvas(raw->rawX, raw->rawY, uncompressed->dataWidth, uncompressed->dataHeight, raw->rawWidth, raw->rawHeight))
  {
    return Gm1CoderResult::CANVAS_CAN_NOT_CONTAIN_IMAGE;
  }
  // the data might not fill every horizontal line
  const size_t lineSize{ uncompressed->dataWidth * sizeof(uint16_t) };

  size_t sourceIndex{ raw->rawX + static_cast<uint64_t>(raw->rawWidth) * raw->rawY };
  size_t linesWithData{ 0 };
  if (uncompressed->data)
  {
    // copy data into receiver
    if (uncompressed->dataSize > static_cast<int64_t>(lineSize) * uncompressed->dataHeight || uncompressed->dataSize % lineSize != 0)
    {
      return Gm1CoderResult::INVALID_DATA_SIZE;
    }

    size_t targetIndex{ 0 };
    linesWithData = uncompressed->dataSize / lineSize;
    for (int y{ 0 }; y < uncompressed->dataHeight - linesWithData; ++y)
    {
      std::memcpy(uncompressed->data + targetIndex, raw->raw + sourceIndex, uncompressed->dataWidth * sizeof(uint16_t));
      sourceIndex += raw->rawWidth;
      targetIndex += uncompressed->dataWidth * sizeof(uint16_t);
    }
  }
  else
  {
    // determine needed size
    for (; linesWithData < uncompressed->dataHeight; ++linesWithData)
    {
      for (int x{ 0 }; x < uncompressed->dataWidth; ++x)
      {
        if (raw->raw[sourceIndex + x] == transparentColor)
        {
          goto transparencyFound;
        }
      }
      sourceIndex += raw->rawWidth;
    }
  transparencyFound:;
  }

  // validate that only transparent pixels are left at the end
  const int lineJump{ raw->rawWidth - uncompressed->dataWidth };
  for (int y{ 0 }; y < uncompressed->dataHeight - linesWithData; ++y)
  {
    for (int x{ 0 }; x < uncompressed->dataWidth; ++x)
    {
      if (raw->raw[sourceIndex++] != transparentColor)
      {
        return Gm1CoderResult::EXPECTED_TRANSPARENT_PIXEL;
      }
    }
    sourceIndex += lineJump;
  }

  if (uncompressed->data)
  {
    return Gm1CoderResult::SUCCESS;
  }
  else
  {
    uncompressed->dataSize = static_cast<uint32_t>(linesWithData * lineSize);
    return Gm1CoderResult::FILLED_ENCODING_SIZE;
  }
}

// TODO implement TGX coder for 8bit variant -> how to do? mix with file?

const char* getGm1ResultDescription(const Gm1CoderResult result)
{
  switch (result)
  {
  case Gm1CoderResult::SUCCESS:
    return "Coder completed successfully.";
  case Gm1CoderResult::CHECKED_PARAMETER:
    return "Parameter check and/or dry run completed successfully.";
  case Gm1CoderResult::FILLED_ENCODING_SIZE:
    return "Dry run completed successfully. Additionally, the data size was filled into given out struct.";

  case Gm1CoderResult::MISSING_REQUIRED_PARAMETER:
    return "Coder was not given the parameter required for de- or encoding.";
  case Gm1CoderResult::INVALID_DATA_SIZE:
      return "The given data size does not relate to the given dimensions.";
  case Gm1CoderResult::EXPECTED_TRANSPARENT_PIXEL:
    return "The coder expected to find a transparent pixel in the source, but encountered another color.";
  case Gm1CoderResult::CANVAS_CAN_NOT_CONTAIN_IMAGE:
    return "The given data does indicate that the image can not be contained in raw 2D pixel canvas.";

  default:
    return "Encountered unknown decoder analysis result. This should not happen.";
  }
}
