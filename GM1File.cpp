#include "GM1File.h"

#include "Gm1Coder.h"

#include "Console.h"
#include "ResourceMetaFormat.h"

#include <fstream>
#include <span>

// TODO: the tile coder might actually need to be precise and not write transparency, assuming the images are
// placed on the canvas. Should it turn out that this is the case, either the coder needs to be different, or
// all decoding actually needs to work this way, by initializing the canvas with the correct transparency (without the decoder)

// TODO: the meaning of the relative data index field is still strange, for example, super chicken.gm1 would have the number 256
// in this index (far to big?), but no flag; some interfaces have -1 in there and a fitting flag for the case that in game flag
// would be true, despite them not having a fitting earlier part; this needs more checks
// although, the flag mode might be unfinished or broken: there are other conditions for this "flag" not being 0, and while
// the menu worked, going on the map crashed the map in my test, so maybe everything that has not the 4 bit flag + the relative pos
// might simply be leftovers


// TODO: the height of the tgx for tile images is always tileOffset + 7, the same as height - tile height + 7
// the data is actually displayed in the GmCrossConverter like it is in the data, with the TGX sunken into 
// the space of the tile, this would be a very important thing to know, as is would change how the decoder would work
// how can this even work then? Assuming the tiles are packed together, meta info would need to be used, no way around this
// width and offset would point to the start of the tgx, and the actual tile part would need to be ignored
// this would even more point to the idea that before decoding, the image was copied to and intermediate buffer, with this then
// being used to encode the data
// finished testing, seems to work: copy offset rect -> split into different rects and encode them separately seems to be the case
// there seems to be a weirdness in the encoded data, this also needs analysis (maybe the encoder respected the tile edge?

namespace GM1File
{
  static bool validateGm1UncompressedResource(const Gm1Resource& resource, const TgxCoderInstruction& instructions)
  {
    for (size_t i{ 0 }; i < resource.gm1Header->info.numberOfPicturesInFile; ++i)
    {
      const Gm1Image& image{ resource.imageHeaders[i] };
      const uint32_t offset{ resource.imageOffsets[i] };
      const uint32_t size{ resource.imageSizes[i] };

      Out("### Image {} ###\n{}\n\n{}\n\n", i, image.imageHeader, image.imageInfo.generalImageInfo);

      const Gm1CoderDataInfo dataInfo{
        .data{ resource.imageData + offset },
        .dataSize{ size },
        .dataWidth{ image.imageHeader.width },
        .dataHeight{ image.imageHeader.height },
      };
      Out("# General Image Info #\n{}\n\n", dataInfo);

      Gm1CoderRawInfo rawInfo{
        .raw{ nullptr },
        .rawWidth{ image.imageHeader.width },
        .rawHeight{ image.imageHeader.height },
        .rawX{ 0 },
        .rawY{ 0 },
      };
      const Gm1CoderResult result{ copyUncompressedToRaw(&dataInfo, &rawInfo, instructions.transparentPixelRawColor) };
      if (result != Gm1CoderResult::CHECKED_PARAMETER)
      {
        Out("{}\n", std::string_view{ getGm1ResultDescription(result) });
        return false;
      }
    }
    return true;
  }

