#define _CRT_SECURE_NO_WARNINGS // for tests

#include <memory>
#include <string>
#include <vector>

#include "Logger.h"

#include "SHCResourceConverter.h"
#include "TGXCoder.h"

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

// rebuild with Cpp helpers
int main(int argc, char* argv[])
{
  std::vector<std::string> arguments{ argv, argv + argc };
  Log(LogLevel::TRACE, "{}", arguments.at(0));

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

    SimpleFileWrapper outRawFile{ fopen(target, "wb") };
    if (!outRawFile.get())
    {
      free(data);
      return 1;
    }

    TgxCoderTgxInfo tgxInfo{ resource->imageData, resource->dataSize, resource->header->width, resource->header->height };
    TgxCoderRawInfo rawInfo{ nullptr, resource->header->width * 2, resource->header->height * 2, resource->header->width, resource->header->height };
    std::unique_ptr<uint16_t[]> rawPixels{ std::make_unique<uint16_t[]>(static_cast<size_t>(rawInfo.rawWidth * rawInfo.rawHeight)) };
    rawInfo.data = rawPixels.get();

    decodeTgxToRaw(&tgxInfo, &rawInfo, &TGX_FILE_DEFAULT_INSTRUCTION, nullptr);
    fwrite(rawInfo.data, sizeof(uint16_t), static_cast<size_t>(rawInfo.rawWidth * rawInfo.rawHeight), outRawFile.get());

    // TODO: test why size is bigger, test in general if valid result
    tgxInfo.data = nullptr;
    tgxInfo.dataSize = 0;
    encodeRawToTgx(&rawInfo, &tgxInfo, &TGX_FILE_DEFAULT_INSTRUCTION);
    std::unique_ptr<uint8_t[]> tgxRaw{ std::make_unique<uint8_t[]>(tgxInfo.dataSize) };
    tgxInfo.data = tgxRaw.get();
    encodeRawToTgx(&rawInfo, &tgxInfo, &TGX_FILE_DEFAULT_INSTRUCTION);

    TgxAnalysis tgxAnalyses{};
    if (analyzeTgxToRaw(&tgxInfo, &TGX_FILE_DEFAULT_INSTRUCTION, &tgxAnalyses) == TgxCoderResult::SUCCESS)
    {
      SimpleFileWrapper outTgxFile{ fopen(std::string(filename).append(".test.tgx").c_str(), "wb") };
      if (!outTgxFile.get())
      {
        free(data);
        return 1;
      }
      fwrite(&resource->header->width, sizeof(uint32_t), 1, outTgxFile.get());
      fwrite(&resource->header->height, sizeof(uint32_t), 1, outTgxFile.get());
      fwrite(tgxRaw.get(), sizeof(uint8_t), static_cast<size_t>(tgxInfo.dataSize), outTgxFile.get());
    }

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
