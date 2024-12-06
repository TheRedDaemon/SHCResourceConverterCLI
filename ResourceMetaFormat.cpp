#include "ResourceMetaFormat.h"

#include "Utility.h"
#include "Console.h"

#include <iostream>

namespace ResourceMetaFormat
{
  constexpr auto max_size = std::numeric_limits<std::streamsize>::max();


  /* ResourceMetaObjectReader */

  ResourceMetaObjectReader::ResourceMetaObjectReader(std::string&& identifier, int version,
    std::vector<std::string>&& listEntries, std::unordered_map<std::string, std::string>&& mapEntries)
    : identifier(std::move(identifier)), version(version), listEntries(std::move(listEntries)), mapEntries(std::move(mapEntries))
  {
  }

  std::string ResourceMetaObjectReader::extractMeaningfulLine(std::istream& stream)
  {
    std::string line{};
    if (stream.eof())
    {
      return line;
    }
    std::getline(stream, line);

    size_t startIndex{ 0 };
    size_t endIndex{ 0 };
    for (size_t i{ 0 }; i < line.size(); ++i)
    {
      const char c{ line[i] };
      if (std::isspace(static_cast<unsigned char>(c))) // requires unsigned char based on docs
      {
        if (startIndex == endIndex)
        {
          startIndex = i + 1;
          endIndex = i + 1;
        }
        continue;
      }
      if (c == MARKER::COMMENT_CHARACTER)
      {
        break;
      }
      endIndex = i + 1;
    }

    return line.erase(endIndex).erase(0, startIndex);
  }

  ResourceMetaObjectReader::~ResourceMetaObjectReader() {}

  const std::string& ResourceMetaObjectReader::getIdentifier() const
  {
    return identifier;
  }

  int ResourceMetaObjectReader::getVersion() const
  {
    return version;
  }

  const std::vector<std::string>& ResourceMetaObjectReader::getListEntries() const
  {
    return listEntries;
  }

  const std::unordered_map<std::string, std::string>& ResourceMetaObjectReader::getMapEntries() const
  {
    return mapEntries;
  }

  ResourceMetaObjectReader ResourceMetaObjectReader::parseFrom(std::istream& stream, int formatVersion)
  {
    // format version currently ignored

    const std::ios::iostate oldExceptions = stream.exceptions();
    stream.exceptions(std::ios::failbit | std::ios::badbit);
    try
    {
      std::string line{ extractMeaningfulLine(stream) };

      // identifier
      if (line.empty())
      {
        throw std::ios::failure("No identifier found in given initial line.");
      }
      const size_t identifierSeparatorIndex{ line.find(MARKER::SPACE_CHARACTER) };
      if (identifierSeparatorIndex == std::string::npos)
      {
        throw std::ios::failure("No valid identifier format found in initial line.");
      }
      std::string identifier{ line.substr(0, identifierSeparatorIndex) };
      trimLeadingAndTrailingWhitespaceInPlace(identifier);

      std::string versionString{ line.substr(identifierSeparatorIndex + sizeof(MARKER::SPACE_CHARACTER)) };
      trimLeadingAndTrailingWhitespaceInPlace(versionString);

      int version{ 0 };
      try
      {
        version = intFromStr(versionString);
      }
      catch (const std::exception& ex)
      {
        std::string exMessage{ "Unable to extract version from identifier line: " };
        exMessage.append(ex.what());
        throw std::ios::failure(exMessage);
      }

      // extract keys and values 
      std::vector<std::string> listEntries{};
      std::unordered_map<std::string, std::string> mapEntries{};
      while (!(line = extractMeaningfulLine(stream)).empty())
      {
        if (line.starts_with(MARKER::LIST_ITEM_CHARACTER))
        {
          std::string value{ line.substr(sizeof(MARKER::LIST_ITEM_CHARACTER)) };
          trimLeadingAndTrailingWhitespaceInPlace(value);
          listEntries.emplace_back(std::move(value));
        }
        else if (line.starts_with(MARKER::MAP_ITEM_CHARACTER))
        {
          const size_t mapSeparatorIndex{ line.find(MARKER::MAP_SEPARATOR_CHARACTER) };
          if (mapSeparatorIndex == std::string::npos)
          {
            throw std::ios::failure("Encountered map entry line without map separator.");
          }
          std::string key{ line.substr(sizeof(MARKER::MAP_ITEM_CHARACTER), mapSeparatorIndex - sizeof(MARKER::MAP_ITEM_CHARACTER)) };
          trimLeadingAndTrailingWhitespaceInPlace(key);
          std::string value{ line.substr(mapSeparatorIndex + sizeof(MARKER::MAP_SEPARATOR_CHARACTER)) };
          trimLeadingAndTrailingWhitespaceInPlace(value);
          if (mapEntries.contains(key))
          {
            Log(LogLevel::WARNING, "Encountered map entry line with duplicate key '{}'. Overwriting previous value.", key);
          }
          mapEntries.insert_or_assign(std::move(key), std::move(value));
        }
        else
        {
          throw std::ios::failure("Encountered object entry line with no valid start format.");
        }
      }

      stream.exceptions(oldExceptions);
      return ResourceMetaObjectReader{ std::move(identifier), version, std::move(listEntries), std::move(mapEntries) };
    }
    catch (...)
    {
      stream.exceptions(oldExceptions);
      throw;
    }
  }