  static bool validateGm1TgxResource(const Gm1Resource& resource, bool tgxAsText)
  {
    for (size_t i{ 0 }; i < resource.gm1Header->info.numberOfPicturesInFile; ++i)
    {
      const Gm1Image& image{ resource.imageHeaders[i] };
      const uint32_t offset{ resource.imageOffsets[i] };
      const uint32_t size{ resource.imageSizes[i] };

      Out("### Image {} ###\n{}\n\n{}\n\n", i, image.imageHeader, image.imageInfo.generalImageInfo);

      const TgxCoderTgxInfo tgxInfo{
        .colorType{ resource.gm1Header->info.gm1Type == Gm1Type::GM1_TYPE_ANIMATIONS ? TgxColorType::INDEXED : TgxColorType::DEFAULT },
        .data{ resource.imageData + offset },
        .dataSize{ size },
        .tgxWidth{ image.imageHeader.width },
        .tgxHeight{ image.imageHeader.height }
      };
      Out("# General TGX Info #\n{}\n\n", tgxInfo);

      // animations use the origin from the header, so to make sense, all of them need to have the same image size
      if (resource.gm1Header->info.gm1Type == Gm1Type::GM1_TYPE_ANIMATIONS
        && (tgxInfo.tgxWidth != resource.gm1Header->info.width || tgxInfo.tgxHeight != resource.gm1Header->info.height))
      {
        Out("Is animation resource, but dimensions of image do not match dimensions of header.\n");
        return false;
      }

      TgxAnalysis tgxAnalysis{};
      const TgxCoderResult result{ analyzeTgxToRaw(&tgxInfo, &tgxAnalysis) };
      if (result != TgxCoderResult::SUCCESS)
      {
        Out("{}\n", std::string_view{ getTgxResultDescription(result) });
        return false;
      }
      Out("# Structure Meta Data #\n{}\n\n", tgxAnalysis);
      if (!tgxAsText)
      {
        continue;
      }
      Log(LogLevel::INFO, "Printing TGX as text to stdout.");
      const TgxCoderResult toTextResult{ decodeTgxToText(tgxInfo, STD_OUT) };
      if (toTextResult != TgxCoderResult::SUCCESS)
      {
        Out("{}\n", std::string_view{ getTgxResultDescription(toTextResult) });
        Log(LogLevel::ERROR, "Failed to print TGX as text.");
        return false;
      }
      Log(LogLevel::INFO, "Completed to print TGX as text.");
      Out("\n");
    }
    return true;
  }

  static bool validateGm1TileObjectResource(const Gm1Resource& resource, bool tgxAsText)
  {
    for (size_t i{ 0 }; i < resource.gm1Header->info.numberOfPicturesInFile; ++i)
    {
      const Gm1Image& image{ resource.imageHeaders[i] };
      const uint32_t offset{ resource.imageOffsets[i] };
      const uint32_t size{ resource.imageSizes[i] };

      Out("### Image {} ###\n{}\n\n{}\n\n", i, image.imageHeader, image.imageInfo.tileObjectImageInfo);

      // the size contains the tile, so this should work
      Gm1CoderRawInfo rawInfo{
        .raw{ nullptr },
        .rawWidth{ TILE_WIDTH },
        .rawHeight{ TILE_HEIGHT },
        .rawX{ 0 },
        .rawY{ 0 },
      };
      const Gm1CoderResult tileResult{ decodeTileToRaw(reinterpret_cast<uint16_t*>(resource.imageData + offset), &rawInfo) };
      if (tileResult != Gm1CoderResult::CHECKED_PARAMETER)
      {
        Out("{}\n", std::string_view{ getGm1ResultDescription(tileResult) });
        return false;
      }

      if (image.imageInfo.tileObjectImageInfo.imagePosition == Gm1TileObjectImagePosition::NONE)
      {
        continue;
      }

      const TgxCoderTgxInfo tgxInfo{
        .data{ resource.imageData + offset + TILE_BYTE_SIZE },
        .dataSize{ size - TILE_BYTE_SIZE },
        .tgxWidth{ image.imageInfo.tileObjectImageInfo.imageWidth },
        .tgxHeight{ image.imageInfo.tileObjectImageInfo.tileOffset + TILE_IMAGE_HEIGHT_OFFSET }
      };
      Out("# General TGX Info #\n{}\n\n", tgxInfo);

      TgxAnalysis tgxAnalysis{};
      const TgxCoderResult tgxResult{ analyzeTgxToRaw(&tgxInfo, &tgxAnalysis) };
      if (tgxResult != TgxCoderResult::SUCCESS)
      {
        Out("{}\n", std::string_view{ getTgxResultDescription(tgxResult) });
        return false;
      }
      Out("# Structure Meta Data #\n{}\n\n", tgxAnalysis);
      if (!tgxAsText)
      {
        continue;
      }
      Log(LogLevel::INFO, "Printing TGX as text to stdout.");
      const TgxCoderResult toTextResult{ decodeTgxToText(tgxInfo, STD_OUT) };
      if (toTextResult != TgxCoderResult::SUCCESS)
      {
        Out("{}\n", std::string_view{ getTgxResultDescription(toTextResult) });
        Log(LogLevel::ERROR, "Failed to print TGX as text.");
        return false;
      }
      Log(LogLevel::INFO, "Completed to print TGX as text.");
      Out("\n");
    }
    return true;
  }

