#pragma once

#include <SDL2/SDL.h>
#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

namespace tewduwu {

class VulkanContext {
public:
  VulkanContext();
  ~VulkanContext();

  // Initialization
  bool initialize(SDL_Window* window);
  void cleanup();

  // Core Vulkan setup
  void createInstance();
  void setupDebugMessenger();
  void createSurface(SDL_Window* window);
  void pickPhysicalDevice();
  void createLogicalDevice();
  void createSwapChain(SDL_Window* window);
  void createRenderPass();
  void createFramebuffers();
  void createCommandPool();
  void createCommandBuffers();
  void createSyncObjects();

  // Swap chain recreation
  void recreateSwapChain(SDL_Window* window);
  void cleanupSwapChain();

  // Rendering functions
  void beginFrame();
  void endFrame();
  void waitIdle();

  // Handle window resize
  void framebufferResizedCallback() { framebufferResized = true; }

  // Accessors
  VkDevice getDevice() const;
  VkPhysicalDevice getPhysicalDevice() const;
  VkCommandPool getCommandPool() const;
  VkRenderPass getRenderPass() const;
  VkExtent2D getSwapChainExtent() const;
  VkCommandBuffer getCurrentCommandBuffer() const;
  VkDescriptorPool getDescriptorPool() const;
  VkQueue getPresentQueue() const { return presentQueue; }

  // Vulkan helper functions
  uint32_t findMemoryType(uint32_t typeFilter,
                          VkMemoryPropertyFlags properties);
  void createBuffer(VkDeviceSize size,
                    VkBufferUsageFlags usage,
                    VkMemoryPropertyFlags properties,
                    VkBuffer& buffer,
                    VkDeviceMemory& bufferMemory);
  void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
  VkCommandBuffer beginSingleTimeCommands();
  void endSingleTimeCommands(VkCommandBuffer commandBuffer);
  void transitionImageLayout(VkImage image,
                             VkFormat format,
                             VkImageLayout oldLayout,
                             VkImageLayout newLayout);
  void copyBufferToImage(VkBuffer buffer,
                         VkImage image,
                         uint32_t width,
                         uint32_t height);

private:
  // Vulkan objects
  VkInstance instance;
  VkDebugUtilsMessengerEXT debugMessenger;
  VkSurfaceKHR surface;
  VkPhysicalDevice physicalDevice;
  VkDevice device;
  VkQueue graphicsQueue;
  VkQueue presentQueue;
  VkSwapchainKHR swapChain;
  std::vector<VkImage> swapChainImages;
  VkFormat swapChainImageFormat;
  VkExtent2D swapChainExtent;
  std::vector<VkImageView> swapChainImageViews;
  VkRenderPass renderPass;
  std::vector<VkFramebuffer> swapChainFramebuffers;
  VkCommandPool commandPool;
  std::vector<VkCommandBuffer> commandBuffers;
  VkDescriptorPool descriptorPool;

  // Sync objects
  std::vector<VkSemaphore> imageAvailableSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores;
  std::vector<VkFence> inFlightFences;

  // State
  SDL_Window* window;  // Store window pointer for recreating swap chain
  size_t currentFrame;
  bool framebufferResized;
  uint32_t currentImageIndex;

  // Helper functions
  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
  VkSurfaceFormatKHR chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& availableFormats);
  VkPresentModeKHR chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR>& availablePresentModes);
  bool createDescriptorPool();
};

}  // namespace tewduwu