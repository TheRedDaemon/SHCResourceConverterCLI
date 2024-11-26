#pragma once

#include <stdint.h>
#include <memory>

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
