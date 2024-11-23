#pragma once

#include <cstdio>

class BinaryCFileReadHelper
{
private:
  FILE* file;
  
public:
  BinaryCFileReadHelper(const char* file);
  BinaryCFileReadHelper(FILE* file);
  ~BinaryCFileReadHelper();

  BinaryCFileReadHelper(const BinaryCFileReadHelper&) = delete;
  BinaryCFileReadHelper& operator=(const BinaryCFileReadHelper&) = delete;

  bool hasValidState() const;
  bool hasInvalidState() const;
  FILE* get() const;
  long tell() const;

  int seek(long offset, int origin);
  int getSize();
  size_t read(void* buffer, size_t elementSize, size_t elementCount);

private:
  int internalGetSize();
};

