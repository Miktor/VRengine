#include "render_core.hpp"

#include <cstddef>
#include <memory>
#include <tuple>
#include <vector>

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

namespace {

constexpr int kMaxFramesInFlight = 2;

const std::vector<const char *> kValidationLayers = {"VK_LAYER_KHRONOS_validation"};

const std::vector<const char *> kDeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

#ifdef NDEBUG
constexpr bool kEnableValidationLayers = false;
#else
constexpr bool kEnableValidationLayers = true;
#endif

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

static uint32_t current_frame = 0;
VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                                             VkDebugUtilsMessageTypeFlagsEXT /*messageType*/,
                                             const VkDebugUtilsMessengerCallbackDataEXT *p_callback_data,
                                             void * /*pUserData*/) {
  spdlog::default_logger_raw()->log(GetSpdLogLevel(message_severity), "Frame: {}: {}", current_frame,
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
  CHECK_VK_SUCCESS(CreateDebugUtilsMessengerEXT(instance, &create_info, nullptr, &debug_messenger));

  return debug_messenger;
}

bool FindQueueFamilies(PhysicalDeviceContext &context) {
  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(context.device, &queue_family_count, nullptr);

  std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(context.device, &queue_family_count, queue_families.data());

  bool is_graphics_family_valid = false;
  bool is_present_family_valid = false;
  for (const auto &[i, queue_family] : Enumerate(queue_families)) {
    if ((queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0U) {
      context.indices.graphics_family = i;
      is_graphics_family_valid = true;
    }

    VkBool32 present_support = 0U;
    CHECK_VK_SUCCESS(
        vkGetPhysicalDeviceSurfaceSupportKHR(context.device, i, context.surface, &present_support));

    if (present_support != 0U) {
      context.indices.present_family = i;
      is_present_family_valid = true;
    }

    if (is_graphics_family_valid && is_present_family_valid) {
      break;
    }
  }

  return is_graphics_family_valid && is_present_family_valid;
}

bool CheckDeviceExtensionSupport(PhysicalDeviceContext &context) {
  uint32_t extension_count;
  vkEnumerateDeviceExtensionProperties(context.device, nullptr, &extension_count, nullptr);

  std::vector<VkExtensionProperties> available_extensions(extension_count);
  vkEnumerateDeviceExtensionProperties(context.device, nullptr, &extension_count,
                                       available_extensions.data());

  auto required_extensions = context.required_extensions;
  SPDLOG_INFO("Available device extension:");
  for (const auto &extension : available_extensions) {
    constexpr char VK_KHR_portability_subset_ext_name[] = "VK_KHR_portability_subset";

    if (memcmp(extension.extensionName, VK_KHR_portability_subset_ext_name,
               sizeof(VK_KHR_portability_subset_ext_name)) == 0) {
      context.required_extensions.insert(VK_KHR_portability_subset_ext_name);
    }

    SPDLOG_INFO("\t{}", extension.extensionName);
    required_extensions.erase(extension.extensionName);
  }

  return required_extensions.empty();
}

bool QuerySwapChainSupport(PhysicalDeviceContext &context) {
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context.device, context.surface, &context.surface_capabilities);

  uint32_t format_count;
  vkGetPhysicalDeviceSurfaceFormatsKHR(context.device, context.surface, &format_count, nullptr);

  if (format_count != 0) {
    context.surface_formats.resize(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(context.device, context.surface, &format_count,
                                         context.surface_formats.data());
  }

  uint32_t present_mode_count;
  vkGetPhysicalDeviceSurfacePresentModesKHR(context.device, context.surface, &present_mode_count, nullptr);

  if (present_mode_count != 0) {
    context.present_modes.resize(present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(context.device, context.surface, &present_mode_count,
                                              context.present_modes.data());
  }

  return !context.surface_formats.empty() && !context.present_modes.empty();
}

bool IsDeviceSuitable(PhysicalDeviceContext &context) {
  if (!FindQueueFamilies(context)) {
    return false;
  }

  if (!CheckDeviceExtensionSupport(context)) {
    return false;
  }

  return QuerySwapChainSupport(context);
}

VkSurfaceKHR CreateSurface(VkInstance instance, GLFWwindow *window) {
  VkSurfaceKHR surface;

  CHECK_VK_SUCCESS(glfwCreateWindowSurface(instance, window, nullptr, &surface));

  return surface;
}

PhysicalDeviceContext PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface) {
  uint32_t device_count = 0;
  vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

  if (device_count == 0) {
    throw std::runtime_error("failed to find GPUs with Vulkan support!");
  }

  std::vector<VkPhysicalDevice> devices(device_count);
  vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

  VkPhysicalDevice physical_device = VK_NULL_HANDLE;
  for (const auto &device : devices) {
    PhysicalDeviceContext context{device, surface};
    context.required_extensions.insert(kDeviceExtensions.begin(), kDeviceExtensions.end());
    if (IsDeviceSuitable(context)) {
      return context;
    }
  }

  throw std::runtime_error("failed to find a suitable GPU!");
}

VkDevice CreateLogicalDevice(const PhysicalDeviceContext &context) {
  std::vector<VkDeviceQueueCreateInfo> queue_create_infos;

  const std::set<uint32_t> unique_queue_families{context.indices.graphics_family,
                                                 context.indices.present_family};
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

  std::vector<const char *> enabled_extension_names;
  for (const auto &extension : context.required_extensions) {
    enabled_extension_names.push_back(extension.c_str());
  }
  create_info.enabledExtensionCount = static_cast<uint32_t>(enabled_extension_names.size());
  create_info.ppEnabledExtensionNames = enabled_extension_names.data();

  if (kEnableValidationLayers) {
    create_info.enabledLayerCount = static_cast<uint32_t>(kValidationLayers.size());
    create_info.ppEnabledLayerNames = kValidationLayers.data();
  } else {
    create_info.enabledLayerCount = 0;
  }

  VkDevice device;
  CHECK_VK_SUCCESS(vkCreateDevice(context.device, &create_info, nullptr, &device));

  return device;
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

VkCommandPool CreateCommandPool(VkDevice device, uint32_t queue_family_index) {
  VkCommandPoolCreateInfo pool_info{};
  pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool_info.queueFamilyIndex = queue_family_index;
  pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

  VkCommandPool command_pool;
  CHECK_VK_SUCCESS(vkCreateCommandPool(device, &pool_info, nullptr, &command_pool));

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

  CHECK_VK_SUCCESS(vkAllocateCommandBuffers(device, &alloc_info, command_buffers.data()));

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

RenderCore::RenderCore() {
}

void RenderCore::InitVulkan(GLFWwindow *window) {
  instance_ = CreateInstance();
  debug_messenger_ = SetupDebugMessenger(instance_);
  surface_ = CreateSurface(instance_, window);

  physical_device_ = PickPhysicalDevice(instance_, surface_);

  device_ = CreateLogicalDevice(physical_device_);

  InitVMA(vma_allocator_, instance_, physical_device_.device, device_);

  constexpr VkDeviceSize kMinUniformBufferOffsetAlignment = 0x100;
  constexpr VkDeviceSize kUniformBufferSize = kMinUniformBufferOffsetAlignment * 1024;
  ubo_allocator_ = std::make_unique<UniformBufferPoolAllocator>(
      *this, kUniformBufferSize, kMinUniformBufferOffsetAlignment, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

  vkGetDeviceQueue(device_, physical_device_.indices.graphics_family, 0, &graphics_queue_);
  vkGetDeviceQueue(device_, physical_device_.indices.present_family, 0, &present_queue_);

  CreateSwapChain(window);
  CreateImageViews();

  command_pool_ = CreateCommandPool(device_, physical_device_.indices.graphics_family);

  CreateSyncObjects();
  command_buffers_ = AllocateCommandBuffers(backbuffers_.size(), device_, command_pool_);

  InitPipelineCache();
}

void RenderCore::InitPipelineCache() {
  auto file_data = vre::platform::Platform::ReadFile("pipeline_cache.bin", false);

  VkPipelineCacheCreateInfo info = {VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
  info.initialDataSize = file_data.size();
  info.pInitialData = file_data.data();

  CHECK_VK_SUCCESS(vkCreatePipelineCache(device_, &info, nullptr, &pipeline_cache_));
}

void RenderCore::SaveAndDestroyPipelineCache() {
  size_t size = 0;
  CHECK_VK_SUCCESS(vkGetPipelineCacheData(device_, pipeline_cache_, &size, nullptr));

  std::vector<uint8_t> data;
  data.resize(size);
  CHECK_VK_SUCCESS(vkGetPipelineCacheData(device_, pipeline_cache_, &size, data.data()));

  vre::platform::Platform::WriteFile("pipeline_cache.bin", data.data(), size);
  vkDestroyPipelineCache(device_, pipeline_cache_, nullptr);
}

void RenderCore::CleanupSwapChain() {
  vkFreeCommandBuffers(device_, command_pool_, static_cast<uint32_t>(command_buffers_.size()),
                       command_buffers_.data());

  backbuffers_.clear();
  framebuffers_.clear();

  vkDestroySwapchainKHR(device_, swap_chain_, nullptr);
}

void RenderCore::Cleanup() {
  SaveAndDestroyPipelineCache();

  CleanupSwapChain();

  render_pass_.reset();
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

void RenderCore::CreateSwapChain(GLFWwindow *window) {
  const auto &capabilities = physical_device_.surface_capabilities;

  VkSurfaceFormatKHR surface_format = ChooseSwapSurfaceFormat(physical_device_.surface_formats);
  VkPresentModeKHR present_mode = ChooseSwapPresentMode(physical_device_.present_modes);
  VkExtent2D extent = ChooseSwapExtent(window, capabilities);

  uint32_t image_count = capabilities.minImageCount + 1;
  if (capabilities.maxImageCount > 0 && image_count > capabilities.maxImageCount) {
    image_count = capabilities.maxImageCount;
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

  uint32_t queue_family_indices[] = {physical_device_.indices.graphics_family,
                                     physical_device_.indices.present_family};

  if (physical_device_.indices.graphics_family != physical_device_.indices.present_family) {
    create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    create_info.queueFamilyIndexCount = 2;
    create_info.pQueueFamilyIndices = queue_family_indices;
  } else {
    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }

  create_info.preTransform = capabilities.currentTransform;
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

  RenderContext context{
      std::make_unique<CommandBuffer>(this, command_buffers_[next_image_index_], pipeline_cache_)};

  context.image_available_semaphore = image_available_semaphores_[current_frame_];
  context.render_finished_semaphore = render_finished_semaphores_[current_frame_];
  context.in_flight_fence = in_flight_fences_[current_frame_];
  context.images_in_flight = images_in_flight_[next_image_index_];

  context.command_buffer->Start();

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

  context.command_buffer->BeginRenderPass(begin_render_info);

  VkViewport viewport{};
  viewport.x = 0.0F;
  viewport.y = 0.0F;
  viewport.width = static_cast<float>(swap_chain_extent_.width);
  viewport.height = static_cast<float>(swap_chain_extent_.height);
  viewport.minDepth = 0.0F;
  viewport.maxDepth = 1.0F;
  context.command_buffer->SetViewport(viewport);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = swap_chain_extent_;
  context.command_buffer->SetScissors(scissor);

  return context;
}

void RenderCore::Present(RenderContext &context) {
  const auto cmd_buffer = context.command_buffer->GetBuffer();

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
  ++current_frame;
}

void RenderCore::WaitDeviceIdle() {
  vkDeviceWaitIdle(device_);
}

UniformBufferPoolAllocator &RenderCore::GetUniformBufferPoolAllocator() {
  return *ubo_allocator_;
}
}  // namespace vre::rendering
