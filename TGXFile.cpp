#include "TGXFile.h"

namespace TGXFile
{
  void validateTgxResource(const TgxResource& resource)
  {
    throw std::exception{ "Not yet implemented." };
  }

  std::unique_ptr<TgxResource> loadTgxResource(const std::filesystem::path& file)
  {
    throw std::exception{ "Not yet implemented." };
  }

  void saveTgxResource(const std::filesystem::path& file, const TgxResource& resource)
  {
    throw std::exception{ "Not yet implemented." };
  }

  std::unique_ptr<TgxResource> loadTgxResourceFromRaw(const std::filesystem::path& folder)
  {
    throw std::exception{ "Not yet implemented." };
  }

  void saveTgxResourceAsRaw(const std::filesystem::path& folder, const TgxResource& resource)
  {
    throw std::exception{ "Not yet implemented." };
  }
}
