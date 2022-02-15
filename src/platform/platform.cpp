#include "platform.hpp"

#include <fstream>
#include <string>

#include "common.hpp"

namespace vre::platform {

std::string Platform::ReadFile(const std::string &filename, bool throw_if_not_exists) {
  std::ifstream file(filename, std::ios::binary);

  if (!file.is_open()) {
    if (throw_if_not_exists) {
      throw std::runtime_error("failed to open file!");
    }
    return std::string();
  }

  const auto file_size = file.tellg();
  std::vector<char> buffer(file_size);

  file.seekg(0);
  file.read(buffer.data(), file_size);

  file.close();

  return std::string(buffer.data(), file_size);
}

void Platform::WriteFile(const std::string &filename, const void *data, size_t size) {
  std::ofstream file(filename, std::ios::binary | std::ios::trunc);

  if (!file.is_open()) {
    throw std::runtime_error("failed to open file!");
  }

  file.write(reinterpret_cast<const char *>(data), size);
  file.close();
}

}  // namespace vre::platform