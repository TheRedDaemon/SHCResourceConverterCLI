
#include <memory>
#include <string>
#include <vector>
#include <fstream>

#include "Logger.h"
#include "Utility.h"

#include "SHCResourceConverter.h"
#include "TGXCoder.h"
#include "BinaryCFileReadHelper.h"


/*
  GENERAL INFO:
  Certain parts use a C-like structure, since the it is also partially intended to experiment for
  potential UCP3 modules. Since the provided APIs by modules are C-like and it is not clear which parts
  will make it into a modules, the use of C-like structures is arbitrary.

  Note for modules:
  - C-like APIs should never throw
  - Memory control should be handled by one module, since passing around structures is error-prone
  - never mix malloc/free with new/delete and the later never with their array versions
*/

// TODO: C like APIs should never throw

// rebuild with Cpp helpers
int main(int argc, char* argv[])
{
  std::vector<std::string> arguments{ argv, argv + argc };
  Log(LogLevel::TRACE, "EXE: {}", arguments.at(0));

  if (argc != 3)
  {
    return 1;
  }
  char* filename{ argv[1] };
  char* target{ argv[2] };

  // test
  auto fileReader{ createWithAdditionalMemory<BinaryCFileReadHelper>(100, filename) };
  if (fileReader->hasInvalidState())
  {
    return 1;
  }
  const int size{ fileReader->getSize() };
  if (size < 0)
  {
    return 1;
  }

  char* extension{ strrchr(filename, '.') };
  if (!extension)
  {
    return 1;
  }

  if (!strcmp(extension, ".tgx"))
  {
    auto resource{ createWithAdditionalMemory<TgxResource>(size) };
    fileReader->read(reinterpret_cast<uint8_t*>(resource.get()) + sizeof(TgxResource), sizeof(uint8_t), size);
    if (fileReader->hasInvalidState())
    {
      return 1;
    }
    resource->base.type = SHCResourceType::SHC_RESOURCE_TGX;
    resource->base.resourceSize = size;
    resource->base.colorFormat = PixeColorFormat::ARGB_1555;
    resource->dataSize = size - sizeof(TgxHeader);
    resource->header = reinterpret_cast<TgxHeader*>(reinterpret_cast<uint8_t*>(resource.get()) + sizeof(TgxResource));
    resource->imageData = reinterpret_cast<uint8_t*>(resource->header) + sizeof(TgxHeader);

    std::ofstream outRawFile{ target, std::ios::binary };
    if (!outRawFile.is_open())
    {
      return 1;
    }

    TgxCoderTgxInfo tgxInfo{ resource->imageData, resource->dataSize, resource->header->width, resource->header->height };
    TgxCoderRawInfo rawInfo{ nullptr, resource->header->width * 2, resource->header->height * 2, resource->header->width, resource->header->height };
    std::unique_ptr<uint16_t[]> rawPixels{ std::make_unique<uint16_t[]>(static_cast<size_t>(rawInfo.rawWidth * rawInfo.rawHeight)) };
    rawInfo.data = rawPixels.get();

    decodeTgxToRaw(&tgxInfo, &rawInfo, &TGX_FILE_DEFAULT_INSTRUCTION, nullptr);
    outRawFile.write((char*) rawInfo.data, rawInfo.rawWidth * rawInfo.rawHeight * sizeof(uint16_t));

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
      std::ofstream outTgxFile{ std::string(filename).append(".test.tgx").c_str(), std::ios::binary };
      if (!outTgxFile.is_open())
      {
        return 1;
      }
      outTgxFile.write(reinterpret_cast<char*>(&resource->header->width), sizeof(uint32_t));
      outTgxFile.write(reinterpret_cast<char*>(&resource->header->height), sizeof(uint32_t));
      outTgxFile.write(reinterpret_cast<char*>(tgxInfo.data), tgxInfo.dataSize);
    }
  }
  else if (!strcmp(extension, ".gm1"))
  {
    auto resource{ createWithAdditionalMemory<Gm1Resource>(size) };
    fileReader->read(reinterpret_cast<uint8_t*>(resource.get()) + sizeof(Gm1Resource), sizeof(uint8_t), size);
    if (fileReader->hasInvalidState())
    {
      return 1;
    }
    resource->base.type = SHCResourceType::SHC_RESOURCE_GM1;
    resource->base.resourceSize = size;
    resource->base.colorFormat = PixeColorFormat::ARGB_1555;
    resource->gm1Header = reinterpret_cast<Gm1Header*>(reinterpret_cast<uint8_t*>(resource.get()) + sizeof(Gm1Resource));
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
  }
  else
  {
    return 1;
  }

  return 0;
}
