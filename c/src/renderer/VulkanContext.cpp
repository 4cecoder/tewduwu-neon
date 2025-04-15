#include "VulkanContext.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <set>
#include <vector>

namespace tewduwu {

// Debug messenger callback
static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
              void* pUserData)
{
  if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;
  }

  return VK_FALSE;
}

// Helper function to create debug messenger
VkResult CreateDebugUtilsMessengerEXT(
  VkInstance instance,
  const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
  const VkAllocationCallbacks* pAllocator,
  VkDebugUtilsMessengerEXT* pDebugMessenger)
{
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
    instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) {
    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
  }
  else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

// Helper function to destroy debug messenger
void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                   VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks* pAllocator)
{
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
    instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) {
    func(instance, debugMessenger, pAllocator);
  }
}

VulkanContext::VulkanContext()
  : instance(VK_NULL_HANDLE)
  , debugMessenger(VK_NULL_HANDLE)
  , surface(VK_NULL_HANDLE)
  , physicalDevice(VK_NULL_HANDLE)
  , device(VK_NULL_HANDLE)
  , graphicsQueue(VK_NULL_HANDLE)
  , presentQueue(VK_NULL_HANDLE)
  , swapChain(VK_NULL_HANDLE)
  , renderPass(VK_NULL_HANDLE)
  , commandPool(VK_NULL_HANDLE)
  , currentFrame(0)
  , framebufferResized(false)
{ }

VulkanContext::~VulkanContext()
{
  cleanup();
}

bool VulkanContext::initialize(SDL_Window* window)
{
  createInstance();
  setupDebugMessenger();
  createSurface(window);
  pickPhysicalDevice();
  createLogicalDevice();
  createSwapChain(window);
  createRenderPass();
  createFramebuffers();
  createCommandPool();
  createCommandBuffers();
  createSyncObjects();

  return true;
}

void VulkanContext::cleanup()
{
  // Wait for all operations to complete
  vkDeviceWaitIdle(device);

  // Clean up sync objects
  for (size_t i = 0; i < imageAvailableSemaphores.size(); i++) {
    vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
    vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
    vkDestroyFence(device, inFlightFences[i], nullptr);
  }

  // Clean up command pool
  vkDestroyCommandPool(device, commandPool, nullptr);

  // Clean up framebuffers
  for (auto framebuffer : swapChainFramebuffers) {
    vkDestroyFramebuffer(device, framebuffer, nullptr);
  }

  // Clean up render pass
  vkDestroyRenderPass(device, renderPass, nullptr);

  // Clean up image views
  for (auto imageView : swapChainImageViews) {
    vkDestroyImageView(device, imageView, nullptr);
  }

  // Clean up swap chain
  vkDestroySwapchainKHR(device, swapChain, nullptr);

  // Clean up device
  vkDestroyDevice(device, nullptr);

  // Clean up debug messenger
  if (debugMessenger != VK_NULL_HANDLE) {
    DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
  }

  // Clean up surface
  vkDestroySurfaceKHR(instance, surface, nullptr);

  // Clean up instance
  vkDestroyInstance(instance, nullptr);
}

void VulkanContext::createInstance()
{
  // Application info
  VkApplicationInfo appInfo {};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "tewduwu-neon";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "No Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_0;

  // Instance create info
  VkInstanceCreateInfo createInfo {};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;

  // Get required extensions
  unsigned int sdlExtensionCount = 0;
  if (!SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensionCount, nullptr)) {
    throw std::runtime_error(
      "Failed to get required Vulkan extensions count from SDL: " +
      std::string(SDL_GetError()));
  }

  std::vector<const char*> extensions(sdlExtensionCount);
  if (!SDL_Vulkan_GetInstanceExtensions(
        window, &sdlExtensionCount, extensions.data())) {
    throw std::runtime_error(
      "Failed to get required Vulkan extensions from SDL: " +
      std::string(SDL_GetError()));
  }

  // Add debug extension
  extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

  createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  // Validation layers
  const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
  };

  createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
  createInfo.ppEnabledLayerNames = validationLayers.data();

  // Create the instance
  if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create Vulkan instance");
  }
}

