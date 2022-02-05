#pragma once

#ifndef NDEBUG
#define VR_ASSERT(x)                            \
  do {                                          \
    if (!bool(x)) {                             \
      SPDLOG_ERROR("Assertion failed: {}", #x); \
      abort();                                  \
    }                                           \
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