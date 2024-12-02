#pragma once

/*
  RESOURCE META FORMAT VERSION 1
  Very simple format to store meta information about a resource as a text file.
  - Objects:
    - The text file consists of a list of "objects" separated by empty newlines. The order has therefore meaning and parsing should remember this.
    - An object starts with an identifier followed by a space and a version number.
      - The identifier describes a handler for the object.
      - The version allows the handler to consider changes to the object structure in the future.
    - Now, on every line without a space, there is either:
      - a list item character '-', followed by an arbitrary value
      - a map item character ':', followed by an arbitrary value forming they key until the map separator character '=' is encountered, followed by an arbitrary value.
    - Map entries have no meaning in their order. Line entries have meaning in their order and need to be read with this in mind. Both entries can mix in the file, but it is recommended to start with the map entries.
    - The map entry keys are conceptually unique within the object. Repeating keys are valid, but only the last value will be used. 
    - Empty list entries, keys and values are allowed. And empty list entry would only contain a '-', an map entry with empty key and value would only contain ':='.
    - Due to the way key and value are split, '=' is not allowed to be part of the key, however, it is allowed to be part of the value. The following is valid: ':key=='.
    - The objects ends with a newline.
  - If a comment character '#' is encountered, every other character until the end of the line is ignored.
  - All values and keys are considered strings in the file. No type information is carried by the format itself.
  - Whitespace:
    - Leading and trailing whitespace is ignored when parsing line content. Trailing whitespace is anything after the last non-whitespace character that is NOT a comment marker or comment text.
    - Additional whitespace around keys or values is ignored and not part of the key or value, but valid.
    - Whitespace is generally considered the same as what "isspace" in C considers whitespace.
  - Header:
    - The first identifier in the file is the RESOURCE_META_HEADER identifier, followed by the version number, and can contain any kind of meta data.
      - However, to ensure the header can always be parsed, the header object will always be handled as version 1 object.
    - A file without a header entry is invalid. A file with objects that can not be parsed is invalid. It is recommended to stop the parsing in this case an communicate the error.

  EXAMPLE:
  """
    RESOURCE_META_HEADER 1
    : key 1 = value 1
    - list entry 1
    - list entry 2

    OBJECT 1
    :key 1=value 1
    :key 2=value 2
    -list entry 1
    -list entry 2
    -list entry 3
  """
 */

#include <string>
#include <vector>
#include <unordered_map>

namespace VERSION
{
  inline constexpr int HEADER{ 1 };
  inline constexpr int CURRENT{ 1 };
}

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

class ResourceMetaObjectReader
{
private:
  std::string identifier;
  int version;
  std::vector<std::string> listEntries;
  std::unordered_map<std::string, std::string> mapEntries;

  explicit ResourceMetaObjectReader(std::string&& identifier, int version, std::vector<std::string>&& listEntries, std::unordered_map<std::string, std::string>&& mapEntries);

  static std::string extractMeaningfulLine(std::istream& stream);
public:
  ~ResourceMetaObjectReader();

  const std::string& getIdentifier() const;
  int getVersion() const;

  // expects to start from the identifier
  static ResourceMetaObjectReader parseFrom(std::istream& stream, int formatVersion);

  ResourceMetaObjectReader(ResourceMetaObjectReader&& resourceMetaObject) = default;
  ResourceMetaObjectReader& operator=(ResourceMetaObjectReader&& resourceMetaObject) = default;

  ResourceMetaObjectReader(const ResourceMetaObjectReader&) = delete;
  ResourceMetaObjectReader& operator=(const ResourceMetaObjectReader&) = delete;
};

class ResourceMetaFileReader
{
private:
  ResourceMetaObjectReader header;
  std::vector<ResourceMetaObjectReader> objects;

  explicit ResourceMetaFileReader(ResourceMetaObjectReader&& header, std::vector<ResourceMetaObjectReader>&& objects);

  // returns true if a new object was found
  static bool consumeTillObject(std::istream& stream);
public:
  ~ResourceMetaFileReader();

  static ResourceMetaFileReader parseFrom(std::istream& stream);

  ResourceMetaFileReader(ResourceMetaFileReader&& resourceMetaFile) = default;
  ResourceMetaFileReader& operator=(ResourceMetaFileReader&& resourceMetaFile) = default;

  ResourceMetaFileReader(const ResourceMetaFileReader&) = delete;
  ResourceMetaFileReader& operator=(const ResourceMetaFileReader&) = delete;
};
