#include "render_core.hpp"

#include <cstddef>
#include <memory>
#include <tuple>

#include <vulkan/vulkan_core.h>
#include "vk_mem_alloc.h"

#include "common.hpp"
#include "helpers.hpp"
#include "platform/platform.hpp"
#include "rendering/buffers.hpp"
#include "rendering/image.hpp"
#include "rendering/shader.hpp"
#include "rendering/uniform_buffer_allocator.hpp"

namespace vre::rendering {

struct QueueFamilyIndices {
  std::optional<uint32_t> graphics_family;
  std::optional<uint32_t> present_family;

  bool IsComplete() { return graphics_family.has_value() && present_family.has_value(); }
};

namespace {

constexpr int kMaxFramesInFlight = 2;

const std::vector<const char *> kValidationLayers = {"VK_LAYER_KHRONOS_validation"};

const std::vector<const char *> kDeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                                                     "VK_KHR_portability_subset"};

#ifdef NDEBUG
constexpr bool kEnableValidationLayers = false;
#else
constexpr bool kEnableValidationLayers = true;
#endif

struct SwapChainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities{};
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> present_modes;
};

bool CheckValidationLayerSupport() {
  uint32_t layer_count;
  vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

  std::vector<VkLayerProperties> available_layers(layer_count);
  vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

  for (const char *layer_name : kValidationLayers) {
    bool layer_found = false;

    for (const auto &layer_properties : available_layers) {
      if (strcmp(layer_name, layer_properties.layerName) == 0) {
        layer_found = true;
        break;
      }
    }

    if (!layer_found) {
      return false;
    }
  }

  return true;
}

std::vector<const char *> GetRequiredExtensions() {
  uint32_t glfw_extension_count = 0;
  const char **glfw_extensions;
  glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

  std::vector<const char *> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

  if (kEnableValidationLayers) {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

  return extensions;
}

spdlog::level::level_enum GetSpdLogLevel(VkDebugUtilsMessageSeverityFlagBitsEXT severity) {
  switch (severity) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
      return spdlog::level::trace;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
      return spdlog::level::info;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
      return spdlog::level::warn;
    default:
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
      return spdlog::level::err;
  }
};

VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                                             VkDebugUtilsMessageTypeFlagsEXT /*messageType*/,
                                             const VkDebugUtilsMessengerCallbackDataEXT *p_callback_data,
                                             void * /*pUserData*/) {
  SPDLOG_LOGGER_CALL(spdlog::default_logger_raw(), GetSpdLogLevel(message_severity), "validation layer: {}",
                     p_callback_data->pMessage);

  return VK_FALSE;
}

void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &create_info) {
  create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  create_info.pfnUserCallback = DebugCallback;
}

VkInstance CreateInstance() {
  if (kEnableValidationLayers && !CheckValidationLayerSupport()) {
    throw std::runtime_error("validation layers requested, but not available!");
  }

  VkApplicationInfo app_info{};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName = "VRengine";
  app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.pEngineName = "No Engine";
  app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.apiVersion = VK_API_VERSION_1_2;

  VkInstanceCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create_info.pApplicationInfo = &app_info;

  auto extensions = GetRequiredExtensions();
  create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  create_info.ppEnabledExtensionNames = extensions.data();

  VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};
  if (kEnableValidationLayers) {
    create_info.enabledLayerCount = static_cast<uint32_t>(kValidationLayers.size());
    create_info.ppEnabledLayerNames = kValidationLayers.data();

    PopulateDebugMessengerCreateInfo(debug_create_info);
    create_info.pNext = &debug_create_info;
  } else {
    create_info.enabledLayerCount = 0;

    create_info.pNext = nullptr;
  }

  VkInstance instance;
  CHECK_VK_SUCCESS(vkCreateInstance(&create_info, nullptr, &instance));

  return instance;
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
                                      const VkDebugUtilsMessengerCreateInfoEXT *p_create_info,
                                      const VkAllocationCallbacks *p_allocator,
                                      VkDebugUtilsMessengerEXT *p_debug_messenger) {
  auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
      vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
  if (func != nullptr) {
    return func(instance, p_create_info, p_allocator, p_debug_messenger);
  }
  return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger,
                                   const VkAllocationCallbacks *p_allocator) {
  auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
      vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
  if (func != nullptr) {
    func(instance, debug_messenger, p_allocator);
  }
}

