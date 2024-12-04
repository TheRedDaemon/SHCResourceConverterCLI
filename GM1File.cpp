#include "GM1File.h"

namespace GM1File
{
  void validateGm1Resource(const Gm1Resource& resource)
  {
    throw std::exception{ "Not yet implemented." };
  }

  UniqueGm1ResourcePointer loadGm1Resource(const std::filesystem::path& file)
  {
    throw std::exception{ "Not yet implemented." };
  }

  void saveGm1Resource(const std::filesystem::path& file, const Gm1Resource& resource)
  {
    throw std::exception{ "Not yet implemented." };
  }

  UniqueGm1ResourcePointer loadGm1ResourceFromRaw(const std::filesystem::path& folder)
  {
    throw std::exception{ "Not yet implemented." };
  }

  void saveGm1ResourceAsRaw(const std::filesystem::path& folder, const Gm1Resource& resource)
  {
    throw std::exception{ "Not yet implemented." };
  }
}
