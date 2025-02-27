#include "TGXCoder.h"

#include "SHCResourceConverter.h"

#include <ostream>
#include <memory>

// TODO: maybe clean logical values? Many could be unsigned

// TODO: apperantly, GM files indicate with a flag if alpha is 0 or 1: https://github.com/PodeCaradox/Gm1KonverterCrossPlatform/blob/5b1ade8c38a3ed5a583dcb7ff3d843a12d14b87f/Gm1KonverterCrossPlatform/HelperClasses/Utility.cs#L170
// this also needs support, also the code might imply that certain format versions short circuit the newline

// TODO: indexed color currently equals the animation gm1s, and they have another difference outside only using one byte:
// newlines finish early, which might effect the encoder the most, but the padding still is required

// TODO: the handling of tile object encoding needs an extra step: the image needs to be copied to its own buffer, and the image needs an extra step to cut
// out the edges of the tile

static constexpr int MAX_PIXEL_PER_MARKER{ 32 };

static constexpr uint16_t FILLED_INDEXED_COLOR_ALPHA{ 0xff00 };


// "instruction" currently unused
TgxCoderResult analyzeTgxToRaw(const TgxCoderTgxInfo* tgxData, TgxAnalysis* tgxAnalysis)
{
  if (!tgxData)
  {
    return TgxCoderResult::MISSING_REQUIRED_STRUCTS;
  }
  const int pixelSize{ tgxData->colorType == TgxColorType::INDEXED ? 1 : 2 };

  if (tgxAnalysis) memset(tgxAnalysis, 0, sizeof(TgxAnalysis));

  int currentWidth{ 0 };
  int currentHeight{ 0 };

  uint32_t sourceIndex{ 0 };
  while (sourceIndex < tgxData->dataSize)
  {
    const TgxStreamMarker marker{ static_cast<TgxStreamMarker>(tgxData->data[sourceIndex] & TgxStreamMarker::TGX_PIXEL_MARKER) };
    const int pixelNumber{ (tgxData->data[sourceIndex] & TgxStreamMarker::TGX_PIXEL_NUMBER) + 1 }; // 0 means one pixel, like an index
    ++sourceIndex;

    if (marker == TgxStreamMarker::TGX_MARKER_NEWLINE)
    {
      if (currentWidth <= 0 && currentHeight == tgxData->tgxHeight) // handle padding at end
      {
        if (tgxAnalysis) ++tgxAnalysis->paddingNewlineMarkerCount;
        continue;
      }
      if (tgxAnalysis) ++tgxAnalysis->markerCountNewline;

      // TODO: are there even TGX without finished widths?
      if (tgxAnalysis && currentWidth < tgxData->tgxWidth) tgxAnalysis->unfinishedWidthPixelCount += tgxData->tgxWidth - currentWidth;

      currentWidth = 0;
      currentHeight += 1;
      if (currentHeight > tgxData->tgxHeight)
      {
        return TgxCoderResult::HEIGHT_TOO_BIG;
      }

      continue;
    }

    if (currentWidth == tgxData->tgxWidth)
    {
      if (tgxAnalysis) ++tgxAnalysis->newlineWithoutMarkerCount;
      // no newline marker?
      currentWidth = 0;
      currentHeight += 1;
      if (currentHeight > tgxData->tgxHeight)
      {
        return TgxCoderResult::HEIGHT_TOO_BIG;
      }
    }

    switch (marker)
    {
    case TgxStreamMarker::TGX_MARKER_STREAM_OF_PIXELS:
      if (tgxAnalysis)
      {
        ++tgxAnalysis->markerCountPixelStream;
        tgxAnalysis->pixelStreamPixelCount += pixelNumber;
      }
      sourceIndex += pixelNumber * pixelSize;
      break;
    case TgxStreamMarker::TGX_MARKER_REPEATING_PIXELS: // there might be a special case for magenta pixels
      if (tgxAnalysis)
      {
        ++tgxAnalysis->markerCountRepeatingPixels;
        tgxAnalysis->repeatingPixelsPixelCount += pixelNumber;
      }
      sourceIndex += pixelSize;
      break;
    case TgxStreamMarker::TGX_MARKER_TRANSPARENT_PIXELS:
      if (tgxAnalysis)
      {
        ++tgxAnalysis->markerCountTransparent;
        tgxAnalysis->transparentPixelCount += pixelNumber;
      }
      break;
    default:
      return TgxCoderResult::UNKNOWN_MARKER;
    }

    currentWidth += pixelNumber;
    if (currentWidth > tgxData->tgxWidth)
    {
      return TgxCoderResult::WIDTH_TOO_BIG;
    }
  }

  if (sourceIndex != tgxData->dataSize)
  {
    return TgxCoderResult::INVALID_TGX_DATA_SIZE;
  }

  if (currentHeight < tgxData->tgxHeight)
  {
    return TgxCoderResult::TGX_HAS_NOT_ENOUGH_PIXELS;
  }

  return TgxCoderResult::SUCCESS;
}

