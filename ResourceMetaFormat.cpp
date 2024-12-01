#include "ResourceMetaFormat.h"

#include <iostream>

constexpr auto max_size = std::numeric_limits<std::streamsize>::max();

ResourceMetaObject::ResourceMetaObject(std::string&& identifier, int version,
  std::vector<std::string>&& listEntries, std::unordered_map<std::string, std::string>&& mapEntries)
  : identifier(std::move(identifier)), version(version), listEntries(std::move(listEntries)), mapEntries(std::move(mapEntries))
{
}

ResourceMetaObject::~ResourceMetaObject() {}

const std::string& ResourceMetaObject::getIdentifier() const
{
  return identifier;
}

int ResourceMetaObject::getVersion() const
{
  return version;
}


ResourceMetaObject ResourceMetaObject::parseFrom(std::istream& stream)
{
  // TODO: create helper to reduce string to be without whitespace at start and no whitespace at end between last value and comment or newline
  // extract header
  // extract individual entries

  throw std::exception("Not implemented.");
}


ResourceMetaFile::ResourceMetaFile(ResourceMetaObject&& header, std::vector<ResourceMetaObject>&& objects)
  : header(std::move(header)), objects(std::move(objects))
{
}

bool ResourceMetaFile::consumeTillObject(std::istream& stream)
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
}

ResourceMetaFile::~ResourceMetaFile() {}

ResourceMetaFile ResourceMetaFile::parseFrom(std::istream& stream)
{
  if (!ResourceMetaFile::consumeTillObject(stream))
  {
    throw std::ios::failure("File is empty.");
  }
  ResourceMetaObject header{ ResourceMetaObject::parseFrom(stream) };
  if (header.getIdentifier() != IDENTIFIER::RESOURCE_META_HEADER)
  {
    throw std::ios::failure("File is not a resource meta file, since it does not start with a header.");
  }
  // file version is ignored for now

  std::vector<ResourceMetaObject> objects{};
  while (ResourceMetaFile::consumeTillObject(stream))
  {
    objects.emplace_back(ResourceMetaObject::parseFrom(stream));
  }

  return ResourceMetaFile{ std::move(header), std::move(objects) };
}