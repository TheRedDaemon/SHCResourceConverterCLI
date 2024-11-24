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
      ::operator delete(p);
      throw;
    }
    ::operator delete(p);
  };
};

template<typename T, class... Args>
std::unique_ptr<T, ObjectWithAdditionalMemoryDeleter<T>> createWithAdditionalMemory(size_t additionalBytes, Args&&... args)
{
  void* data{ ::operator new(sizeof(T) + additionalBytes) };
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
