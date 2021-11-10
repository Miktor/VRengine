#pragma once

#include <vector>

namespace vre::platform {

struct Platform {
  static std::string ReadFile(const std::string &filename);
};

}  // namespace vre::platform