void VulkanContext::setupDebugMessenger()
{
  VkDebugUtilsMessengerCreateInfoEXT createInfo {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo.pfnUserCallback = debugCallback;
  createInfo.pUserData = nullptr;

  if (CreateDebugUtilsMessengerEXT(
        instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
    throw std::runtime_error("Failed to set up debug messenger");
  }
}

void VulkanContext::createSurface(SDL_Window* window)
{
  if (!SDL_Vulkan_CreateSurface(window, instance, &surface)) {
    throw std::runtime_error("Failed to create Vulkan surface: " +
                             std::string(SDL_GetError()));
  }
}

void VulkanContext::pickPhysicalDevice()
{
  // Get all physical devices
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

  if (deviceCount == 0) {
    throw std::runtime_error("Failed to find GPUs with Vulkan support");
  }

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

  // Find a suitable device
  for (const auto& device : devices) {
    // Check device features and properties
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    // Check queue families
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(
      device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(
      device, &queueFamilyCount, queueFamilies.data());

    bool hasGraphicsQueue = false;
    bool hasPresentQueue = false;

    for (uint32_t i = 0; i < queueFamilyCount; i++) {
      if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        hasGraphicsQueue = true;
      }

      VkBool32 presentSupport = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

      if (presentSupport) {
        hasPresentQueue = true;
      }
    }

    // Check device extensions
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(
      device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(
      device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    for (const auto& extension : availableExtensions) {
      requiredExtensions.erase(extension.extensionName);
    }

    // If this device meets all requirements, use it
    if (hasGraphicsQueue && hasPresentQueue && requiredExtensions.empty() &&
        deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
      physicalDevice = device;
      break;
    }
  }

  // If no discrete GPU was found, pick the first available device
  if (physicalDevice == VK_NULL_HANDLE && !devices.empty()) {
    physicalDevice = devices[0];
  }

  if (physicalDevice == VK_NULL_HANDLE) {
    throw std::runtime_error("Failed to find a suitable GPU");
  }
}

void VulkanContext::createLogicalDevice()
{
  // Find queue families
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(
    physicalDevice, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(
    physicalDevice, &queueFamilyCount, queueFamilies.data());

  uint32_t graphicsFamily = UINT32_MAX;
  uint32_t presentFamily = UINT32_MAX;

  for (uint32_t i = 0; i < queueFamilyCount; i++) {
    if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      graphicsFamily = i;
    }

    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(
      physicalDevice, i, surface, &presentSupport);

    if (presentSupport) {
      presentFamily = i;
    }

    if (graphicsFamily != UINT32_MAX && presentFamily != UINT32_MAX) {
      break;
    }
  }

  // Create a set of unique queue families needed
  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueFamilies = { graphicsFamily, presentFamily };

  float queuePriority = 1.0f;
  for (uint32_t queueFamily : uniqueQueueFamilies) {
    VkDeviceQueueCreateInfo queueCreateInfo {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(queueCreateInfo);
  }

  // Specify device features
  VkPhysicalDeviceFeatures deviceFeatures {};

  // Required extensions
  std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
  };

  // Create the logical device
  VkDeviceCreateInfo createInfo {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.queueCreateInfoCount =
    static_cast<uint32_t>(queueCreateInfos.size());
  createInfo.pQueueCreateInfos = queueCreateInfos.data();
  createInfo.pEnabledFeatures = &deviceFeatures;
  createInfo.enabledExtensionCount =
    static_cast<uint32_t>(deviceExtensions.size());
  createInfo.ppEnabledExtensionNames = deviceExtensions.data();

  if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create logical device");
  }

  // Get queue handles
  vkGetDeviceQueue(device, graphicsFamily, 0, &graphicsQueue);
  vkGetDeviceQueue(device, presentFamily, 0, &presentQueue);
}

void VulkanContext::createSwapChain(SDL_Window* window)
{
  // Query surface capabilities
  VkSurfaceCapabilitiesKHR capabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
    physicalDevice, surface, &capabilities);

  // Query supported surface formats
  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(
    physicalDevice, surface, &formatCount, nullptr);

  std::vector<VkSurfaceFormatKHR> formats;
  if (formatCount != 0) {
    formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(
      physicalDevice, surface, &formatCount, formats.data());
  }

  // Query supported presentation modes
  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(
    physicalDevice, surface, &presentModeCount, nullptr);

  std::vector<VkPresentModeKHR> presentModes;
  if (presentModeCount != 0) {
    presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
      physicalDevice, surface, &presentModeCount, presentModes.data());
  }

  // Choose swap surface format
  VkSurfaceFormatKHR surfaceFormat = formats[0];
  for (const auto& format : formats) {
    if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
        format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      surfaceFormat = format;
      break;
    }
  }

  // Choose swap present mode (prefer mailbox for triple buffering if available)
  VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
  for (const auto& mode : presentModes) {
    if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
      presentMode = mode;
      break;
    }
  }

  // Choose swap extent
  VkExtent2D extent;
  if (capabilities.currentExtent.width != UINT32_MAX) {
    extent = capabilities.currentExtent;
  }
  else {
    int width, height;
    SDL_GetWindowSizeInPixels(window, &width, &height);

    extent.width = std::clamp(static_cast<uint32_t>(width),
                              capabilities.minImageExtent.width,
                              capabilities.maxImageExtent.width);
    extent.height = std::clamp(static_cast<uint32_t>(height),
                               capabilities.minImageExtent.height,
                               capabilities.maxImageExtent.height);
  }

  // Choose image count (triple buffering if possible)
  uint32_t imageCount = capabilities.minImageCount + 1;
  if (capabilities.maxImageCount > 0 &&
      imageCount > capabilities.maxImageCount) {
    imageCount = capabilities.maxImageCount;
  }

  // Create the swap chain
  VkSwapchainCreateInfoKHR createInfo {};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = surface;
  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = extent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  // Find queue families
  uint32_t queueFamilyIndices[2] = { 0, 0 };
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(
    physicalDevice, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(
    physicalDevice, &queueFamilyCount, queueFamilies.data());

  for (uint32_t i = 0; i < queueFamilyCount; i++) {
    if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      queueFamilyIndices[0] = i;
    }

    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(
      physicalDevice, i, surface, &presentSupport);

    if (presentSupport) {
      queueFamilyIndices[1] = i;
    }
  }

  if (queueFamilyIndices[0] != queueFamilyIndices[1]) {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  }
  else {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }

  createInfo.preTransform = capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;
  createInfo.oldSwapchain = VK_NULL_HANDLE;

  if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create swap chain");
  }

  // Retrieve swap chain images
  vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
  swapChainImages.resize(imageCount);
  vkGetSwapchainImagesKHR(
    device, swapChain, &imageCount, swapChainImages.data());

  // Store format and extent
  swapChainImageFormat = surfaceFormat.format;
  swapChainExtent = extent;

  // Create image views
  swapChainImageViews.resize(swapChainImages.size());
  for (size_t i = 0; i < swapChainImages.size(); i++) {
    VkImageViewCreateInfo viewInfo {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = swapChainImages[i];
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = swapChainImageFormat;
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(
          device, &viewInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create image views");
    }
  }
}

