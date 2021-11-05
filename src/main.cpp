
#include <spdlog/sinks/stdout_color_sinks.h>

#include <cstdlib>

#include "application.hpp"

int main(const int /*argc*/, const char ** /*argv[]*/) {
  spdlog::info("Start");

  vre::Application app;

  try {
    app.Run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
