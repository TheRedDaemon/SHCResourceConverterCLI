#pragma once

/*
  RESOURCE META FORMAT
  Very simple format to store meta information about a resource as a text file.
  - The text file consists of a list of "objects" separated by empty newlines. The order has therefore meaning and parsing should remember this.
  - An object starts with an identifier followed by a space and a version number.
  - Now, on every line without a space, there is either:
    - a list item character '-', followed by a space, followed by an arbitrary value
    - a map item character ':', followed by a space, followed by an arbitrary value until the char combination of a space, the map separator character '=' and a space, followed by an arbitrary value
  - Map entries have no meaning in their order. Line entries have meaning in their order and need to be read with this in mind. Both entries can mix in the file, but it is recommended to start with the map entries.
  - The objects ends with a newline.
  - If a comment character '#' is encountered, every other character until the end of the line is ignored.
  - All values and keys are handled as strings during read and write operations. The user has to take care of conversions.
  - Leading and trailing whitespace is ignored.
  - The first identifier in the file is the RESOURCE_META_HEADER identifier, followed by the version number, and can contain any kind of meta data.
  - A file without a header entry is invalid. A file with objects that can not be parsed is invalid. The processing of an invalid file should stop and communicate an error.
*/

#include <string>
#include <vector>
#include <unordered_map>

namespace MARKER
{ 
  inline constexpr char SPACE_CHARACTER{ ' ' };
  inline constexpr char COMMENT_CHARACTER{ '#' };
  inline constexpr char LIST_ITEM_CHARACTER{ '-' };
  inline constexpr char MAP_ITEM_CHARACTER{ ':' };
  inline constexpr char MAP_SEPARATOR_CHARACTER{ '=' };
}

namespace IDENTIFIER
{
  inline const std::string RESOURCE_META_HEADER{ "RESOURCE_META_HEADER" };
}

class ResourceMetaObject
{
private:
  std::string identifier;
  int version;
  std::vector<std::string> listEntries;
  std::unordered_map<std::string, std::string> mapEntries;

  explicit ResourceMetaObject(std::string&& identifier, int version, std::vector<std::string>&& listEntries, std::unordered_map<std::string, std::string>&& mapEntries);
public:
  ~ResourceMetaObject();

  const std::string& getIdentifier() const;
  int getVersion() const;

  // expects to start from the identifier
  static ResourceMetaObject parseFrom(std::istream& stream);

  ResourceMetaObject(ResourceMetaObject&& resourceMetaObject) = default;
  ResourceMetaObject& operator=(ResourceMetaObject&& resourceMetaObject) = default;

  ResourceMetaObject(const ResourceMetaObject&) = delete;
  ResourceMetaObject& operator=(const ResourceMetaObject&) = delete;
};

class ResourceMetaFile
{
private:
  ResourceMetaObject header;
  std::vector<ResourceMetaObject> objects;

  explicit ResourceMetaFile(ResourceMetaObject&& header, std::vector<ResourceMetaObject>&& objects);

  // returns true if a new object was found
  static bool consumeTillObject(std::istream& stream);
public:
  ~ResourceMetaFile();

  static ResourceMetaFile parseFrom(std::istream& stream);

  ResourceMetaFile(ResourceMetaFile&& resourceMetaFile) = default;
  ResourceMetaFile& operator=(ResourceMetaFile&& resourceMetaFile) = default;

  ResourceMetaFile(const ResourceMetaFile&) = delete;
  ResourceMetaFile& operator=(const ResourceMetaFile&) = delete;
};
