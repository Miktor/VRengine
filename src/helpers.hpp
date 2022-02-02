#pragma once

#ifdef VR_DEBUG
#define VR_ASSERT(x)                                        \
  do {                                                      \
    if (!bool(x)) {                                         \
      LOGE("Vulkan error at %s:%d.\n", __FILE__, __LINE__); \
      abort();                                              \
    }                                                       \
  } while (0)
#else
#define VR_ASSERT(x) ((void)0)
#endif

#define CHECK_VK_SUCCESS(x)                                         \
  do {                                                              \
    if (VK_SUCCESS != (x)) {                                        \
      SPDLOG_ERROR("Vulkan error at %s:%d.\n", __FILE__, __LINE__); \
      abort();                                                      \
    }                                                               \
  } while (0)