// target needs to be able to fit the result, no safety for this case
TgxCoderResult decodeTgxToRaw(const TgxCoderTgxInfo* tgxData, TgxCoderRawInfo* rawData, TgxAnalysis* tgxAnalysis)
{
  if (!(tgxData && rawData))
  {
    return TgxCoderResult::MISSING_REQUIRED_STRUCTS;
  }

  // pre-scan, since this code is meant to be careful, not fast
  const TgxCoderResult result{ analyzeTgxToRaw(tgxData, tgxAnalysis) };
  if (result != TgxCoderResult::SUCCESS)
  {
    return result;
  }
  const bool indexedColor{ tgxData->colorType == TgxColorType::INDEXED };
  // removed safety here, since scan happens before, target needs to be big enough, though

  const int lineJump{ rawData->rawWidth - tgxData->tgxWidth };
  if (lineJump < 0)
  {
    return TgxCoderResult::RAW_WIDTH_TOO_SMALL;
  }

  int currentWidth{ 0 };
  int currentHeight{ 0 }; // only required to properly handle padding
  int targetIndex{ rawData->rawX + rawData->rawWidth * rawData->rawY };
  for (uint32_t sourceIndex{ 0 }; sourceIndex < tgxData->dataSize;)
  {
    const TgxStreamMarker marker{ static_cast<TgxStreamMarker>(tgxData->data[sourceIndex] & TgxStreamMarker::TGX_PIXEL_MARKER) };
    const int pixelNumber{ (tgxData->data[sourceIndex] & TgxStreamMarker::TGX_PIXEL_NUMBER) + 1 }; // 0 means one pixel, like an index
    sourceIndex += 1;

    if (marker == TgxStreamMarker::TGX_MARKER_NEWLINE)
    {
      if (currentWidth <= 0 && currentHeight == tgxData->tgxHeight)
      {
        continue;
      }

      // could be removed if guarantee could be made that this structure does not exist
      if (currentWidth < tgxData->tgxWidth)
      {
        targetIndex += tgxData->tgxWidth - currentWidth;
      }

      currentWidth = 0; 
      currentHeight += 1;
      targetIndex += lineJump;
      continue;
    }

    // could be removed if guarantee is made that all line end with newline markers 
    if (currentWidth == tgxData->tgxWidth)
    {
      currentWidth = 0;
      currentHeight += 1;
      targetIndex += lineJump;
    }

    switch (marker)
    {
    case TgxStreamMarker::TGX_MARKER_STREAM_OF_PIXELS:
      if (indexedColor)
      {
        for (const int indexEnd{ targetIndex + pixelNumber }; targetIndex < indexEnd;)
        {
          rawData->data[targetIndex++] = FILLED_INDEXED_COLOR_ALPHA + tgxData->data[sourceIndex++];
        }
      }
      else
      {
        memcpy(rawData->data + targetIndex, tgxData->data + sourceIndex, pixelNumber * 2);
        sourceIndex += pixelNumber * 2;
        targetIndex += pixelNumber;
      }
      break;
    case TgxStreamMarker::TGX_MARKER_REPEATING_PIXELS:
      if (indexedColor)
      {
        for (const int indexEnd{ targetIndex + pixelNumber }; targetIndex < indexEnd;)
        {
          rawData->data[targetIndex++] = FILLED_INDEXED_COLOR_ALPHA + tgxData->data[sourceIndex];
        }
        ++sourceIndex;
      }
      else
      {
        for (const int indexEnd{ targetIndex + pixelNumber }; targetIndex < indexEnd; ++targetIndex)
        {
          rawData->data[targetIndex] = *(uint16_t*) (tgxData->data + sourceIndex);
        }
        sourceIndex += 2;
      }
      break;
    case TgxStreamMarker::TGX_MARKER_TRANSPARENT_PIXELS:
      targetIndex += pixelNumber;
      break;
    }
    currentWidth += pixelNumber;
  }

  return result;
}