VkDebugUtilsMessengerEXT SetupDebugMessenger(VkInstance instance) {
  if (!kEnableValidationLayers) {
    return VK_NULL_HANDLE;
  }

  VkDebugUtilsMessengerCreateInfoEXT create_info;
  PopulateDebugMessengerCreateInfo(create_info);

  VkDebugUtilsMessengerEXT debug_messenger;
  if (CreateDebugUtilsMessengerEXT(instance, &create_info, nullptr, &debug_messenger) != VK_SUCCESS) {
    throw std::runtime_error("failed to set up debug messenger!");
  }

  return debug_messenger;
}

QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
  QueueFamilyIndices indices;

  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

  std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

  int i = 0;
  for (const auto &queue_family : queue_families) {
    if ((queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0U) {
      indices.graphics_family = i;
    }

    VkBool32 present_support = 0U;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);

    if (present_support != 0U) {
      indices.present_family = i;
    }

    if (indices.IsComplete()) {
      break;
    }

    i++;
  }

  return indices;
}

bool CheckDeviceExtensionSupport(VkPhysicalDevice device) {
  uint32_t extension_count;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

  std::vector<VkExtensionProperties> available_extensions(extension_count);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

  std::set<std::string> required_extensions(kDeviceExtensions.begin(), kDeviceExtensions.end());

  SPDLOG_INFO("Available device extension:");
  for (const auto &extension : available_extensions) {
    SPDLOG_INFO("\t{}", extension.extensionName);
    required_extensions.erase(extension.extensionName);
  }

  return required_extensions.empty();
}

SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
  SwapChainSupportDetails details;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

  uint32_t format_count;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);

  if (format_count != 0) {
    details.formats.resize(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, details.formats.data());
  }

  uint32_t present_mode_count;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);

  if (present_mode_count != 0) {
    details.present_modes.resize(present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count,
                                              details.present_modes.data());
  }

  return details;
}

bool IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) {
  QueueFamilyIndices indices = FindQueueFamilies(device, surface);

  bool extensions_supported = CheckDeviceExtensionSupport(device);

  bool swap_chain_adequate = false;
  if (extensions_supported) {
    SwapChainSupportDetails swap_chain_support = QuerySwapChainSupport(device, surface);
    swap_chain_adequate = !swap_chain_support.formats.empty() && !swap_chain_support.present_modes.empty();
  }

  return indices.IsComplete() && extensions_supported && swap_chain_adequate;
}

VkSurfaceKHR CreateSurface(VkInstance instance, GLFWwindow *window) {
  VkSurfaceKHR surface;

  CHECK_VK_SUCCESS(glfwCreateWindowSurface(instance, window, nullptr, &surface));

  return surface;
}

VkPhysicalDevice PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface) {
  uint32_t device_count = 0;
  vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

  if (device_count == 0) {
    throw std::runtime_error("failed to find GPUs with Vulkan support!");
  }

  std::vector<VkPhysicalDevice> devices(device_count);
  vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

  VkPhysicalDevice physical_device;
  for (const auto &device : devices) {
    if (IsDeviceSuitable(device, surface)) {
      physical_device = device;
      break;
    }
  }

  if (physical_device == VK_NULL_HANDLE) {
    throw std::runtime_error("failed to find a suitable GPU!");
  }

  return physical_device;
}

auto CreateLogicalDevice(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
  QueueFamilyIndices indices = FindQueueFamilies(physical_device, surface);

  std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
  std::set<uint32_t> unique_queue_families = {indices.graphics_family.value(),
                                              indices.present_family.value()};

  float queue_priority = 1.0F;
  for (uint32_t queue_family : unique_queue_families) {
    VkDeviceQueueCreateInfo queue_create_info{};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = queue_family;
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = &queue_priority;
    queue_create_infos.push_back(queue_create_info);
  }

  VkPhysicalDeviceFeatures device_features{};

  VkDeviceCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

  create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
  create_info.pQueueCreateInfos = queue_create_infos.data();

  create_info.pEnabledFeatures = &device_features;

  create_info.enabledExtensionCount = static_cast<uint32_t>(kDeviceExtensions.size());
  create_info.ppEnabledExtensionNames = kDeviceExtensions.data();

  if (kEnableValidationLayers) {
    create_info.enabledLayerCount = static_cast<uint32_t>(kValidationLayers.size());
    create_info.ppEnabledLayerNames = kValidationLayers.data();
  } else {
    create_info.enabledLayerCount = 0;
  }

  VkDevice device;
  if (vkCreateDevice(physical_device, &create_info, nullptr, &device) != VK_SUCCESS) {
    throw std::runtime_error("failed to create logical device_!");
  }

  return std::make_tuple(device, indices);
}

VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &available_formats) {
  for (const auto &available_format : available_formats) {
    if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB &&
        available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return available_format;
    }
  }

  return available_formats[0];
}

VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &available_present_modes) {
  for (const auto &available_present_mode : available_present_modes) {
    if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
      return available_present_mode;
    }
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D ChooseSwapExtent(GLFWwindow *window, const VkSurfaceCapabilitiesKHR &capabilities) {
  if (capabilities.currentExtent.width != UINT32_MAX) {
    return capabilities.currentExtent;
  }

  int width;
  int height;
  glfwGetFramebufferSize(window, &width, &height);

  VkExtent2D actual_extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

  actual_extent.width =
      std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
  actual_extent.height = std::clamp(actual_extent.height, capabilities.minImageExtent.height,
                                    capabilities.maxImageExtent.height);

  return actual_extent;
}

VkCommandPool CreateCommandPool(VkDevice device, const QueueFamilyIndices &queue_family_indices) {
  VkCommandPoolCreateInfo pool_info{};
  pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool_info.queueFamilyIndex = queue_family_indices.graphics_family.value();
  pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

  VkCommandPool command_pool;
  if (vkCreateCommandPool(device, &pool_info, nullptr, &command_pool) != VK_SUCCESS) {
    throw std::runtime_error("failed to create command pool!");
  }

  return command_pool;
}

std::vector<VkCommandBuffer> AllocateCommandBuffers(uint8_t count, VkDevice device,
                                                    VkCommandPool command_pool) {
  std::vector<VkCommandBuffer> command_buffers(count);

  VkCommandBufferAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.commandPool = command_pool;
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandBufferCount = static_cast<uint32_t>(command_buffers.size());

  if (vkAllocateCommandBuffers(device, &alloc_info, command_buffers.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate command buffers!");
  }

  return command_buffers;
}

RenderPassInfo CreateDefaultRenderPass(ImageViewPtr image_view) {
  RenderPassInfo info;
  info.color_attachments.push_back(image_view);
  info.clear_attachments = 1U << 0;
  info.store_attachments = 1U << 0;

  info.clear_color.push_back({.0F, .0F, .0F});

  RenderPassInfo::Subpass subpass_info{};
  subpass_info.color_attachments.reserve(info.color_attachments.size());
  for (unsigned i = 0; i < info.color_attachments.size(); i++) {
    subpass_info.color_attachments.push_back(i);
  }
  info.subpasses.push_back(std::move(subpass_info));

  return info;
}

void InitVMA(VmaAllocator &allocator, VkInstance instance, VkPhysicalDevice physical_device,
             VkDevice device) {
  VmaAllocatorCreateInfo allocator_create_info{};
  allocator_create_info.vulkanApiVersion = VK_API_VERSION_1_2;
  allocator_create_info.physicalDevice = physical_device;
  allocator_create_info.device = device;
  allocator_create_info.instance = instance;

  vmaCreateAllocator(&allocator_create_info, &allocator);
}

}  // namespace

RenderCore::RenderCore() {}

void RenderCore::InitVulkan(GLFWwindow *window) {
  instance_ = CreateInstance();
  debug_messenger_ = SetupDebugMessenger(instance_);
  surface_ = CreateSurface(instance_, window);
  physical_device_ = PickPhysicalDevice(instance_, surface_);

  const auto &[device, indices] = CreateLogicalDevice(physical_device_, surface_);
  device_ = device;

  InitVMA(vma_allocator_, instance_, physical_device_, device_);

  ubo_allocator_ =
      std::make_unique<UniformBufferPoolAllocator>(*this, 256 * 1024, 16, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

  vkGetDeviceQueue(device_, indices.graphics_family.value(), 0, &graphics_queue_);
  vkGetDeviceQueue(device_, indices.present_family.value(), 0, &present_queue_);

  CreateSwapChain(window, indices);
  CreateImageViews();

  command_pool_ = CreateCommandPool(device, indices);

  CreateSyncObjects();
  command_buffers_ = AllocateCommandBuffers(backbuffers_.size(), device, command_pool_);
}

void RenderCore::CleanupSwapChain() {
  vkFreeCommandBuffers(device_, command_pool_, static_cast<uint32_t>(command_buffers_.size()),
                       command_buffers_.data());

  vkDestroySwapchainKHR(device_, swap_chain_, nullptr);
}

void RenderCore::Cleanup() {
  CleanupSwapChain();

  ubo_allocator_.reset();

  for (size_t i = 0; i < kMaxFramesInFlight; i++) {
    vkDestroySemaphore(device_, render_finished_semaphores_[i], nullptr);
    vkDestroySemaphore(device_, image_available_semaphores_[i], nullptr);
    vkDestroyFence(device_, in_flight_fences_[i], nullptr);
  }

  vkDestroyCommandPool(device_, command_pool_, nullptr);

  vmaDestroyAllocator(vma_allocator_);
  vkDestroyDevice(device_, nullptr);

  if (kEnableValidationLayers) {
    DestroyDebugUtilsMessengerEXT(instance_, debug_messenger_, nullptr);
  }

  vkDestroySurfaceKHR(instance_, surface_, nullptr);
  vkDestroyInstance(instance_, nullptr);
}

void RenderCore::CreateSwapChain(GLFWwindow *window, const QueueFamilyIndices &indices) {
  SwapChainSupportDetails swap_chain_support = QuerySwapChainSupport(physical_device_, surface_);

  VkSurfaceFormatKHR surface_format = ChooseSwapSurfaceFormat(swap_chain_support.formats);
  VkPresentModeKHR present_mode = ChooseSwapPresentMode(swap_chain_support.present_modes);
  VkExtent2D extent = ChooseSwapExtent(window, swap_chain_support.capabilities);

  uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;
  if (swap_chain_support.capabilities.maxImageCount > 0 &&
      image_count > swap_chain_support.capabilities.maxImageCount) {
    image_count = swap_chain_support.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  create_info.surface = surface_;

  create_info.minImageCount = image_count;
  create_info.imageFormat = surface_format.format;
  create_info.imageColorSpace = surface_format.colorSpace;
  create_info.imageExtent = extent;
  create_info.imageArrayLayers = 1;
  create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  uint32_t queue_family_indices[] = {indices.graphics_family.value(), indices.present_family.value()};

  if (indices.graphics_family != indices.present_family) {
    create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    create_info.queueFamilyIndexCount = 2;
    create_info.pQueueFamilyIndices = queue_family_indices;
  } else {
    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }

  create_info.preTransform = swap_chain_support.capabilities.currentTransform;
  create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  create_info.presentMode = present_mode;
  create_info.clipped = VK_TRUE;

  create_info.oldSwapchain = VK_NULL_HANDLE;

  if (vkCreateSwapchainKHR(device_, &create_info, nullptr, &swap_chain_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create swap chain!");
  }

  vkGetSwapchainImagesKHR(device_, swap_chain_, &image_count, nullptr);
  swap_chain_images_.resize(image_count);
  vkGetSwapchainImagesKHR(device_, swap_chain_, &image_count, swap_chain_images_.data());

  swap_chain_image_format_ = surface_format.format;
  swap_chain_extent_ = extent;
}

void RenderCore::CreateImageViews() {
  const auto image_create_info = ImageCreateInfo::RenderTarget(
      swap_chain_extent_.width, swap_chain_extent_.height, swap_chain_image_format_);
  backbuffers_.reserve(swap_chain_images_.size());

  for (auto &swap_chain_image : swap_chain_images_) {
    VkImageViewCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.image = swap_chain_image;
    create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    create_info.format = swap_chain_image_format_;
    create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    create_info.subresourceRange.baseMipLevel = 0;
    create_info.subresourceRange.levelCount = 1;
    create_info.subresourceRange.baseArrayLayer = 0;
    create_info.subresourceRange.layerCount = 1;

    VkImageView image_view;
    if (vkCreateImageView(device_, &create_info, nullptr, &image_view) != VK_SUCCESS) {
      throw std::runtime_error("failed to create image views!");
    }

    backbuffers_.emplace_back(device_, swap_chain_image, image_view, image_create_info,
                              VK_IMAGE_VIEW_TYPE_2D);
    backbuffers_.back().SetSwapchainLayout(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
  }

  framebuffers_.resize(backbuffers_.size());
}

void RenderCore::CreateSyncObjects() {
  image_available_semaphores_.resize(kMaxFramesInFlight);
  render_finished_semaphores_.resize(kMaxFramesInFlight);
  in_flight_fences_.resize(kMaxFramesInFlight);
  images_in_flight_.resize(swap_chain_images_.size(), VK_NULL_HANDLE);

  VkSemaphoreCreateInfo semaphore_info{};
  semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fence_info{};
  fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < kMaxFramesInFlight; i++) {
    if (vkCreateSemaphore(device_, &semaphore_info, nullptr, &image_available_semaphores_[i]) != VK_SUCCESS ||
        vkCreateSemaphore(device_, &semaphore_info, nullptr, &render_finished_semaphores_[i]) != VK_SUCCESS ||
        vkCreateFence(device_, &fence_info, nullptr, &in_flight_fences_[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to create synchronization objects for a frame!");
    }
  }
}

RenderContext RenderCore::BeginDraw() {
  vkWaitForFences(device_, 1, &in_flight_fences_[current_frame_], VK_TRUE, UINT64_MAX);

  vkAcquireNextImageKHR(device_, swap_chain_, UINT64_MAX, image_available_semaphores_[current_frame_],
                        VK_NULL_HANDLE, &next_image_index_);

  if (images_in_flight_[next_image_index_] != VK_NULL_HANDLE) {
    vkWaitForFences(device_, 1, &images_in_flight_[next_image_index_], VK_TRUE, UINT64_MAX);
  }

  images_in_flight_[next_image_index_] = in_flight_fences_[current_frame_];

  RenderContext context{CommandBuffer(this, command_buffers_[next_image_index_])};

  context.image_available_semaphore = image_available_semaphores_[current_frame_];
  context.render_finished_semaphore = render_finished_semaphores_[current_frame_];
  context.in_flight_fence = in_flight_fences_[current_frame_];
  context.images_in_flight = images_in_flight_[next_image_index_];

  context.command_buffer.Start();

  BeginRenderInfo begin_render_info{};
  begin_render_info.render_pass_info = CreateDefaultRenderPass(backbuffers_[next_image_index_].GetView());

  if (render_pass_ == nullptr) {
    render_pass_ = std::make_shared<RenderPass>(device_, begin_render_info.render_pass_info);
  }
  
  begin_render_info.render_pass = render_pass_;

  if (framebuffers_[next_image_index_] == nullptr) {
    framebuffers_[next_image_index_] = std::make_shared<Framebuffer>(device_, *begin_render_info.render_pass,
                                                                     begin_render_info.render_pass_info);
  }
  begin_render_info.framebuffer = framebuffers_[next_image_index_];

  context.command_buffer.BeginRenderPass(begin_render_info);

  VkViewport viewport{};
  viewport.x = 0.0F;
  viewport.y = 0.0F;
  viewport.width = static_cast<float>(swap_chain_extent_.width);
  viewport.height = static_cast<float>(swap_chain_extent_.height);
  viewport.minDepth = 0.0F;
  viewport.maxDepth = 1.0F;
  context.command_buffer.SetViewport(viewport);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = swap_chain_extent_;
  context.command_buffer.SetScissors(scissor);

  return context;
}

void RenderCore::Present(RenderContext &context) {
  const auto cmd_buffer = context.command_buffer.GetBuffer();

  vkCmdEndRenderPass(cmd_buffer);

  if (vkEndCommandBuffer(cmd_buffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to record command buffer!");
  }

  VkSubmitInfo submit_info{};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore wait_semaphores[] = {context.image_available_semaphore};
  VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores = wait_semaphores;
  submit_info.pWaitDstStageMask = wait_stages;

  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &cmd_buffer;

  VkSemaphore signal_semaphores[] = {context.render_finished_semaphore};
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores = signal_semaphores;

  vkResetFences(device_, 1, &context.in_flight_fence);

  if (vkQueueSubmit(graphics_queue_, 1, &submit_info, context.in_flight_fence) != VK_SUCCESS) {
    throw std::runtime_error("failed to submit draw command buffer!");
  }

  VkPresentInfoKHR present_info{};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = signal_semaphores;

  VkSwapchainKHR swap_chains[] = {swap_chain_};
  present_info.swapchainCount = 1;
  present_info.pSwapchains = swap_chains;

  present_info.pImageIndices = &next_image_index_;

  vkQueuePresentKHR(present_queue_, &present_info);

  current_frame_ = (current_frame_ + 1) % kMaxFramesInFlight;
}

void RenderCore::WaitDeviceIdle() { vkDeviceWaitIdle(device_); }

UniformBufferPoolAllocator &RenderCore::GetUniformBufferPoolAllocator() { return *ubo_allocator_; }
}  // namespace vre::rendering