void VulkanContext::createRenderPass()
{
  // Color attachment
  VkAttachmentDescription colorAttachment {};
  colorAttachment.format = swapChainImageFormat;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  // Color attachment reference
  VkAttachmentReference colorAttachmentRef {};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  // Subpass
  VkSubpassDescription subpass {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;

  // Subpass dependency
  VkSubpassDependency dependency {};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  // Create render pass
  VkRenderPassCreateInfo renderPassInfo {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &colorAttachment;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create render pass");
  }
}

void VulkanContext::createFramebuffers()
{
  swapChainFramebuffers.resize(swapChainImageViews.size());

  for (size_t i = 0; i < swapChainImageViews.size(); i++) {
    VkImageView attachments[] = { swapChainImageViews[i] };

    VkFramebufferCreateInfo framebufferInfo {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = swapChainExtent.width;
    framebufferInfo.height = swapChainExtent.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(
          device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) !=
        VK_SUCCESS) {
      throw std::runtime_error("Failed to create framebuffer");
    }
  }
}

void VulkanContext::createCommandPool()
{
  // Find queue family indices
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(
    physicalDevice, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(
    physicalDevice, &queueFamilyCount, queueFamilies.data());

  uint32_t graphicsFamily = 0;
  for (uint32_t i = 0; i < queueFamilyCount; i++) {
    if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      graphicsFamily = i;
      break;
    }
  }

  // Create command pool
  VkCommandPoolCreateInfo poolInfo {};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.queueFamilyIndex = graphicsFamily;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

  if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create command pool");
  }
}