TgxCoderResult encodeRawToTgx(const TgxCoderRawInfo* rawData, TgxCoderTgxInfo* tgxData, const TgxCoderInstruction* instruction)
{
  if (!(rawData && tgxData && instruction))
  {
    return TgxCoderResult::MISSING_REQUIRED_STRUCTS;
  }

  const int lineJump{ rawData->rawWidth - tgxData->tgxWidth };
  if (lineJump < 0)
  {
    return TgxCoderResult::RAW_WIDTH_TOO_SMALL;
  }
  // indexed can work with the alpha form like normal, it just needs to cut out the higher order byte
  const bool indexedColor{ tgxData->colorType == TgxColorType::INDEXED };

  // TODO: test and clean up

  uint32_t resultSize{ 0 };
  int sourceIndex{ rawData->rawX + rawData->rawWidth * rawData->rawY };
  int targetIndex{ 0 };
  for (int yIndex{ 0 }; yIndex < tgxData->tgxHeight; ++yIndex)
  {
    for (int xIndex{ 0 }; xIndex < tgxData->tgxWidth;)
    {
      int transparentPixelCount{ 0 };
      while (xIndex < tgxData->tgxWidth && rawData->data[sourceIndex] == instruction->transparentPixelRawColor) // consume all transparency
      {
        ++transparentPixelCount;
        ++xIndex;
        ++sourceIndex;
      }

      if (!indexedColor || xIndex < tgxData->tgxWidth) // if indexed and end of the line, short circuit to newline
      {
        while (transparentPixelCount > 0)
        {
          const int pixelThisBatch{ transparentPixelCount > MAX_PIXEL_PER_MARKER ? MAX_PIXEL_PER_MARKER : transparentPixelCount };
          transparentPixelCount -= pixelThisBatch;

          ++resultSize;
          if (tgxData->data)
          {
            if (resultSize > tgxData->dataSize)
            {
              return TgxCoderResult::INVALID_TGX_DATA_SIZE;
            }
            tgxData->data[targetIndex++] = TgxStreamMarker::TGX_MARKER_TRANSPARENT_PIXELS | (pixelThisBatch - 1);
          };
        }
      }

      // TODO?: is there a special handling for the magenta transparent-marker color pixel, since the RGB transform ignores it, but only for stream pixels?
      uint16_t pixelBuffer[MAX_PIXEL_PER_MARKER]{ 0 };
      int count{ 0 };
      int repeatingPixelCount{ 0 };
      uint16_t repeatingPixel{ 0 };
      while (xIndex < tgxData->tgxWidth && count < MAX_PIXEL_PER_MARKER)
      {
        uint16_t nextPixel{ rawData->data[sourceIndex] };
        if (nextPixel == instruction->transparentPixelRawColor)
        {
          break;
        }

        // count all repeating pixels that can be considered this line, but check pixels of next lines for this decision
        // TODO?: Is there a better approach to this? This loop always starts for every single pixel, even if it is not needed
        // TODO: threshold > 32 would cause issues now
        int tempXIndex{ xIndex };
        int tempYIndex{ yIndex };
        int tempSourceIndex{ sourceIndex };
        int tempRepeatingPixelCount{ 0 };
        while (true)
        {
          if (tempRepeatingPixelCount >= MAX_PIXEL_PER_MARKER)
          {
            repeatingPixelCount += MAX_PIXEL_PER_MARKER;
            tempRepeatingPixelCount = 0;
          }
          if (tempYIndex != yIndex && tempRepeatingPixelCount >= instruction->pixelRepeatThreshold)
          {
            break; // if we reach next line and the threshold is reached, we can stop, since the next line starts new
          }
          if (tempXIndex >= tgxData->tgxWidth)
          {
            ++tempYIndex;
            if (tempYIndex >= tgxData->tgxHeight)
            {
              break;
            }
            tempXIndex = 0;
            tempSourceIndex += lineJump;
          }
          if (rawData->data[tempSourceIndex] != nextPixel)
          {
            break;
          }
          ++tempRepeatingPixelCount;
          ++tempSourceIndex;
          ++tempXIndex;
        }
        // if more then one batch, only add remaining pixel count if threshold is reached by them
        if (repeatingPixelCount == 0 || tempRepeatingPixelCount >= instruction->pixelRepeatThreshold)
        {
          repeatingPixelCount += tempRepeatingPixelCount;
        }
        const bool reachedThreshold{ repeatingPixelCount >= instruction->pixelRepeatThreshold };

        // always fix number of pixels extend over line, since the number is used to know how many repeated pixels to write
        const int remainingPixelCount{ tgxData->tgxWidth - xIndex };
        if (remainingPixelCount < repeatingPixelCount)
        {
          repeatingPixelCount = remainingPixelCount;
        }

        if (reachedThreshold)
        {
          repeatingPixel = nextPixel;
          break;
        }

        // fix if repeating pixel not long enough for stream
        int adjustPixel{ count + repeatingPixelCount };
        if (adjustPixel > MAX_PIXEL_PER_MARKER)
        {
          adjustPixel = MAX_PIXEL_PER_MARKER;
        }

        while (count < adjustPixel)
        {
          ++sourceIndex;
          ++xIndex;
          pixelBuffer[count++] = nextPixel;
        }
        repeatingPixelCount = 0;
      }

      if (count > 0)
      {
        const int pixelSize{ indexedColor ? count : count * 2 };
        resultSize += 1 + pixelSize;
        if (tgxData->data)
        {
          if (resultSize > tgxData->dataSize)
          {
            return TgxCoderResult::INVALID_TGX_DATA_SIZE;
          }
          tgxData->data[targetIndex++] = TgxStreamMarker::TGX_MARKER_STREAM_OF_PIXELS | (count - 1);
          if (indexedColor)
          {
            for (int i{ 0 }; i < count; ++i)
            {
              tgxData->data[targetIndex++] = static_cast<uint8_t>(~FILLED_INDEXED_COLOR_ALPHA & pixelBuffer[i]);
            }
          }
          else
          {
            memcpy(tgxData->data + targetIndex, pixelBuffer, pixelSize);
            targetIndex += pixelSize;
          }
        }
      }

      while (repeatingPixelCount > 0)
      {
        const int pixelThisBatch{ repeatingPixelCount > MAX_PIXEL_PER_MARKER ? MAX_PIXEL_PER_MARKER : repeatingPixelCount };
        repeatingPixelCount -= pixelThisBatch;

        // adjust indexes
        xIndex += pixelThisBatch;
        sourceIndex += pixelThisBatch;

        // add to data
        resultSize += indexedColor ? 2 : 3;
        if (tgxData->data)
        {
          if (resultSize > tgxData->dataSize)
          {
            return TgxCoderResult::INVALID_TGX_DATA_SIZE;
          }
          tgxData->data[targetIndex++] = TgxStreamMarker::TGX_MARKER_REPEATING_PIXELS | (pixelThisBatch - 1);
          if (indexedColor)
          {
            tgxData->data[targetIndex++] = static_cast<uint8_t>(~FILLED_INDEXED_COLOR_ALPHA & repeatingPixel);
          }
          else
          {
            *((uint16_t*) (tgxData->data + targetIndex)) = repeatingPixel;
            targetIndex += 2;
          }
        }
      }
    }
    // line end
    ++resultSize;
    if (tgxData->data) {
      if (resultSize > tgxData->dataSize)
      {
        return TgxCoderResult::INVALID_TGX_DATA_SIZE;
      }
      tgxData->data[targetIndex++] = TgxStreamMarker::TGX_MARKER_NEWLINE;
    };
    sourceIndex += lineJump;
  }

  const uint32_t reminder{ resultSize % instruction->paddingAlignment };
  if (reminder > 0)
  {
    const uint32_t requiredPadding{ instruction->paddingAlignment - reminder };
    resultSize += requiredPadding;
    if (tgxData->data)
    {
      if (resultSize > tgxData->dataSize)
      {
        return TgxCoderResult::INVALID_TGX_DATA_SIZE;
      }
      for (uint32_t i{ 0 }; i < requiredPadding; ++i)
      {
        tgxData->data[targetIndex++] = TgxStreamMarker::TGX_MARKER_NEWLINE;
      }
    }
  }

  tgxData->dataSize = resultSize;
  return tgxData->data ? TgxCoderResult::SUCCESS : TgxCoderResult::FILLED_ENCODING_SIZE;
}

