#define _CRT_SECURE_NO_WARNINGS // used here since we are working with the C APIs and a single thread

#include "BinaryCFileReadHelper.h"

#include "Console.h"

BinaryCFileReadHelper::BinaryCFileReadHelper(const char* filepath) : file{ fopen(filepath, "rb") }
{
  if (!file)
  {
    Log(LogLevel::ERROR, "BinaryCFileReadHelper: Failed to open file with path '{}': {}", filepath, strerror(errno));
  }
}

BinaryCFileReadHelper::BinaryCFileReadHelper(FILE* file) : file{ file }
{
  if (!file)
  {
    Log(LogLevel::ERROR, "BinaryCFileReadHelper: Received invalid file pointer.");
  }
}

BinaryCFileReadHelper::~BinaryCFileReadHelper()
{
  if (file && fclose(file) != 0)
  {
    Log(LogLevel::ERROR, "BinaryCFileReadHelper: Failed to close file.", strerror(errno));
  }
}

bool BinaryCFileReadHelper::hasValidState() const
{
  return file && !ferror(file) && !feof(file);
}

bool BinaryCFileReadHelper::hasInvalidState() const
{
  return !hasValidState();
}

FILE* BinaryCFileReadHelper::get() const
{
  return file;
}


long BinaryCFileReadHelper::tell() const
{
  const long position{ ftell(file) };
  if (position == -1L)
  {
    Log(LogLevel::ERROR, "BinaryCFileReadHelper: Failed to tell file.", strerror(errno));
  }
  return position;
}

int BinaryCFileReadHelper::seek(long offset, int origin)
{
  const int result{ fseek(file, offset, origin) };
  if (result != 0)
  {
    Log(LogLevel::ERROR, "BinaryCFileReadHelper: Failed to seek file to offset {} with origin id {}.", offset, origin);
  }
  return result;
}

int BinaryCFileReadHelper::getSize()
{
  int size{ internalGetSize() };
  if (size < 0)
  {
    Log(LogLevel::ERROR, "BinaryCFileReadHelper: Failed to obtain file size.");
  }
  return size;
}

int BinaryCFileReadHelper::internalGetSize()
{
  const int currentPosition{ tell() };
  if (currentPosition < 0 || seek(0L, SEEK_END) != 0)
  {
    return -1L;
  }
  const int size = tell();
  if (size < 0 || seek(currentPosition, SEEK_SET) != 0)
  {
    return -1L;
  }
  return size;
}

size_t BinaryCFileReadHelper::read(void* buffer, size_t elementSize, size_t elementCount)
{
  clearerr(file); // ok, since we currently not use this info anywhere
  const size_t bytesRead{ fread(buffer, elementSize, elementCount, file) };
  if (ferror(file) != 0)
  {
    Log(LogLevel::ERROR, "BinaryCFileReadHelper: Encountered error while trying to read file.");
  }
  if (feof(file) != 0)
  {
    Log(LogLevel::ERROR, "BinaryCFileReadHelper: Encountered EOF while trying to read file.");
  }
  return bytesRead;
}
