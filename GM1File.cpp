#include "GM1File.h"

#include "Console.h"
#include "ResourceMetaFormat.h"

#include <fstream>

// TODO: the tile coder might actually need to be precise and not write transparency, assuming the images are
// placed on the canvas. Should it turn out that this is the case, either the coder needs to be different, or
// all decoding actually needs to work this way, by initializing the canvas with the correct transparency (without the decoder)

namespace GM1File
{
  void validateGm1Resource(const Gm1Resource& resource)
  {
    throw std::exception{ "Not yet implemented." };
  }

  UniqueGm1ResourcePointer loadGm1Resource(const std::filesystem::path& file)
  {
    Log(LogLevel::INFO, "Try loading GM1 file.");
    if (!std::filesystem::is_regular_file(file))
    {
      Log(LogLevel::ERROR, "Provided GM1 file is not a regular file.");
      return {};
    }
    const uintmax_t fileSize{ std::filesystem::file_size(file) };
    if (fileSize < MIN_FILE_SIZE)
    {
      Log(LogLevel::ERROR, "Provided file is too small for a GM1 file.");
      return {};
    }
    if (fileSize > MAX_FILE_SIZE)
    {
      Log(LogLevel::ERROR, "Provided GM1 file is too big to be handled by this implementation.");
      return {};
    }
    const uint32_t size{ static_cast<uint32_t>(fileSize) };

    std::ifstream in;
    in.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    in.open(file, std::ios::in | std::ios::binary);

    auto resource{ createWithAdditionalMemory<Gm1Resource>(size) };
    resource->base.type = SHCResourceType::SHC_RESOURCE_GM1;
    resource->base.resourceSize = size;
    resource->base.colorFormat = PixeColorFormat::ARGB_1555;

    Log(LogLevel::DEBUG, "Loading GM1 header.");
    in.read(reinterpret_cast<char*>(resource.get()) + sizeof(Gm1Resource), sizeof(Gm1Header));
    resource->gm1Header = reinterpret_cast<Gm1Header*>(reinterpret_cast<uint8_t*>(resource.get()) + sizeof(Gm1Resource));

    const uint32_t numberOfImages{ resource->gm1Header->numberOfPicturesInFile };
    const uint32_t gm1BodySize{ size - sizeof(Gm1Header) };

    // check if info in header matches data size in body (full size = header + (imageOffset + imageSize + imageHeader) * imageNumber + dataSize)
    if (resource->gm1Header->dataSize != gm1BodySize - (2 * sizeof(uint32_t) + sizeof(Gm1ImageHeader)) * numberOfImages)
    {
      Log(LogLevel::ERROR, "Provided GM1 body does not have the size as specified in the header.");
      return {};
    }
    // simple type check in load to at least understand how the file should be handled
    if (resource->gm1Header->gm1Type < Gm1Type::GM1_TYPE_INTERFACE || resource->gm1Header->gm1Type > Gm1Type::GM1_TYPE_NO_COMPRESSION_2)
    {
      Log(LogLevel::ERROR, "Provided GM1 header does not specify known GM1 type.");
      return {};
    }

    Log(LogLevel::DEBUG, "Loading GM1 body.");
    in.read(reinterpret_cast<char*>(resource.get()) + sizeof(Gm1Resource) + sizeof(Gm1Header), gm1BodySize);
    resource->imageOffsets = reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(resource->gm1Header) + sizeof(Gm1Header));
    resource->imageSizes = reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(resource->imageOffsets) + sizeof(uint32_t) * numberOfImages);
    resource->imageHeaders = reinterpret_cast<Gm1ImageHeader*>(reinterpret_cast<uint8_t*>(resource->imageSizes) + sizeof(uint32_t) * numberOfImages);
    resource->imageData = reinterpret_cast<uint8_t*>(resource->imageHeaders) + sizeof(Gm1ImageHeader) * numberOfImages;

    // individual images are not checked without explicit validation

    Log(LogLevel::INFO, "Loaded GM1 resource.");
    return resource;
  }

  void saveGm1Resource(const std::filesystem::path& file, const Gm1Resource& resource)
  {
    Log(LogLevel::INFO, "Try saving GM1 resource as GM1 file.");

    std::filesystem::create_directories(file.parent_path());
    Log(LogLevel::DEBUG, "Created directories.");

    // inner block, to wrap file action
    {
      std::ofstream out;
      out.exceptions(std::ofstream::failbit | std::ofstream::badbit);
      out.open(file, std::ios::out | std::ios::trunc | std::ios::binary);

      const uint32_t numberOfImages{ resource.gm1Header->numberOfPicturesInFile };
      out.write(reinterpret_cast<char*>(resource.gm1Header), sizeof(Gm1Header));
      out.write(reinterpret_cast<char*>(resource.imageOffsets), sizeof(uint32_t) * numberOfImages);
      out.write(reinterpret_cast<char*>(resource.imageSizes), sizeof(uint32_t) * numberOfImages);
      out.write(reinterpret_cast<char*>(resource.imageHeaders), sizeof(Gm1ImageHeader) * numberOfImages);
      out.write(reinterpret_cast<char*>(resource.imageData), resource.gm1Header->dataSize);
    }

    // small validation
    if (std::filesystem::file_size(file) != resource.base.resourceSize)
    {
      Log(LogLevel::ERROR, "Saved GM1 resource as GM1 file, but saved file has not expected size. Might be corrupted.");
      return;
    }
    else
    {
      Log(LogLevel::INFO, "Saved GM1 resource as GM1 file.");
    }
  }

  UniqueGm1ResourcePointer loadGm1ResourceFromRaw(const std::filesystem::path& folder)
  {
    throw std::exception{ "Not yet implemented." };
  }

  void saveGm1ResourceAsRaw(const std::filesystem::path& folder, const Gm1Resource& resource)
  {
    throw std::exception{ "Not yet implemented." };
  }
}
