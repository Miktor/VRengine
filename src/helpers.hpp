#pragma once

#ifndef NDEBUG
#define VR_ASSERT(x)                                                                     \
  do {                                                                                   \
    if (!bool(x)) {                                                                      \
      SPDLOG_ERROR("Assertation failed, %s != true @ %s:%d.\n", #x, __FILE__, __LINE__); \
      abort();                                                                           \
    }                                                                                    \
  } while (0)
#else
#define VR_ASSERT(x) ((void)0)
#endif

#define VR_CHECK(x)                                                                     \
  do {                                                                                   \
    if (!bool(x)) {                                                                      \
      SPDLOG_ERROR("Check failed, %s != true @ %s:%d.\n", #x, __FILE__, __LINE__); \
      abort();                                                                           \
    }                                                                                    \
  } while (0)

#define CHECK_VK_SUCCESS(x)                                                              \
  do {                                                                                   \
    if (VK_SUCCESS != (x)) {                                                             \
      SPDLOG_ERROR("Vulkan error: %s != VK_SUCCESS @ %s:%d.\n", #x, __FILE__, __LINE__); \
      abort();                                                                           \
    }                                                                                    \
  } while (0)