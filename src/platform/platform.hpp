#pragma once

#include <vector>

namespace vre::platform {

struct Platform {
  static std::string ReadFile(const std::string &filename, bool throw_if_not_exists = true);
  static void WriteFile(const std::string &filename, const void *data, size_t size);
};

}  // namespace vre::platform