void VulkanContext::createCommandBuffers()
{
  // Create command buffers (one for each frame in flight)
  commandBuffers.resize(swapChainFramebuffers.size());

  VkCommandBufferAllocateInfo allocInfo {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

  if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate command buffers");
  }
}

void VulkanContext::createSyncObjects()
{
  // Create synchronization objects for each frame in flight
  imageAvailableSemaphores.resize(swapChainFramebuffers.size());
  renderFinishedSemaphores.resize(swapChainFramebuffers.size());
  inFlightFences.resize(swapChainFramebuffers.size());

  VkSemaphoreCreateInfo semaphoreInfo {};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo {};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < swapChainFramebuffers.size(); i++) {
    if (vkCreateSemaphore(
          device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) !=
          VK_SUCCESS ||
        vkCreateSemaphore(
          device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) !=
          VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) !=
          VK_SUCCESS) {
      throw std::runtime_error("Failed to create synchronization objects");
    }
  }
}

void VulkanContext::beginFrame()
{
  // Wait for the previous frame to finish
  vkWaitForFences(
    device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

  // Reset the fence for the current frame
  vkResetFences(device, 1, &inFlightFences[currentFrame]);

  // Acquire an image from the swap chain
  uint32_t imageIndex;
  VkResult result =
    vkAcquireNextImageKHR(device,
                          swapChain,
                          1000000000,
                          imageAvailableSemaphores[currentFrame],
                          VK_NULL_HANDLE,
                          &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapChain(window);
    return;
  }
  else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("Failed to acquire swap chain image");
  }

  currentImageIndex = imageIndex;

  // Reset the command buffer
  vkResetCommandBuffer(commandBuffers[currentImageIndex], 0);

  // Begin command buffer recording
  VkCommandBufferBeginInfo beginInfo {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffers[currentImageIndex], &beginInfo);

  // Begin render pass
  VkRenderPassBeginInfo renderPassInfo {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = renderPass;
  renderPassInfo.framebuffer = swapChainFramebuffers[currentImageIndex];
  renderPassInfo.renderArea.offset = { 0, 0 };
  renderPassInfo.renderArea.extent = swapChainExtent;

  // Change clear color to dark blue for testing visibility
  VkClearValue clearColor = { { { 0.0f, 0.0f, 0.2f, 1.0f } } };
  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues = &clearColor;

  vkCmdBeginRenderPass(commandBuffers[currentImageIndex],
                       &renderPassInfo,
                       VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanContext::endFrame()
{
  // End render pass
  vkCmdEndRenderPass(commandBuffers[currentImageIndex]);

  // End command buffer recording
  if (vkEndCommandBuffer(commandBuffers[currentImageIndex]) != VK_SUCCESS) {
    throw std::runtime_error("Failed to record command buffer");
  }

  // Submit command buffer
  VkSubmitInfo submitInfo {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
  VkPipelineStageFlags waitStages[] = {
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
  };
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffers[currentImageIndex];

  VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  if (vkQueueSubmit(
        graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to submit draw command buffer");
  }

  // Present the image
  VkPresentInfoKHR presentInfo {};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;

  VkSwapchainKHR swapChains[] = { swapChain };
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;
  presentInfo.pImageIndices = &currentImageIndex;

  VkResult result = vkQueuePresentKHR(presentQueue, &presentInfo);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
      framebufferResized) {
    framebufferResized = false;
    recreateSwapChain(window);
  }
  else if (result != VK_SUCCESS) {
    throw std::runtime_error("Failed to present swap chain image");
  }

  currentFrame = (currentFrame + 1) % inFlightFences.size();
}

void VulkanContext::waitIdle()
{
  vkDeviceWaitIdle(device);
}

void VulkanContext::recreateSwapChain(SDL_Window* window)
{
  // Wait for operations to complete
  vkDeviceWaitIdle(device);

  // Clean up existing swap chain resources
  cleanupSwapChain();

  // Recreate swap chain and dependent resources
  createSwapChain(window);
  createFramebuffers();
}

void VulkanContext::cleanupSwapChain()
{
  // Destroy framebuffers
  for (auto framebuffer : swapChainFramebuffers) {
    vkDestroyFramebuffer(device, framebuffer, nullptr);
  }
  swapChainFramebuffers.clear();

  // Destroy image views
  for (auto imageView : swapChainImageViews) {
    vkDestroyImageView(device, imageView, nullptr);
  }
  swapChainImageViews.clear();

  // Destroy swap chain
  vkDestroySwapchainKHR(device, swapChain, nullptr);
  swapChain = VK_NULL_HANDLE;
}

VkDevice VulkanContext::getDevice() const
{
  return device;
}

VkPhysicalDevice VulkanContext::getPhysicalDevice() const
{
  return physicalDevice;
}

VkCommandPool VulkanContext::getCommandPool() const
{
  return commandPool;
}

VkRenderPass VulkanContext::getRenderPass() const
{
  return renderPass;
}

VkExtent2D VulkanContext::getSwapChainExtent() const
{
  return swapChainExtent;
}

VkCommandBuffer VulkanContext::getCurrentCommandBuffer() const
{
  // Check if commandBuffers is large enough for currentImageIndex
  if (currentImageIndex >= commandBuffers.size()) {
    // Handle error: Index out of bounds. This shouldn't happen in a correct setup.
    // You might want to throw an exception or return VK_NULL_HANDLE after logging an error.
    // For now, let's throw an exception for clarity.
    throw std::runtime_error(
      "Error: currentImageIndex is out of bounds for commandBuffers.");
  }
  return commandBuffers[currentImageIndex];
}

VkDescriptorPool VulkanContext::getDescriptorPool() const
{
  return descriptorPool;
}

uint32_t VulkanContext::findMemoryType(uint32_t typeFilter,
                                       VkMemoryPropertyFlags properties)
{
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags &
                                    properties) == properties) {
      return i;
    }
  }

  throw std::runtime_error("Failed to find suitable memory type!");
}

void VulkanContext::createBuffer(VkDeviceSize size,
                                 VkBufferUsageFlags usage,
                                 VkMemoryPropertyFlags properties,
                                 VkBuffer& buffer,
                                 VkDeviceMemory& bufferMemory)
{
  VkBufferCreateInfo bufferInfo {};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create buffer!");
  }

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo {};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex =
    findMemoryType(memRequirements.memoryTypeBits, properties);

  if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate buffer memory!");
  }

  vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

