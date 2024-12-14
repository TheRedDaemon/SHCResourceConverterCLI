
#include "SHCResourceConverter.h"
#include "TGXCoder.h"

#include <ostream>
#include <memory>

// TODO: maybe clean logical values? Many could be unsigned

// TODO: apperantly, GM files indicate with a flag if alpha is 0 or 1: https://github.com/PodeCaradox/Gm1KonverterCrossPlatform/blob/5b1ade8c38a3ed5a583dcb7ff3d843a12d14b87f/Gm1KonverterCrossPlatform/HelperClasses/Utility.cs#L170
// this also needs support, also the code might imply that certain format versions short circuit the newline

static constexpr int MAX_PIXEL_PER_MARKER{ 32 };


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
        rawData->data[targetIndex] = *(uint16_t*) (tgxData->data + sourceIndex);
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
    for (int xIndex{ 0 }; xIndex < tgxData->tgxWidth;)
    {
      for (uint8_t count{ 0 };;) // consume all transparency
      {
        const bool endLoop{ !(xIndex < tgxData->tgxWidth && rawData->data[sourceIndex] == instruction->transparentPixelRawColor) };
        if (endLoop || count >= MAX_PIXEL_PER_MARKER)
        {
          if (count > 0)
          {
            ++resultSize;
            if (tgxData->data)
            {
              if (resultSize > tgxData->dataSize)
              {
                return TgxCoderResult::INVALID_TGX_DATA_SIZE;
              }
              tgxData->data[targetIndex++] = TgxStreamMarker::TGX_MARKER_TRANSPARENT_PIXELS | (count - 1);
            };
            count = 0;
          }
          if (endLoop)
          {
            break;
          }
        }
        ++count;
        ++xIndex;
        ++sourceIndex;
      }

      // TODO?: is there a special handling for the magenta transparent-marker color pixel, since the RGB transform ignores it, but only for stream pixels?
      uint16_t pixelBuffer[MAX_PIXEL_PER_MARKER]{ 0 };
      int count{ 0 };
      int32_t repeatingPixel{ -1 };
      while (xIndex < tgxData->tgxWidth && count < MAX_PIXEL_PER_MARKER)
      {
        uint16_t nextPixel{ rawData->data[sourceIndex] };
        if (nextPixel == instruction->transparentPixelRawColor)
        {
          break;
        }

        // check if repeating pixel reach threshold, but consider pixels of next lines for this decision
        int repeatingPixelCount{ 0 };
        int tempXIndex{ xIndex };
        int tempYIndex{ yIndex };
        int tempSourceIndex{ sourceIndex };
        while (true)
        {
          if (repeatingPixelCount >= instruction->pixelRepeatThreshold)
          {
            break;
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
          ++repeatingPixelCount;
          ++tempSourceIndex;
          ++tempXIndex;
        }
        if (repeatingPixelCount >= instruction->pixelRepeatThreshold)
        {
          repeatingPixel = nextPixel;
          break;
        }

        // fix if number of pixels extend over line
        const int remainingPixelCount{ tgxData->tgxWidth - xIndex };
        if (remainingPixelCount < repeatingPixelCount)
        {
          repeatingPixelCount = remainingPixelCount;
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

      if (repeatingPixel < 0)
      {
        continue;
      }
      const uint16_t foundRepeatingPixel{ static_cast<uint16_t>(repeatingPixel) };
      for (int repeatingPixelCount{0};;) // there should be at least one, due to the check above
      {
        const bool endLoop{ !(xIndex < tgxData->tgxWidth && rawData->data[sourceIndex] == foundRepeatingPixel) };
        if (endLoop || repeatingPixelCount >= MAX_PIXEL_PER_MARKER)
        {
          // end of line short-circuits to repeating pixel under threshold if reached during repeating pixel
          // TODO: it might still be the case that the condition is always "next repeating pixels reach threshold",
          // which would require another handling here that also steps over into the next line
          if (repeatingPixelCount >= instruction->pixelRepeatThreshold || xIndex >= tgxData->tgxWidth)
          {
            resultSize += 3;
            if (tgxData->data)
            {
              if (resultSize > tgxData->dataSize)
              {
                return TgxCoderResult::INVALID_TGX_DATA_SIZE;
              }
              tgxData->data[targetIndex++] = TgxStreamMarker::TGX_MARKER_REPEATING_PIXELS | (repeatingPixelCount - 1);
              *((uint16_t*) (tgxData->data + targetIndex)) = foundRepeatingPixel;
              targetIndex += 2;
            }
            repeatingPixelCount = 0;
          }
          if (endLoop)
          {
            // adjust not taken pixels
            sourceIndex -= repeatingPixelCount;
            xIndex -= repeatingPixelCount;
            // end loop
            break;
          }
        }
        ++repeatingPixelCount;
        ++xIndex;
        ++sourceIndex;
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

TgxCoderResult decodeTgxToText(const TgxCoderTgxInfo& tgxData, const TgxCoderInstruction& instruction, std::ostream& outStream)
{
  const TgxCoderResult result{ analyzeTgxToRaw(&tgxData, &instruction, nullptr) };
  if (result != TgxCoderResult::SUCCESS)
  {
    return result;
  }

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
      for (int i{ 0 }; i < pixelNumber * 2; i += 2)
      {
        std::print(outStream, " {:#06x}", *(uint16_t*) (tgxData.data + sourceIndex));
        sourceIndex += 2;
      }
      std::print(outStream, "\n");
      break;
    case TgxStreamMarker::TGX_MARKER_REPEATING_PIXELS:
      std::print(outStream, "REPEAT_PIXEL {} {:#06x}\n", pixelNumber, *(uint16_t*) (tgxData.data + sourceIndex));
      sourceIndex += 2;
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
