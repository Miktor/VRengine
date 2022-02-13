#pragma once

#include <string>

#include <spdlog/spdlog.h>
#include <vulkan/vulkan.h>

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

#define CHECK_VK_SUCCESS(x)                                              \
  do {                                                                   \
    VkResult res = (x);                                                  \
    if (VK_SUCCESS != res) {                                             \
      SPDLOG_ERROR("Vulkan error: {} == {}", #x, VkResultToString(res)); \
      abort();                                                           \
    }                                                                    \
  } while (0)

std::string VkResultToString(VkResult result);