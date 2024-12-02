#include "ResourceMetaFormat.h"

#include "Utility.h"

#include <iostream>

constexpr auto max_size = std::numeric_limits<std::streamsize>::max();

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
      size_t countOfUsedVersionChars{ 0 };
      version = std::stoi(versionString, &countOfUsedVersionChars);
      if (countOfUsedVersionChars != versionString.size())
      {
        throw std::ios::failure("Version in identifier line is not a valid integer.");
      }
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
    stream.ignore(max_size, '\n');
  }
  return false;
}

ResourceMetaFileReader::~ResourceMetaFileReader() {}

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
  catch (...) {
    stream.exceptions(oldExceptions);
    throw;
  }
}