const char* getTgxResultDescription(const TgxCoderResult result)
{
  switch (result)
  {
  case TgxCoderResult::SUCCESS:
    return "Coder completed successfully.";
  case TgxCoderResult::FILLED_ENCODING_SIZE:
    return "Dry run of encoder completed successfully. Encoding data size was filled into given struct.";

  case TgxCoderResult::WIDTH_TOO_BIG:
    return "Decoder encountered line with bigger width than said by meta data.";
  case TgxCoderResult::HEIGHT_TOO_BIG:
    return "Decoder encountered bigger height than said by meta data.";
  case TgxCoderResult::UNKNOWN_MARKER:
    return "Decoder encountered an unknown marker in the encoded data.";
  case TgxCoderResult::INVALID_TGX_DATA_SIZE:
    return "Decoder attempted to run beyond the given encoded data. Data likely invalid or incomplete.";
  case TgxCoderResult::TGX_HAS_NOT_ENOUGH_PIXELS:
    return "Decoder produced an image with less pixels than required by the dimensions requested by the meta data.";
  case TgxCoderResult::RAW_WIDTH_TOO_SMALL:
    return "Coder was given a raw image width that is not compatible with the other meta data.";
  case TgxCoderResult::MISSING_REQUIRED_STRUCTS:
    return "Coder was not given the structs required for de- or encoding.";

  default:
    return "Encountered unknown decoder analysis result. This should not happen.";
  }
}

