#pragma once

#include <SDL3/SDL.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <memory>

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
    void createSwapChain();
    void createRenderPass();
    void createFramebuffers();
    void createCommandPool();
    void createCommandBuffers();
    void createSyncObjects();
    
    // Rendering functions
    void beginFrame();
    void endFrame();
    void waitIdle();
    
    // Accessors
    VkDevice getDevice() const;
    VkPhysicalDevice getPhysicalDevice() const;
    VkCommandPool getCommandPool() const;
    VkRenderPass getRenderPass() const;
    VkExtent2D getSwapChainExtent() const;
    
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
    
    // Sync objects
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    
    // State
    size_t currentFrame;
    bool framebufferResized;
};

} // namespace tewduwu