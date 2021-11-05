#pragma once

#include "common.hpp"

namespace vre {

namespace scene {
struct Node;
}

namespace serialization {
class GLTFLoader {
 public:
  static std::vector<std::shared_ptr<scene::Node>> LoadFromFile(const std::string &filename);
};

}  // namespace serialization
}  // namespace vre