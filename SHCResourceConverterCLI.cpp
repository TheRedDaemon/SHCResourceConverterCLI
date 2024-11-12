#define _CRT_SECURE_NO_WARNINGS // for tests

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <memory>

#include "SHCResourceConverter.h"

// defined for transformation
constexpr uint16_t TRANSPARENT_PIXEL{ 0 };
constexpr uint16_t TRANSPARENT_COLOR{ 0b1111100000011111 }; // used by game for some cases (repeating pixels seem excluded?)

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

// currently without check, maybe better use some CPP structures for buffer safety?
int transformTgxToRaw(const uint8_t* source, const int sourceSize, uint16_t* target, const int width, const int height/*, const int targetX, const int targetY, const int targetWidth*/)
{
  const int rawPixelSize{ width * height };
  
  int totalPixels{ 0 };
  int currentWidth{ 0 };
  int currentHeight{ 0 };

  int targetIndex{ 0 };
  for (int sourceIndex{ 0 }; sourceIndex < sourceSize;)
  {
    const TgxStreamMarker marker{ static_cast<TgxStreamMarker>(source[sourceIndex] & TgxStreamMarker::TGX_PIXEL_MARKER) };
    const int pixelNumber{ (source[sourceIndex] & TgxStreamMarker::TGX_PIXEL_NUMBER) + 1 }; // 0 means one pixel, like an index
    sourceIndex += 1;

    if (marker == TgxStreamMarker::TGX_MARKER_NEWLINE)
    {
      // maybe 3 "newlines" is a end marker? -> is not, at least not with everything, maybe it is padding?
      if (currentWidth <= 0)
      {
        continue;
      }

      // need to fill with transparent pixels
      // TODO: are there even TGX without finished widths?
      if (currentWidth < width)
      {
        const int missingPixels{ width - currentWidth };
        for (int i{ 0 }; i < missingPixels; i++)
        {
          target[targetIndex] = TRANSPARENT_PIXEL;
          targetIndex += 1;
        }
        totalPixels += missingPixels;
      }

      currentWidth = 0;
      currentHeight += 1;
      continue;
    }
    else if (currentWidth == width)
    {
      // no newline marker?
      currentWidth = 0;
      currentHeight += 1;
    }
    else if (currentWidth > width)
    {
      return -1;
    }

    switch (marker)
    {
    case TgxStreamMarker::TGX_MARKER_STREAM_OF_PIXELS:
      memcpy(target + targetIndex, source + sourceIndex, pixelNumber * 2);
      sourceIndex += pixelNumber * 2;
      targetIndex += pixelNumber;
      totalPixels += pixelNumber;
      currentWidth += pixelNumber;
      break;
    case TgxStreamMarker::TGX_MARKER_REPEATING_PIXELS:
      for (int i{ 0 }; i < pixelNumber; i++)
      {
        target[targetIndex] = ((int16_t*)source)[sourceIndex];
        targetIndex += 1;
      }
      sourceIndex += 2;
      totalPixels += pixelNumber;
      currentWidth += pixelNumber;
      break;
    case TgxStreamMarker::TGX_MARKER_TRANSPARENT_PIXELS:
      for (int i{ 0 }; i < pixelNumber; i++)
      {
        target[targetIndex] = TRANSPARENT_PIXEL;
        targetIndex += 1;
      }
      totalPixels += pixelNumber;
      currentWidth += pixelNumber;
      break;
    default:
      return -1;
    }
  }

  if (rawPixelSize != totalPixels)
  {
    return -1;
  }

  return totalPixels;
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

    std::unique_ptr<uint16_t[]> rawPixels{ std::make_unique<uint16_t[]>(resource->header->width * resource->header->height) };

    transformTgxToRaw(resource->imageData, resource->dataSize, rawPixels.get(), resource->header->width, resource->header->height);
    fwrite(rawPixels.get(), sizeof(uint16_t), resource->header->width * resource->header->height, file.get());

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