VkCommandBuffer VulkanContext::beginSingleTimeCommands()
{
  VkCommandBufferAllocateInfo allocInfo {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = commandPool;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  return commandBuffer;
}

void VulkanContext::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(graphicsQueue);

  vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void VulkanContext::transitionImageLayout(VkImage image,
                                          VkFormat format,
                                          VkImageLayout oldLayout,
                                          VkImageLayout newLayout)
{
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkImageMemoryBarrier barrier {};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VkPipelineStageFlags sourceStage;
  VkPipelineStageFlags destinationStage;

  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
      newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  }
  else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
           newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  }
  else {
    throw std::invalid_argument("Unsupported layout transition!");
  }

  vkCmdPipelineBarrier(commandBuffer,
                       sourceStage,
                       destinationStage,
                       0,
                       0,
                       nullptr,
                       0,
                       nullptr,
                       1,
                       &barrier);

  endSingleTimeCommands(commandBuffer);
}

void VulkanContext::copyBufferToImage(VkBuffer buffer,
                                      VkImage image,
                                      uint32_t width,
                                      uint32_t height)
{
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkBufferImageCopy region {};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;

  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;

  region.imageOffset = { 0, 0, 0 };
  region.imageExtent = { width, height, 1 };

  vkCmdCopyBufferToImage(commandBuffer,
                         buffer,
                         image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         1,
                         &region);

  endSingleTimeCommands(commandBuffer);
}

void VulkanContext::copyBuffer(VkBuffer srcBuffer,
                               VkBuffer dstBuffer,
                               VkDeviceSize size)
{
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkBufferCopy copyRegion {};
  copyRegion.size = size;
  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

  endSingleTimeCommands(commandBuffer);
}

}  // namespace tewduwu