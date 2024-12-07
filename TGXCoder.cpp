
#include <memory>

#include "SHCResourceConverter.h"
#include "TGXCoder.h"

// TODO: maybe clean logical values? Many could be unsigned

// TODO: apperantly, GM files indicate with a flag if alpha is 0 or 1: https://github.com/PodeCaradox/Gm1KonverterCrossPlatform/blob/5b1ade8c38a3ed5a583dcb7ff3d843a12d14b87f/Gm1KonverterCrossPlatform/HelperClasses/Utility.cs#L170
// this also needs support, also the code might imply that certain format versions short circuit the newline

// "instruction" currently unused
TgxCoderResult analyzeTgxToRaw(const TgxCoderTgxInfo* tgxData, const TgxCoderInstruction* instruction, TgxAnalysis* tgxAnalysis)
{
  if (!(tgxData && instruction))
  {
    return TgxCoderResult::MISSING_REQUIRED_STRUCTS;
  }

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
      if (tgxAnalysis) ++tgxAnalysis->markerCountNewline;
      if (currentWidth <= 0 && currentHeight == tgxData->tgxHeight) // handle padding at end
      {
        if (tgxAnalysis) ++tgxAnalysis->paddingNewlineMarkerCount;
        continue;
      }

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
      sourceIndex += pixelNumber * 2;
      break;
    case TgxStreamMarker::TGX_MARKER_REPEATING_PIXELS: // there might be a special case for magenta pixels
      if (tgxAnalysis)
      {
        ++tgxAnalysis->markerCountRepeatingPixels;
        tgxAnalysis->repeatingPixelsPixelCount += pixelNumber;
      }
      sourceIndex += 2;
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
TgxCoderResult decodeTgxToRaw(const TgxCoderTgxInfo* tgxData, TgxCoderRawInfo* rawData, const TgxCoderInstruction* instruction, TgxAnalysis* tgxAnalysis)
{
  if (!(tgxData && rawData && instruction))
  {
    return TgxCoderResult::MISSING_REQUIRED_STRUCTS;
  }

  // pre-scan, since this code is meant to be careful, not fast
  const TgxCoderResult result{ analyzeTgxToRaw(tgxData, instruction, tgxAnalysis) };
  if (result != TgxCoderResult::SUCCESS)
  {
    return result;
  }
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
        for (const int indexEnd{ targetIndex + tgxData->tgxWidth - currentWidth }; targetIndex < indexEnd; ++targetIndex)
        {
          rawData->data[targetIndex] = instruction->transparentPixelRawColor;
        }
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
      memcpy(rawData->data + targetIndex, tgxData->data + sourceIndex, pixelNumber * 2);
      sourceIndex += pixelNumber * 2;
      targetIndex += pixelNumber;
      break;
    case TgxStreamMarker::TGX_MARKER_REPEATING_PIXELS:
      for (const int indexEnd{ targetIndex + pixelNumber }; targetIndex < indexEnd; ++targetIndex)
      {
        rawData->data[targetIndex] = *(int16_t*) (tgxData->data + sourceIndex);
      }
      sourceIndex += 2;
      break;
    case TgxStreamMarker::TGX_MARKER_TRANSPARENT_PIXELS:
      for (const int indexEnd{ targetIndex + pixelNumber }; targetIndex < indexEnd; ++targetIndex)
      {
        rawData->data[targetIndex] = instruction->transparentPixelRawColor;
      }
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

  // TODO: test and clean up

  uint32_t resultSize{ 0 };
  int sourceIndex{ rawData->rawX + rawData->rawWidth * rawData->rawY };
  int targetIndex{ 0 };
  for (int yIndex{ 0 }; yIndex < tgxData->tgxHeight; ++yIndex)
  {
    int xIndex{ 0 };
    while (xIndex < tgxData->tgxWidth)
    {
      uint8_t count{ 0 };

      while (xIndex < tgxData->tgxWidth && count < 32 && rawData->data[sourceIndex] == instruction->transparentPixelRawColor)
      {
        ++count;
        ++xIndex;
        ++sourceIndex;
      }

      if (count > 0) // we added transparency, so continue with next loop
      {
        ++resultSize;
        if (tgxData->data) { 
          if (resultSize > tgxData->dataSize)
          {
            return TgxCoderResult::INVALID_TGX_DATA_SIZE;
          }
          tgxData->data[targetIndex++] = TgxStreamMarker::TGX_MARKER_TRANSPARENT_PIXELS | (count - 1);
        };
        continue;
      }

      // TODO?: is there a special handling for the magenta transparent-marker color pixel, since the RGB transform ignores it, but only for stream pixels?
      uint16_t pixelBuffer[32];
      int repeatingPixelCount{ 0 };
      uint16_t repeatingPixel{ 0 };
      while (xIndex < tgxData->tgxWidth && count < 32)
      {
        uint16_t nextPixel{ rawData->data[sourceIndex] };
        if (nextPixel == instruction->transparentPixelRawColor)
        {
          break;
        }

        while (xIndex < tgxData->tgxWidth && repeatingPixelCount < 32 && rawData->data[sourceIndex] == nextPixel)
        {
          ++repeatingPixelCount;
          ++sourceIndex;
          ++xIndex;
        }
        if (repeatingPixelCount >= instruction->pixelRepeatThreshold)
        {
          repeatingPixel = nextPixel;
          break;
        }

        // fix if repeating pixel not long enough for stream
        int adjustPixel{ count + repeatingPixelCount };
        if (adjustPixel > 32)
        {
          const int reduceSteps{ adjustPixel - 32 };
          sourceIndex -= reduceSteps;
          xIndex -= reduceSteps;
          adjustPixel = 32;
        }

        while (count < adjustPixel)
        {
          pixelBuffer[count++] = nextPixel;
        }
        repeatingPixelCount = 0;
      }

      if (count > 0)
      {
        const int pixelSize{ count * 2 };
        resultSize += 1 + pixelSize;
        if (tgxData->data)
        {
          if (resultSize > tgxData->dataSize)
          {
            return TgxCoderResult::INVALID_TGX_DATA_SIZE;
          }
          tgxData->data[targetIndex++] = TgxStreamMarker::TGX_MARKER_STREAM_OF_PIXELS | (count - 1);
          memcpy(tgxData->data + targetIndex, pixelBuffer, pixelSize);
          targetIndex += pixelSize;
        }
      }

      if (repeatingPixelCount > 0)
      {
        resultSize += 3;
        if (tgxData->data)
        {
          if (resultSize > tgxData->dataSize)
          {
            return TgxCoderResult::INVALID_TGX_DATA_SIZE;
          }
          tgxData->data[targetIndex++] = TgxStreamMarker::TGX_MARKER_REPEATING_PIXELS | (repeatingPixelCount - 1);
          *((uint16_t*) (tgxData->data + targetIndex)) = repeatingPixel;
          targetIndex += 2;
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

  const uint32_t requiredPadding{ resultSize % instruction->paddingAlignment };
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
