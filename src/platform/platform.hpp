#pragma once

#include <vector>

namespace vre::platform {

struct Platform {
  static std::vector<char> ReadFile(const std::string &filename);
};

}  // namespace vre::platform