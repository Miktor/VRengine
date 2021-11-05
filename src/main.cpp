
#include <spdlog/sinks/stdout_color_sinks.h>

#include <cstdlib>

#include "application.hpp"

int main(const int /*argc*/, const char ** /*argv[]*/) {
  spdlog::info("Start");

  vre::Application app;

  try {
    app.Run();
  } catch (const std::exception &e) {
    SPDLOG_ERROR(e.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