  /* ResourceMetaFileReader */

  ResourceMetaFileReader::ResourceMetaFileReader(ResourceMetaObjectReader&& header, std::vector<ResourceMetaObjectReader>&& objects)
    : header(std::move(header)), objects(std::move(objects))
  {
  }

  bool ResourceMetaFileReader::consumeTillObject(std::istream& stream)
  {
    while (!stream.eof())
    {
      stream >> std::ws; // remove whitespace
      if (stream.eof())
      {
        return false;
      }
      if (stream.peek() != MARKER::COMMENT_CHARACTER)
      {
        return true;
      }
      stream.ignore(max_size, MARKER::NEWLINE_CHARACTER);
    }
    return false;
  }

  ResourceMetaFileReader::~ResourceMetaFileReader() {}

  const ResourceMetaObjectReader& ResourceMetaFileReader::getHeader() const
  {
    return header;
  }

  const std::vector<ResourceMetaObjectReader>& ResourceMetaFileReader::getObjects() const
  {
    return objects;
  }

  ResourceMetaFileReader ResourceMetaFileReader::parseFrom(std::istream& stream)
  {
    const std::ios::iostate oldExceptions = stream.exceptions();
    stream.exceptions(std::ios::failbit | std::ios::badbit);
    try
    {
      if (!ResourceMetaFileReader::consumeTillObject(stream))
      {
        throw std::ios::failure("File is empty.");
      }
      ResourceMetaObjectReader header{ ResourceMetaObjectReader::parseFrom(stream, VERSION::HEADER) };  // header can only be version 1 format
      if (header.getIdentifier() != IDENTIFIER::RESOURCE_META_HEADER)
      {
        throw std::ios::failure("File is not a resource meta file, since it does not start with a header.");
      }
      // format version is ignored for now

      std::vector<ResourceMetaObjectReader> objects{};
      while (ResourceMetaFileReader::consumeTillObject(stream))
      {
        objects.emplace_back(ResourceMetaObjectReader::parseFrom(stream, header.getVersion()));
      }

      stream.exceptions(oldExceptions); // reset stream exception
      return ResourceMetaFileReader{ std::move(header), std::move(objects) };
    }
    catch (...)
    {
      stream.exceptions(oldExceptions);
      throw;
    }
  }


  /* ResourceMetaObjectWriter */

  ResourceMetaFileWriter::ResourceMetaObjectWriter::ResourceMetaObjectWriter(ResourceMetaFileWriter& parent) : parent{ parent }, active{ false }
  {
  }

  ResourceMetaFileWriter::ResourceMetaObjectWriter::~ResourceMetaObjectWriter()
  {
  }

  int ResourceMetaFileWriter::ResourceMetaObjectWriter::getFormatVersion() const
  {
    return parent.formatVersion;
  }

  bool ResourceMetaFileWriter::ResourceMetaObjectWriter::isObjectActive() const
  {
    return active;
  }

  bool ResourceMetaFileWriter::ResourceMetaObjectWriter::isFileActive() const
  {
    return parent.isFileActive();
  }

  ResourceMetaFileWriter::ResourceMetaObjectWriter& ResourceMetaFileWriter::ResourceMetaObjectWriter::startObject(
    std::string_view identifier, int version, std::string_view comment)
  {
    parent.validateFileActive();
    parent.validateObjectInactive();
    parent.validateStreamState();

    active = true;
    parent.internalStream << identifier << MARKER::SPACE_CHARACTER << version;
    parent.internalWriteComment(comment, true);
    parent.internalStream << MARKER::NEWLINE_CHARACTER;
    return *this;
  }

  ResourceMetaFileWriter::ResourceMetaObjectWriter& ResourceMetaFileWriter::ResourceMetaObjectWriter::writeListEntry(
    std::string_view entry, std::string_view comment)
  {
    parent.validateFileActive();
    parent.validateObjectActive();
    parent.validateStreamState();

    parent.internalStream << MARKER::LIST_ITEM_CHARACTER << MARKER::SPACE_CHARACTER << entry;
    parent.internalWriteComment(comment, true);
    parent.internalStream << MARKER::NEWLINE_CHARACTER;
    return *this;
  }

