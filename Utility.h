#pragma once

#include <stdint.h>
#include <memory>
#include <string>
#include <stdexcept>

/* Smart ptr of object with additional memory */

template<typename T>
struct ObjectWithAdditionalMemoryDeleter
{
  void operator()(T* p) const
  {
    try
    {
      p->~T();
    }
    catch (...)
    {
      if constexpr (alignof(T) > __STDCPP_DEFAULT_NEW_ALIGNMENT__)
      {
        ::operator delete(p, std::align_val_t{ alignof(T) });
      }
      else
      {
        ::operator delete(p);
      }
      throw;
    }
    if constexpr (alignof(T) > __STDCPP_DEFAULT_NEW_ALIGNMENT__)
    {
      ::operator delete(p, std::align_val_t{ alignof(T) });
    }
    else
    {
      ::operator delete(p);
    }
  };
};

template<typename T, class... Args>
std::unique_ptr<T, ObjectWithAdditionalMemoryDeleter<T>> createWithAdditionalMemory(size_t additionalBytes, Args&&... args)
{
  void* data;
  if constexpr (alignof(T) > __STDCPP_DEFAULT_NEW_ALIGNMENT__)
  {
    data = ::operator new(sizeof(T) + additionalBytes, std::align_val_t{ alignof(T) });
  }
  else
  {
    data = ::operator new(sizeof(T) + additionalBytes);
  }
  try
  {
    return std::unique_ptr<T, ObjectWithAdditionalMemoryDeleter<T>>{ new (data) T(std::forward<Args>(args)...) };
  }
  catch (...)
  {
    ::operator delete(data);
    throw;
  }
}

/* String helpers */

void trimLeadingWhitespaceInPlace(std::string& str);
void trimTrailingWhitespaceInPlace(std::string& str);
void trimLeadingAndTrailingWhitespaceInPlace(std::string& str);

/* Value from String helper */

template<typename Int = int, int base = 0, // auto-detect base by default
  Int min = std::numeric_limits<Int>::min(), Int max = std::numeric_limits<Int>::max()>
Int intFromStr(const std::string& str)
{
  size_t countOfUsedChars{ 0 };
  const long long result{ std::stoll(str, &countOfUsedChars, base) };
  if (countOfUsedChars != str.size())
  {
    throw std::invalid_argument("Number does not fill given string.");
  }
  if (result > max || result < min)
  {
    throw std::out_of_range("Value is out of range.");
  }
  return static_cast<Int>(result);
}

template<typename UInt = unsigned int, int base = 0, // auto-detect base by default
  UInt min = std::numeric_limits<UInt>::min(), UInt max = std::numeric_limits<UInt>::max()>
UInt uintFromStr(const std::string& str)
{
  size_t countOfUsedChars{ 0 };
  const unsigned long long result{ std::stoull(str, &countOfUsedChars, base) };
  if (countOfUsedChars != str.size())
  {
    throw std::invalid_argument("Number does not fill given string.");
  }
  if (result > max || result < min)
  {
    throw std::out_of_range("Value is out of range.");
  }
  return static_cast<UInt>(result);
}

bool boolFromStr(const std::string& str);
