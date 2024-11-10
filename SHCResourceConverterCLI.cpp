#define _CRT_SECURE_NO_WARNINGS // for tests

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "SHCResourceConverter.h"

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

int getFileSize(FILE* file)
{
  int currentPosition{ ftell(file) };
  fseek(file, 0L, SEEK_END);
  int size{ ftell(file) };
  fseek(file, currentPosition, SEEK_SET);
  return size;
}

int main(int argc, char* argv[])
{
  if (argc != 2)
  {
    return 1;
  }
  char* filename{ argv[1] };
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
    resource->header = reinterpret_cast<TgxHeader*>(data + sizeof(TgxResource));
    resource->imageData = reinterpret_cast<uint8_t*>(resource->header) + sizeof(TgxHeader);

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