TgxCoderResult decodeTgxToText(const TgxCoderTgxInfo& tgxData, std::ostream& outStream)
{
  const TgxCoderResult result{ analyzeTgxToRaw(&tgxData, nullptr) };
  if (result != TgxCoderResult::SUCCESS)
  {
    return result;
  }
  const bool indexedColor{ tgxData.colorType == TgxColorType::INDEXED };

  uint32_t sourceIndex{ 0 };
  while (sourceIndex < tgxData.dataSize)
  {
    const TgxStreamMarker marker{ static_cast<TgxStreamMarker>(tgxData.data[sourceIndex] & TgxStreamMarker::TGX_PIXEL_MARKER) };
    const int pixelNumber{ (tgxData.data[sourceIndex] & TgxStreamMarker::TGX_PIXEL_NUMBER) + 1 }; // 0 means one pixel, like an index
    ++sourceIndex;

    switch (marker)
    {
    case TgxStreamMarker::TGX_MARKER_STREAM_OF_PIXELS:
      std::print(outStream, "STREAM_PIXEL {}", pixelNumber);
      if (indexedColor)
      {
        for (int i{ 0 }; i < pixelNumber; ++i)
        {
          std::print(outStream, " {:#04x}", *(uint8_t*) (tgxData.data + sourceIndex));
          ++sourceIndex;
        }
      }
      else
      {
        for (int i{ 0 }; i < pixelNumber * 2; i += 2)
        {
          std::print(outStream, " {:#06x}", *(uint16_t*) (tgxData.data + sourceIndex));
          sourceIndex += 2;
        }
      }
      std::print(outStream, "\n");
      break;
    case TgxStreamMarker::TGX_MARKER_REPEATING_PIXELS:
      if (indexedColor)
      {
        std::print(outStream, "REPEAT_PIXEL {} {:#04x}\n", pixelNumber, *(uint8_t*) (tgxData.data + sourceIndex));
        ++sourceIndex;
      }
      else
      {
        std::print(outStream, "REPEAT_PIXEL {} {:#06x}\n", pixelNumber, *(uint16_t*) (tgxData.data + sourceIndex));
        sourceIndex += 2;
      }
      break;
    case TgxStreamMarker::TGX_MARKER_TRANSPARENT_PIXELS:
      std::print(outStream, "TRANSPARENT_PIXEL {}\n", pixelNumber);
      break;
    case TgxStreamMarker::TGX_MARKER_NEWLINE:
      std::print(outStream, "NEWLINE {}\n", pixelNumber);
      break;
    default:
      return TgxCoderResult::UNKNOWN_MARKER;
    }
  }
  return TgxCoderResult::SUCCESS;
}
