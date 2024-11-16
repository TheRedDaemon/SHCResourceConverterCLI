#define _CRT_SECURE_NO_WARNINGS // for tests

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <memory>

#include "SHCResourceConverter.h"

// defined for transformation
constexpr uint16_t TRANSPARENT_PIXEL{ 0 }; // for placing them
constexpr uint16_t TRANSPARENT_COLOR{ 0b1111100000011111 }; // used by game for some cases (repeating pixels seem excluded?)
constexpr uint16_t TRANSPARENT_PIXEL_FLAG{ 0x8000 }; // for checking if pixel should be transparent in tgx

class SimpleFileWrapper
{
private:
  FILE* file;
public:
  SimpleFileWrapper(FILE* file) : file{ file } {};
  ~SimpleFileWrapper()
  {
    if (file)
    {
      fclose(file);
    }
  }
  SimpleFileWrapper(const SimpleFileWrapper&) = delete;
  SimpleFileWrapper& operator=(const SimpleFileWrapper&) = delete;

  FILE* get() { return file; }
};

enum class TgxToRawResult
{
  SUCCESS,
  UNKNOWN_MARKER,
  WIDTH_TOO_BIG,
  HEIGHT_TOO_BIG,
  INVALID_SOURCE_DATA_SIZE,
  SOURCE_HAS_NOT_ENOUGH_PIXELS,
  TARGET_WIDTH_TOO_SMALL,
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

TgxToRawResult analyzeTgxToRaw(const uint8_t* source, const int sourceSize, const int expectedWidth, const int expectedHeight, TgxAnalysis* tgxAnalysis)
{
  TgxAnalysis internalTgxAnalysis{};

  int currentWidth{ 0 };
  int currentHeight{ 0 };

  int sourceIndex{ 0 };
  while (sourceIndex < sourceSize)
  {
    const TgxStreamMarker marker{ static_cast<TgxStreamMarker>(source[sourceIndex] & TgxStreamMarker::TGX_PIXEL_MARKER) };
    const int pixelNumber{ (source[sourceIndex] & TgxStreamMarker::TGX_PIXEL_NUMBER) + 1 }; // 0 means one pixel, like an index
    ++sourceIndex;

    if (marker == TgxStreamMarker::TGX_MARKER_NEWLINE)
    {
      ++internalTgxAnalysis.markerCountNewline;
      if (currentWidth <= 0) // there can be multiple newlines, maybe it is padding?
      {
        continue;
      }

      // TODO: are there even TGX without finished widths?
      if (currentWidth < expectedWidth)
      {
        internalTgxAnalysis.unfinishedWidthPixelCount += expectedWidth - currentWidth;
      }

      currentWidth = 0;
      currentHeight += 1;
      if (currentHeight > expectedHeight)
      {
        return TgxToRawResult::HEIGHT_TOO_BIG;
      }

      continue;
    }
    
    if (currentWidth == expectedWidth)
    {
      ++internalTgxAnalysis.newlineWithoutMarkerCount;
      // no newline marker?
      currentWidth = 0;
      currentHeight += 1;
      if (currentHeight > expectedHeight)
      {
        return TgxToRawResult::HEIGHT_TOO_BIG;
      }
    }

    switch (marker)
    {
    case TgxStreamMarker::TGX_MARKER_STREAM_OF_PIXELS:
      ++internalTgxAnalysis.markerCountPixelStream;
      internalTgxAnalysis.pixelStreamPixelCount += pixelNumber;
      sourceIndex += pixelNumber * 2;
      break;
    case TgxStreamMarker::TGX_MARKER_REPEATING_PIXELS: // there might be a special case for magenta pixels
      ++internalTgxAnalysis.markerCountRepeatingPixels;
      internalTgxAnalysis.repeatingPixelsPixelCount += pixelNumber;
      sourceIndex += 2;
      break;
    case TgxStreamMarker::TGX_MARKER_TRANSPARENT_PIXELS:
      ++internalTgxAnalysis.markerCountTransparent;
      internalTgxAnalysis.transparentPixelCount += pixelNumber;
      break;
    default:
      return TgxToRawResult::UNKNOWN_MARKER;
    }

    currentWidth += pixelNumber;
    if (currentWidth > expectedWidth)
    {
      return TgxToRawResult::WIDTH_TOO_BIG;
    }
  }

  if (sourceIndex != sourceSize)
  {
    return TgxToRawResult::INVALID_SOURCE_DATA_SIZE;
  }

  if (currentHeight < expectedHeight)
  {
    return TgxToRawResult::SOURCE_HAS_NOT_ENOUGH_PIXELS;
  }

  if (tgxAnalysis)
  {
    *tgxAnalysis = internalTgxAnalysis;
  }

  return TgxToRawResult::SUCCESS;
}

// target needs to be able to fit the result, no safety for this case
TgxToRawResult decodeTgxToRaw(const uint8_t* source, const int sourceSize, const int width, const int height, uint16_t* target, const int targetX, const int targetY, const int targetWidth)
{
  // pre-scan, since this code is meant to be careful, not fast
  const TgxToRawResult result{ analyzeTgxToRaw(source, sourceSize, width, height, nullptr) };
  if (result != TgxToRawResult::SUCCESS)
  {
    return result;
  }
  // removed safety here, since scan happens before, target needs to be big enough, though

  const int lineJump{ targetWidth - width };
  if (lineJump < 0)
  {
    return TgxToRawResult::TARGET_WIDTH_TOO_SMALL;
  }
  
  int currentWidth{ 0 };
  int targetIndex{ targetX + targetWidth * targetY };
  for (int sourceIndex{ 0 }; sourceIndex < sourceSize;)
  {
    const TgxStreamMarker marker{ static_cast<TgxStreamMarker>(source[sourceIndex] & TgxStreamMarker::TGX_PIXEL_MARKER) };
    const int pixelNumber{ (source[sourceIndex] & TgxStreamMarker::TGX_PIXEL_NUMBER) + 1 }; // 0 means one pixel, like an index
    sourceIndex += 1;

    if (marker == TgxStreamMarker::TGX_MARKER_NEWLINE)
    {
      if (currentWidth <= 0)
      {
        continue;
      }

      // could be removed if guarantee could be made that this structure does not exist
      if (currentWidth < width)
      {
        for (const int indexEnd{ targetIndex + width - currentWidth }; targetIndex < indexEnd; ++targetIndex)
        {
          target[targetIndex] = TRANSPARENT_PIXEL;
        }
      }

      currentWidth = 0;
      targetIndex += lineJump;
      continue;
    }
    
    // could be removed if guarantee is made that all line end with newline markers 
    if (currentWidth == width)
    {
      currentWidth = 0;
      targetIndex += lineJump;
    }

    switch (marker)
    {
    case TgxStreamMarker::TGX_MARKER_STREAM_OF_PIXELS:
      memcpy(target + targetIndex, source + sourceIndex, pixelNumber * 2);
      sourceIndex += pixelNumber * 2;
      targetIndex += pixelNumber;
      break;
    case TgxStreamMarker::TGX_MARKER_REPEATING_PIXELS:
      for (const int indexEnd{ targetIndex + pixelNumber }; targetIndex < indexEnd; ++targetIndex)
      {
        target[targetIndex] = ((int16_t*) source)[sourceIndex];
      }
      sourceIndex += 2;
      break;
    case TgxStreamMarker::TGX_MARKER_TRANSPARENT_PIXELS:
      for (const int indexEnd{ targetIndex + pixelNumber }; targetIndex < indexEnd; ++targetIndex)
      {
        target[targetIndex] = TRANSPARENT_PIXEL;
      }
      break;
    }
    currentWidth += pixelNumber;
  }

  return result;
}

// expects target to be big enough, returns actual size, send target as nullptr to determine size without writing, returns -1 on error
int encodeRawToTgx(const uint16_t* source, const int sourceX, const int sourceY, const int sourceWidth, const int targetWidth, const int targetHeight, uint8_t* target)
{
  const int lineJump{ sourceWidth - targetWidth };
  if (lineJump < 0)
  {
    return -1;
  }

  // TODO: test and clean up

  int resultSize{ 0 };
  int sourceIndex{ sourceX + sourceWidth * sourceY };
  int targetIndex{ 0 };
  for (int currentHeight{ 0 }; currentHeight < targetHeight; ++currentHeight)
  {
    for (int currentWidth{ 0 }; currentWidth < targetWidth; ++currentWidth)
    {
      uint16_t initialPixel{ source[sourceIndex++] };

      if (!(initialPixel & TRANSPARENT_PIXEL_FLAG))
      {
        uint8_t count{ 0 }; // 0 means one pixel
        while (currentWidth < targetWidth - 1 && count < 31 && !(source[sourceIndex] & TRANSPARENT_PIXEL_FLAG))
        {
          ++count;
          ++currentWidth;
          ++sourceIndex;
        }
        ++resultSize;
        if (target) target[targetIndex++] = TgxStreamMarker::TGX_MARKER_TRANSPARENT_PIXELS | count;
      }
      else if (currentWidth < targetWidth - 1 && initialPixel == source[sourceIndex])
      {
        // TODO?: is there a special handling for the magenta transparent color pixel?
        uint8_t count{ 0 };
        while (currentWidth < targetWidth - 1 && count < 31 && initialPixel == source[sourceIndex])
        {
          ++count;
          ++currentWidth;
          ++sourceIndex;
        }
        resultSize += 3;
        if (target)
        {
          target[targetIndex++] = TgxStreamMarker::TGX_MARKER_REPEATING_PIXELS | count;
          *((uint16_t*) (target + targetIndex)) = initialPixel;
          targetIndex += 2;
        }
      }
      else {
        uint16_t pixelBuffer[32];
        pixelBuffer[0] = initialPixel;
        
        uint8_t count{ 0 };
        while (currentWidth < targetWidth - 1 && count < 31 && (source[sourceIndex] & TRANSPARENT_PIXEL_FLAG) && pixelBuffer[count] != source[sourceIndex])
        {
          pixelBuffer[++count] = source[sourceIndex++];
          ++currentWidth;
        }

        // correct repeat pixels
        if (currentWidth < targetWidth - 1 && pixelBuffer[count] == source[sourceIndex])
        {
          --count;
          --sourceIndex;
          --currentWidth;
        }


        const int pixelSize{ (count + 1) * 2 };
        resultSize += 1 + pixelSize;
        if (target)
        {
          target[targetIndex++] = TgxStreamMarker::TGX_MARKER_STREAM_OF_PIXELS | count;
          memcpy(target + targetIndex, pixelBuffer, pixelSize);
          targetIndex += pixelSize;
        }
      }
    }
    // line end
    ++resultSize;
    if (target) target[targetIndex++] = TgxStreamMarker::TGX_MARKER_NEWLINE;
    sourceIndex += lineJump;
    // TODO?: does it need padding at the end
  }

  return resultSize;
}

int getFileSize(FILE* file)
{
  int currentPosition{ ftell(file) };
  fseek(file, 0L, SEEK_END);
  int size{ ftell(file) };
  fseek(file, currentPosition, SEEK_SET);
  return size;
}

// rebuild with Cpp helpers
int main(int argc, char* argv[])
{
  if (argc != 3)
  {
    return 1;
  }
  char* filename{ argv[1] };
  char* target{ argv[2] };

  SimpleFileWrapper file{ fopen(filename, "rb") };
  if (!file.get())
  {
    return 1;
  }
  int size{ getFileSize(file.get()) };

  char* extension{ strrchr(filename, '.') };
  if (!extension)
  {
    return 1;
  }

  if (!strcmp(extension, ".tgx"))
  {
    char* data{ static_cast<char*>(malloc(sizeof(TgxResource) + size)) };
    if (!data)
    {
      return 1;
    }
    fread(data + sizeof(TgxResource), sizeof(uint8_t), size, file.get());

    // TODO: extend resource with raw file size and color encoding 
    TgxResource* resource{ reinterpret_cast<TgxResource*>(data) };
    resource->base.type = SHCResourceType::SHC_RESOURCE_TGX;
    resource->dataSize = size - sizeof(TgxHeader);
    resource->header = reinterpret_cast<TgxHeader*>(data + sizeof(TgxResource));
    resource->imageData = reinterpret_cast<uint8_t*>(resource->header) + sizeof(TgxHeader);

    SimpleFileWrapper file{ fopen(target, "wb") };
    if (!file.get())
    {
      free(data);
      return 1;
    }

    std::unique_ptr<uint16_t[]> rawPixels{ std::make_unique<uint16_t[]>(resource->header->width * 2 * resource->header->height * 2) };

    decodeTgxToRaw(resource->imageData, resource->dataSize, resource->header->width, resource->header->height, rawPixels.get(), resource->header->width, resource->header->height, resource->header->width * 2);
    fwrite(rawPixels.get(), sizeof(uint16_t), resource->header->width * 2 * resource->header->height * 2, file.get());


    // TODO: test why size is bigger, test in general if valid result
    int size{ encodeRawToTgx(rawPixels.get(), resource->header->width, resource->header->height, resource->header->width * 2, resource->header->width, resource->header->height, nullptr) };

    free(data);
  }
  else if (!strcmp(extension, ".gm1"))
  {
    char* data{ static_cast<char*>(malloc(sizeof(Gm1Resource) + size)) };
    if (!data)
    {
      return 1;
    }
    fread(data + sizeof(Gm1Resource), sizeof(uint8_t), size, file.get());

    Gm1Resource* resource{ reinterpret_cast<Gm1Resource*>(data) };
    resource->base.type = SHCResourceType::SHC_RESOURCE_GM1;
    resource->gm1Header = reinterpret_cast<Gm1Header*>(data + sizeof(Gm1Resource));
    resource->imageOffsets = reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(resource->gm1Header) + sizeof(Gm1Header));
    resource->imageSizes = reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(resource->imageOffsets) + sizeof(uint32_t) * resource->gm1Header->numberOfPicturesInFile);
    resource->imageHeaders = reinterpret_cast<Gm1ImageHeader*>(reinterpret_cast<uint8_t*>(resource->imageSizes) + sizeof(uint32_t) * resource->gm1Header->numberOfPicturesInFile);
    resource->imageData = reinterpret_cast<uint8_t*>(resource->imageHeaders) + sizeof(Gm1ImageHeader) * resource->gm1Header->numberOfPicturesInFile;

    uint32_t offsetOne{ resource->imageOffsets[1]};
    uint32_t sizeOne{ resource->imageSizes[1] };
    Gm1ImageHeader* headerOne{ resource->imageHeaders + 1 };

    uint32_t sizeOfDataBasedOnHeader{ resource->gm1Header->dataSize };
    uint32_t sizeOfDataBasedOnSize{ size - sizeof(Gm1Header) - (2 * sizeof(uint32_t) + sizeof(Gm1ImageHeader) ) * resource->gm1Header->numberOfPicturesInFile };

    // validation required? -> Maybe partial read?

    free(data);
  }
  else
  {
    return 1;
  }

  return 0;
}