  ResourceMetaFileWriter::ResourceMetaObjectWriter& ResourceMetaFileWriter::ResourceMetaObjectWriter::writeMapEntry(
    std::string_view key, std::string_view value, std::string_view comment)
  {
    parent.validateFileActive();
    parent.validateObjectActive();
    parent.validateStreamState();

    parent.internalStream << MARKER::MAP_ITEM_CHARACTER << MARKER::SPACE_CHARACTER << key
      << MARKER::SPACE_CHARACTER << MARKER::MAP_SEPARATOR_CHARACTER << MARKER::SPACE_CHARACTER << value;
    parent.internalWriteComment(comment, true);
    parent.internalStream << MARKER::NEWLINE_CHARACTER;
    return *this;
  }

  ResourceMetaFileWriter& ResourceMetaFileWriter::ResourceMetaObjectWriter::endObject(std::string_view comment)
  {
    parent.validateFileActive();
    parent.validateObjectActive();
    parent.validateStreamState();

    parent.internalWriteComment(comment, false);
    parent.internalStream << MARKER::NEWLINE_CHARACTER;
    active = false;
    return parent;
  }


  /* ResourceMetaFileWriter */

  ResourceMetaFileWriter::ResourceMetaFileWriter(std::ostream& stream, int formatVersion)
    : formatVersion{ formatVersion }, internalStream{ stream }, active{ true }, headerPlaced{ false }, writerObject{ *this }
  {
  }

  void ResourceMetaFileWriter::validateObjectActive() const
  {
    if (!isObjectActive())
    {
      throw std::ios::failure("Object is not active.");
    }
  }

  void ResourceMetaFileWriter::validateObjectInactive() const
  {
    if (isObjectActive())
    {
      throw std::ios::failure("Object is still active.");
    }
  }

  void ResourceMetaFileWriter::validateFileActive() const
  {
    if (!isFileActive())
    {
      throw std::ios::failure("File is not active.");
    }
  }

  void ResourceMetaFileWriter::validateStreamState() const
  {
    if (internalStream.fail())
    {
      throw std::ios::failure("Stream is in error state.");
    }
    if (internalStream.bad())
    {
      throw std::ios::failure("Stream is in bad state.");
    }
  }

  void ResourceMetaFileWriter::internalWriteComment(std::string_view comment, bool prefixSpace)
  {
    if (comment.data() == nullptr)
    {
      return;
    }
    if (prefixSpace)
    {
      internalStream << MARKER::SPACE_CHARACTER;
    }
    internalStream << MARKER::COMMENT_CHARACTER << MARKER::SPACE_CHARACTER << comment;
  }

  ResourceMetaFileWriter::~ResourceMetaFileWriter()
  {
    try
    {
      if (isObjectActive())
      {
        writerObject.endObject();
      }
      if (isFileActive())
      {
        endFile();
      }
    }
    catch (const std::exception& ex)
    {
      Log(LogLevel::ERROR, "Encountered error during clean up of ResourceMetaFileWriter. File might be in an invalid state: {}", ex.what());
    }
  }

  int ResourceMetaFileWriter::getFormatVersion() const
  {
    return formatVersion;
  }

  bool ResourceMetaFileWriter::isObjectActive() const
  {
    return writerObject.isObjectActive();
  }

  bool ResourceMetaFileWriter::isFileActive() const
  {
    return active;
  }

  ResourceMetaFileWriter::ResourceMetaObjectWriter& ResourceMetaFileWriter::startHeader(std::string_view comment)
  {
    if (headerPlaced)
    {
      throw std::ios::failure("Header already placed.");
    }
    headerPlaced = true;
    return writerObject.startObject(IDENTIFIER::RESOURCE_META_HEADER, VERSION::HEADER, comment);
  }
  ResourceMetaFileWriter::ResourceMetaObjectWriter& ResourceMetaFileWriter::startObject(std::string_view identifier, int version, std::string_view comment)
  {
    if (!headerPlaced)
    {
      throw std::ios::failure("Header has not been written yet. Can not create object.");
    }
    return writerObject.startObject(identifier, version, comment);
  }

  ResourceMetaFileWriter& ResourceMetaFileWriter::newline(std::string_view comment)
  {
    validateFileActive();
    validateObjectInactive();
    validateStreamState();

    internalWriteComment(comment, false);
    internalStream << MARKER::NEWLINE_CHARACTER;
    return *this;
  }

  void ResourceMetaFileWriter::endFile()
  {
    validateFileActive();
    // does not need to validate stream, since inactive
    active = false; // nothing special to do here, only complete the file
  }

  ResourceMetaFileWriter ResourceMetaFileWriter::startFile(std::ostream& stream, int formatVersion)
  {
    // only one version currently supported
    if (formatVersion != VERSION::CURRENT)
    {
      throw std::ios::failure("Unsupported format version.");
    }

    return ResourceMetaFileWriter{ stream, formatVersion };
  }
}
