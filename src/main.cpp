#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <memory>

#include <spdlog/sinks/stdout_color_sinks.h>

#include "application.hpp"

void Handler(int sig) {
  void *array[10];
  size_t size;

  size = backtrace(array, 10);
  fprintf(stderr, "Error: signal %d:\n", sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(sig);
}

int main(const int /*argc*/, const char ** /*argv[]*/) {
  //signal(SIGSEGV, Handler);
  //signal(SIGSTOP, Handler);

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
