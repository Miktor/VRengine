#pragma once

#include <execinfo.h>
#include <unistd.h>

#ifndef NDEBUG
#define VR_ASSERT(x)                                    \
  do {                                                  \
    if (!bool(x)) {                                     \
      SPDLOG_ERROR("Assertion failed: {}", #x);         \
      void *array[10];                                  \
      size_t size;                                      \
      size = backtrace(array, 10);                      \
      backtrace_symbols_fd(array, size, STDERR_FILENO); \
      abort();                                          \
    }                                                   \
  } while (0)
#else
#define VR_ASSERT(x) ((void)0)
#endif

#define VR_CHECK(x)                         \
  do {                                      \
    if (!bool(x)) {                         \
      SPDLOG_ERROR("Check failed: {}", #x); \
      abort();                              \
    }                                       \
  } while (0)

#define CHECK_VK_SUCCESS(x)                               \
  do {                                                    \
    if (VK_SUCCESS != (x)) {                              \
      SPDLOG_ERROR("Vulkan error: {} != VK_SUCCESS", #x); \
      abort();                                            \
    }                                                     \
  } while (0)