  void validateGm1Resource(const Gm1Resource& resource, const TgxCoderInstruction& instructions, bool tgxAsText)
  {
    Log(LogLevel::INFO, "Try validating given resource.");

    Out("### General GM1 info ###\nType: {}\nNumber of pictures: {}\nImage data size: {}\n\n",
      resource.gm1Header->info.gm1Type, resource.gm1Header->info.numberOfPicturesInFile, resource.gm1Header->info.dataSize);

    Out("### GM1 Header ###\n{}\n\n", *resource.gm1Header);

    bool validationSuccessful{ false };
    switch (resource.gm1Header->info.gm1Type)
    {
    case Gm1Type::GM1_TYPE_INTERFACE:
    case Gm1Type::GM1_TYPE_TGX_CONST_SIZE:
    case Gm1Type::GM1_TYPE_FONT:
    case Gm1Type::GM1_TYPE_ANIMATIONS:
      validationSuccessful = validateGm1TgxResource(resource, tgxAsText);
      break;
    case Gm1Type::GM1_TYPE_TILES_OBJECT:
      validationSuccessful = validateGm1TileObjectResource(resource, tgxAsText);
      break;
    case Gm1Type::GM1_TYPE_NO_COMPRESSION_1:
    case Gm1Type::GM1_TYPE_NO_COMPRESSION_2:
      validationSuccessful = validateGm1UncompressedResource(resource, instructions);
      break;

    default:
      Log(LogLevel::ERROR, "Resource has unknown type.");
      break;
    }

    if (validationSuccessful)
    {
      Out("### GM1 seems valid ###\n");
      Log(LogLevel::INFO, "Validation completed successfully.");
    }
    else
    {
      Out("\n### GM1 seems invalid. Remaining checks are skipped. ###\n");
      Log(LogLevel::ERROR, "Validation completed. TGX is invalid.");
    }
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

    const uint32_t numberOfImages{ resource->gm1Header->info.numberOfPicturesInFile };
    const uint32_t gm1BodySize{ size - sizeof(Gm1Header) };

    // check if info in header matches data size in body (full size = header + (imageOffset + imageSize + imageHeader) * imageNumber + dataSize)
    if (resource->gm1Header->info.dataSize != gm1BodySize - (2 * sizeof(uint32_t) + sizeof(Gm1Image)) * numberOfImages)
    {
      Log(LogLevel::ERROR, "Provided GM1 body does not have the size as specified in the header.");
      return {};
    }
    // simple type check in load to at least understand how the file should be handled
    if (resource->gm1Header->info.gm1Type < Gm1Type::GM1_TYPE_INTERFACE || resource->gm1Header->info.gm1Type > Gm1Type::GM1_TYPE_NO_COMPRESSION_2)
    {
      Log(LogLevel::ERROR, "Provided GM1 header does not specify known GM1 type.");
      return {};
    }

    Log(LogLevel::DEBUG, "Loading GM1 body.");
    in.read(reinterpret_cast<char*>(resource.get()) + sizeof(Gm1Resource) + sizeof(Gm1Header), gm1BodySize);
    resource->imageOffsets = reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(resource->gm1Header) + sizeof(Gm1Header));
    resource->imageSizes = reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(resource->imageOffsets) + sizeof(uint32_t) * numberOfImages);
    resource->imageHeaders = reinterpret_cast<Gm1Image*>(reinterpret_cast<uint8_t*>(resource->imageSizes) + sizeof(uint32_t) * numberOfImages);
    resource->imageData = reinterpret_cast<uint8_t*>(resource->imageHeaders) + sizeof(Gm1Image) * numberOfImages;

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

      const uint32_t numberOfImages{ resource.gm1Header->info.numberOfPicturesInFile };
      out.write(reinterpret_cast<char*>(resource.gm1Header), sizeof(Gm1Header));
      out.write(reinterpret_cast<char*>(resource.imageOffsets), sizeof(uint32_t) * numberOfImages);
      out.write(reinterpret_cast<char*>(resource.imageSizes), sizeof(uint32_t) * numberOfImages);
      out.write(reinterpret_cast<char*>(resource.imageHeaders), sizeof(Gm1Image) * numberOfImages);
      out.write(reinterpret_cast<char*>(resource.imageData), resource.gm1Header->info.dataSize);
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

  static bool isExpectedMetaObject(const std::string_view identifier, const std::string_view expectedIdentifier,
    int version, std::span<const int> supportedVersions)
  {
    if (identifier != expectedIdentifier)
    {
      Log(LogLevel::ERROR, "Did not receive a {} object at the expected position.", expectedIdentifier);
      return false;
    }
    const auto it{ std::find(supportedVersions.begin(), supportedVersions.end(), version) };
    if (it == supportedVersions.end())
    {
      Log(LogLevel::ERROR, "{} object has no supported version. (Provided version: {})", identifier, version);
      return false;
    }
    return true;
  }

  static bool hasExpectedEntryNumber(const ResourceMetaFormat::ResourceMetaObjectReader& metaObject, int mapEntries, int listEntries)
  {
    if (metaObject.getMapEntries().size() != mapEntries || metaObject.getListEntries().size() != listEntries)
    {
      Log(LogLevel::ERROR, "{} object has not expected number of map and list entries.", metaObject.getIdentifier());
      return false;
    }
    return true;
  }

  static const std::string* getResourceObjectMapEntry(const ResourceMetaFormat::ResourceMetaObjectReader& metaObject,
    const std::string_view identifier, const std::string_view key)
  {
    auto mapEntries{ metaObject.getMapEntries() };
    auto it{ mapEntries.find(key) };
    if (it == mapEntries.end())
    {
      Log(LogLevel::ERROR, "{} object has not expected entry '{}'.", identifier, key);
      return nullptr;
    }
    return &it->second;
  }

  static bool readGm1HeaderInfoFromResourceMetaObject(const ResourceMetaFormat::ResourceMetaObjectReader& metaObject, Gm1HeaderInfo& outHeaderInfo)
  {
    Log(LogLevel::DEBUG, "Read Gm1Header info object from meta file.");
    if (!isExpectedMetaObject(metaObject.getIdentifier(), Gm1HeaderMeta::RESOURCE_IDENTIFIER, metaObject.getVersion(), Gm1HeaderMeta::SUPPORTED_VERSIONS))
    {
      return false;
    }
    // version currently ignored, since only one available
    if (!hasExpectedEntryNumber(metaObject, Gm1HeaderMeta::MAP_ENTRIES, Gm1HeaderMeta::LIST_ENTRIES))
    {
      return false;
    }

    // read header
    const auto& headerInfoEntries{ metaObject.getListEntries() };
    outHeaderInfo.unknown_0x0 = intFromStr<uint32_t>(headerInfoEntries.at(0));
    outHeaderInfo.unknown_0x4 = intFromStr<uint32_t>(headerInfoEntries.at(1));
    outHeaderInfo.unknown_0x8 = intFromStr<uint32_t>(headerInfoEntries.at(2));
    outHeaderInfo.numberOfPicturesInFile = intFromStr<uint32_t>(headerInfoEntries.at(3));
    outHeaderInfo.unknown_0x10 = intFromStr<uint32_t>(headerInfoEntries.at(4));

    // horrible form, but data enums are fixed
    outHeaderInfo.gm1Type = static_cast<Gm1Type>(intFromStr<int32_t, 0, static_cast<int32_t>(Gm1Type::GM1_TYPE_INTERFACE), static_cast<int32_t>(Gm1Type::GM1_TYPE_NO_COMPRESSION_2)>(headerInfoEntries.at(5)));

    outHeaderInfo.unknown_0x18 = intFromStr<uint32_t>(headerInfoEntries.at(6));
    outHeaderInfo.unknown_0x1C = intFromStr<uint32_t>(headerInfoEntries.at(7));
    outHeaderInfo.unknown_0x20 = intFromStr<uint32_t>(headerInfoEntries.at(8));
    outHeaderInfo.unknown_0x24 = intFromStr<uint32_t>(headerInfoEntries.at(9));
    outHeaderInfo.unknown_0x28 = intFromStr<uint32_t>(headerInfoEntries.at(10));
    outHeaderInfo.unknown_0x2C = intFromStr<uint32_t>(headerInfoEntries.at(11));
    outHeaderInfo.width = intFromStr<uint32_t>(headerInfoEntries.at(12));
    outHeaderInfo.height = intFromStr<uint32_t>(headerInfoEntries.at(13));;
    outHeaderInfo.unknown_0x38 = intFromStr<uint32_t>(headerInfoEntries.at(14));
    outHeaderInfo.unknown_0x3C = intFromStr<uint32_t>(headerInfoEntries.at(15));
    outHeaderInfo.unknown_0x40 = intFromStr<uint32_t>(headerInfoEntries.at(16));
    outHeaderInfo.unknown_0x44 = intFromStr<uint32_t>(headerInfoEntries.at(17));
    outHeaderInfo.originX = intFromStr<uint32_t>(headerInfoEntries.at(18));
    outHeaderInfo.originY = intFromStr<uint32_t>(headerInfoEntries.at(19));;
    outHeaderInfo.dataSize = intFromStr<uint32_t>(headerInfoEntries.at(20));
    outHeaderInfo.unknown_0x54 = intFromStr<uint32_t>(headerInfoEntries.at(21));
    return true;
  }

  static bool loadPaletteFromFile(const std::filesystem::path& palettePath, std::span<uint16_t> outPalette)
  {
    if (outPalette.size() * sizeof(uint16_t) != PALETTE_SIZE)
    {
      Log(LogLevel::ERROR, "Receiver span has not the right size for a palette.");
      return false;
    }
    if (!std::filesystem::is_regular_file(palettePath))
    {
      Log(LogLevel::ERROR, "Provided palette path is not a regular file.");
      return false;
    }
    if (PALETTE_SIZE != std::filesystem::file_size(palettePath))
    {
      Log(LogLevel::ERROR, "Provided palette has not the fitting size.");
      return false;
    }

    std::ifstream in;
    in.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    in.open(palettePath, std::ios::in | std::ios::binary);
    in.read(reinterpret_cast<char*>(outPalette.data()), PALETTE_SIZE);
    return true;
  }

  static bool readGm1ImageHeaderFromResourceMetaObject(const ResourceMetaFormat::ResourceMetaObjectReader& metaObject,
    uint32_t& outOffset, uint32_t& outSize, Gm1ImageHeader& outImageHeader)
  {
    Log(LogLevel::DEBUG, "Read Gm1ImageHeader object from meta file.");
    if (!isExpectedMetaObject(metaObject.getIdentifier(), Gm1ImageHeaderMeta::RESOURCE_IDENTIFIER, metaObject.getVersion(), Gm1ImageHeaderMeta::SUPPORTED_VERSIONS))
    {
      return false;
    }
    // version currently ignored, since only one available
    if (!hasExpectedEntryNumber(metaObject, Gm1ImageHeaderMeta::MAP_ENTRIES, Gm1ImageHeaderMeta::LIST_ENTRIES))
    {
      return false;
    }

    const auto& imageOffset{ getResourceObjectMapEntry(metaObject, Gm1ImageHeaderMeta::RESOURCE_IDENTIFIER, Gm1ImageHeaderMeta::OFFSET_KEY) };
    if (!imageOffset)
    {
      return false;
    }
    outOffset = intFromStr<uint32_t>(*imageOffset);

    const auto& imageSize{ getResourceObjectMapEntry(metaObject, Gm1ImageHeaderMeta::RESOURCE_IDENTIFIER, Gm1ImageHeaderMeta::SIZE_KEY) };
    if (!imageOffset)
    {
      return false;
    }
    outSize = intFromStr<uint32_t>(*imageSize);

    const auto& imageHeaderListEntries{ metaObject.getListEntries() };
    outImageHeader.width = intFromStr<uint16_t>(imageHeaderListEntries.at(0));
    outImageHeader.height = intFromStr<uint16_t>(imageHeaderListEntries.at(1));
    outImageHeader.offsetX = intFromStr<uint16_t>(imageHeaderListEntries.at(2));
    outImageHeader.offsetY = intFromStr<uint16_t>(imageHeaderListEntries.at(3));
    return true;
  }

  static bool readGm1TileObjectImageInfoFromResourceMetaObject(const ResourceMetaFormat::ResourceMetaObjectReader& metaObject,
    Gm1TileObjectImageInfo& outGm1TileObjectImageInfo)
  {
    Log(LogLevel::DEBUG, "Read Gm1TileObjectImageInfo object from meta file.");
    if (!isExpectedMetaObject(metaObject.getIdentifier(), Gm1TileObjectImageInfoMeta::RESOURCE_IDENTIFIER,
      metaObject.getVersion(), Gm1TileObjectImageInfoMeta::SUPPORTED_VERSIONS))
    {
      return false;
    }
    // version currently ignored, since only one available
    if (!hasExpectedEntryNumber(metaObject, Gm1TileObjectImageInfoMeta::MAP_ENTRIES, Gm1TileObjectImageInfoMeta::LIST_ENTRIES))
    {
      return false;
    }

    const auto& imageHeaderListEntries{ metaObject.getListEntries() };
    outGm1TileObjectImageInfo.imagePart = intFromStr<uint8_t>(imageHeaderListEntries.at(0));
    outGm1TileObjectImageInfo.subParts = intFromStr<uint8_t>(imageHeaderListEntries.at(1));
    outGm1TileObjectImageInfo.tileOffset = intFromStr<uint16_t>(imageHeaderListEntries.at(2));
    outGm1TileObjectImageInfo.imagePosition = static_cast<Gm1TileObjectImagePosition>(intFromStr<int8_t, 0, static_cast<int8_t>(Gm1TileObjectImagePosition::NONE), static_cast<int8_t>(Gm1TileObjectImagePosition::UPPER_RIGHT)>(imageHeaderListEntries.at(3)));
    outGm1TileObjectImageInfo.imageOffsetX = intFromStr<int8_t>(imageHeaderListEntries.at(4));
    outGm1TileObjectImageInfo.imageWidth = intFromStr<uint8_t>(imageHeaderListEntries.at(5));
    outGm1TileObjectImageInfo.animatedColor = intFromStr<uint8_t>(imageHeaderListEntries.at(6));
    return true;
  }

  static bool readGm1GeneralImageInfoFromResourceMetaObject(const ResourceMetaFormat::ResourceMetaObjectReader& metaObject,
    Gm1GeneralImageInfo& outGm1GeneralImageInfo)
  {
    Log(LogLevel::DEBUG, "Read Gm1GeneralImageInfo object from meta file.");
    if (!isExpectedMetaObject(metaObject.getIdentifier(), Gm1GeneralImageInfoMeta::RESOURCE_IDENTIFIER,
      metaObject.getVersion(), Gm1GeneralImageInfoMeta::SUPPORTED_VERSIONS))
    {
      return false;
    }
    // version currently ignored, since only one available
    if (!hasExpectedEntryNumber(metaObject, Gm1GeneralImageInfoMeta::MAP_ENTRIES, Gm1GeneralImageInfoMeta::LIST_ENTRIES))
    {
      return false;
    }

    const auto& imageHeaderListEntries{ metaObject.getListEntries() };
    outGm1GeneralImageInfo.relativeDataPos = intFromStr<int16_t>(imageHeaderListEntries.at(0));
    outGm1GeneralImageInfo.fontRelatedSize = intFromStr<int16_t>(imageHeaderListEntries.at(1));
    outGm1GeneralImageInfo.unknown_0x4 = intFromStr<uint8_t>(imageHeaderListEntries.at(2));
    outGm1GeneralImageInfo.unknown_0x5 = intFromStr<uint8_t>(imageHeaderListEntries.at(3));
    outGm1GeneralImageInfo.unknown_0x6 = intFromStr<uint8_t>(imageHeaderListEntries.at(4));
    outGm1GeneralImageInfo.flags = intFromStr<uint8_t, 2>(imageHeaderListEntries.at(5));
    return true;
  }

  UniqueGm1ResourcePointer loadGm1ResourceFromRaw(const std::filesystem::path& folder)
  {
    throw std::exception{ "Not yet implemented." };
  }

  static void writeGm1HeaderInfoToResourceMetaObject(const Gm1HeaderInfo& headerInfo, ResourceMetaFormat::ResourceMetaFileWriter& metaWriter)
  {
    Log(LogLevel::DEBUG, "Write Gm1Header info object to meta file.");
    metaWriter.startObject(Gm1HeaderMeta::RESOURCE_IDENTIFIER, Gm1HeaderMeta::CURRENT_VERSION)
      .writeListEntry(std::to_string(headerInfo.unknown_0x0), Gm1HeaderMeta::COMMENT_UNKNOWN_0x0)
      .writeListEntry(std::to_string(headerInfo.unknown_0x4), Gm1HeaderMeta::COMMENT_UNKNOWN_0x4)
      .writeListEntry(std::to_string(headerInfo.unknown_0x8), Gm1HeaderMeta::COMMENT_UNKNOWN_0x8)
      .writeListEntry(std::to_string(headerInfo.numberOfPicturesInFile), Gm1HeaderMeta::COMMENT_NUMBER_OF_PICTURES_IN_FILE)
      .writeListEntry(std::to_string(headerInfo.unknown_0x10), Gm1HeaderMeta::COMMENT_UNKNOWN_0x10)
      .writeListEntry(std::to_string(static_cast<int32_t>(headerInfo.gm1Type)), Gm1HeaderMeta::COMMENT_GM1_TYPE)
      .writeListEntry(std::to_string(headerInfo.unknown_0x18), Gm1HeaderMeta::COMMENT_UNKNOWN_0x18)
      .writeListEntry(std::to_string(headerInfo.unknown_0x1C), Gm1HeaderMeta::COMMENT_UNKNOWN_0x1C)
      .writeListEntry(std::to_string(headerInfo.unknown_0x20), Gm1HeaderMeta::COMMENT_UNKNOWN_0x20)
      .writeListEntry(std::to_string(headerInfo.unknown_0x24), Gm1HeaderMeta::COMMENT_UNKNOWN_0x24)
      .writeListEntry(std::to_string(headerInfo.unknown_0x28), Gm1HeaderMeta::COMMENT_UNKNOWN_0x28)
      .writeListEntry(std::to_string(headerInfo.unknown_0x2C), Gm1HeaderMeta::COMMENT_UNKNOWN_0x2C)
      .writeListEntry(std::to_string(headerInfo.width), Gm1HeaderMeta::COMMENT_WIDTH)
      .writeListEntry(std::to_string(headerInfo.height), Gm1HeaderMeta::COMMENT_HEIGHT)
      .writeListEntry(std::to_string(headerInfo.unknown_0x38), Gm1HeaderMeta::COMMENT_UNKNOWN_0x38)
      .writeListEntry(std::to_string(headerInfo.unknown_0x3C), Gm1HeaderMeta::COMMENT_UNKNOWN_0x3C)
      .writeListEntry(std::to_string(headerInfo.unknown_0x40), Gm1HeaderMeta::COMMENT_UNKNOWN_0x40)
      .writeListEntry(std::to_string(headerInfo.unknown_0x44), Gm1HeaderMeta::COMMENT_UNKNOWN_0x44)
      .writeListEntry(std::to_string(headerInfo.originX), Gm1HeaderMeta::COMMENT_ORIGIN_X)
      .writeListEntry(std::to_string(headerInfo.originY), Gm1HeaderMeta::COMMENT_ORIGIN_Y)
      .writeListEntry(std::to_string(headerInfo.dataSize), Gm1HeaderMeta::COMMENT_DATA_SIZE)
      .writeListEntry(std::to_string(headerInfo.unknown_0x54), Gm1HeaderMeta::COMMENT_UNKNOWN_0x54)
      .endObject();
  }

  static void savePaletteToFile(const std::filesystem::path& palettePath, std::span<const uint16_t> palette)
  {
    std::ofstream out;
    out.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    out.open(palettePath, std::ios::out | std::ios::trunc | std::ios::binary);
    out.write(reinterpret_cast<const char*>(palette.data()), palette.size() * sizeof(uint16_t));
  }

  static void writeGm1ImageHeaderToResourceMetaObject(const uint32_t offset, const uint32_t size, const Gm1ImageHeader& imageHeader,
    ResourceMetaFormat::ResourceMetaFileWriter& metaWriter)
  {
    Log(LogLevel::DEBUG, "Write Gm1ImageHeader object to meta file.");
    metaWriter.startObject(Gm1ImageHeaderMeta::RESOURCE_IDENTIFIER, Gm1ImageHeaderMeta::CURRENT_VERSION)
      .writeMapEntry(Gm1ImageHeaderMeta::OFFSET_KEY, std::to_string(offset))
      .writeMapEntry(Gm1ImageHeaderMeta::SIZE_KEY, std::to_string(size))
      .writeListEntry(std::to_string(imageHeader.width), Gm1ImageHeaderMeta::COMMENT_WIDTH)
      .writeListEntry(std::to_string(imageHeader.height), Gm1ImageHeaderMeta::COMMENT_HEIGHT)
      .writeListEntry(std::to_string(imageHeader.offsetX), Gm1ImageHeaderMeta::COMMENT_OFFSET_X)
      .writeListEntry(std::to_string(imageHeader.offsetY), Gm1ImageHeaderMeta::COMMENT_OFFSET_Y)
      .endObject();
  }

  static void writeGm1TileObjectImageInfoToResourceMetaObject(const Gm1TileObjectImageInfo& gm1TileObjectImageInfo,
    ResourceMetaFormat::ResourceMetaFileWriter& metaWriter)
  {
    Log(LogLevel::DEBUG, "Write Gm1TileObjectImageInfo object to meta file.");
    metaWriter.startObject(Gm1TileObjectImageInfoMeta::RESOURCE_IDENTIFIER, Gm1TileObjectImageInfoMeta::CURRENT_VERSION)
      .writeListEntry(std::to_string(gm1TileObjectImageInfo.imagePart), Gm1TileObjectImageInfoMeta::COMMENT_IMAGE_PART)
      .writeListEntry(std::to_string(gm1TileObjectImageInfo.subParts), Gm1TileObjectImageInfoMeta::COMMENT_SUB_PARTS)
      .writeListEntry(std::to_string(gm1TileObjectImageInfo.tileOffset), Gm1TileObjectImageInfoMeta::COMMENT_TILE_OFFSET)
      .writeListEntry(std::to_string(static_cast<int8_t>(gm1TileObjectImageInfo.imagePosition)), Gm1TileObjectImageInfoMeta::COMMENT_IMAGE_POSITION)
      .writeListEntry(std::to_string(gm1TileObjectImageInfo.imageOffsetX), Gm1TileObjectImageInfoMeta::COMMENT_IMAGE_OFFSET_X)
      .writeListEntry(std::to_string(gm1TileObjectImageInfo.imageWidth), Gm1TileObjectImageInfoMeta::COMMENT_IMAGE_WIDTH)
      .writeListEntry(std::to_string(gm1TileObjectImageInfo.animatedColor), Gm1TileObjectImageInfoMeta::COMMENT_ANIMATED_COLOR)
      .endObject();
  }

  static void writeGm1GeneralImageInfoToResourceMetaObject(const Gm1GeneralImageInfo& gm1GeneralImageInfo,
    ResourceMetaFormat::ResourceMetaFileWriter& metaWriter)
  {
    Log(LogLevel::DEBUG, "Write Gm1GeneralImageInfo object to meta file.");
    metaWriter.startObject(Gm1GeneralImageInfoMeta::RESOURCE_IDENTIFIER, Gm1GeneralImageInfoMeta::CURRENT_VERSION)
      .writeListEntry(std::to_string(gm1GeneralImageInfo.relativeDataPos), Gm1GeneralImageInfoMeta::COMMENT_RELATIVE_DATA_POS)
      .writeListEntry(std::to_string(gm1GeneralImageInfo.fontRelatedSize), Gm1GeneralImageInfoMeta::COMMENT_FONT_RELATED_SIZE)
      .writeListEntry(std::to_string(gm1GeneralImageInfo.unknown_0x4), Gm1GeneralImageInfoMeta::COMMENT_UNKNOWN_0x4)
      .writeListEntry(std::to_string(gm1GeneralImageInfo.unknown_0x5), Gm1GeneralImageInfoMeta::COMMENT_UNKNOWN_0x5)
      .writeListEntry(std::to_string(gm1GeneralImageInfo.unknown_0x6), Gm1GeneralImageInfoMeta::COMMENT_UNKNOWN_0x6)
      .writeListEntry(std::to_string(gm1GeneralImageInfo.flags), Gm1GeneralImageInfoMeta::COMMENT_FLAGS)
      .endObject();
  }

  void saveGm1ResourceAsRaw(const std::filesystem::path& folder, const Gm1Resource& resource)
  {
    throw std::exception{ "Not yet implemented." };
  }
}
