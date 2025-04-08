#pragma once

#include <unordered_map>
#include <string>
#include <memory>

namespace tewduwu {

class Shader;
class TextRenderer;
class VulkanContext;

// Singleton Resource Manager for shaders, textures, fonts, etc.
class ResourceManager {
public:
    static ResourceManager& getInstance() {
        static ResourceManager instance;
        return instance;
    }
    
    // Initialize resources
    void initialize(VulkanContext& context);
    
    // Clean up all resources
    void cleanup(VulkanContext& context);
    
    // Shader management
    std::shared_ptr<Shader> loadShader(VulkanContext& context, const std::string& name, 
                                      const std::string& vertPath, const std::string& fragPath);
    std::shared_ptr<Shader> getShader(const std::string& name);
    
    // Font management
    bool loadFont(VulkanContext& context, const std::string& name, 
                 const std::string& path, unsigned int size = 24);
    std::shared_ptr<TextRenderer> getTextRenderer(const std::string& fontName);
    
private:
    // Private constructor and destructor for singleton
    ResourceManager() {}
    ~ResourceManager() {}
    
    // Prevent copy construction and assignment
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
    
    // Resource collections
    std::unordered_map<std::string, std::shared_ptr<Shader>> shaders;
    std::unordered_map<std::string, std::shared_ptr<TextRenderer>> textRenderers;
    
    // Flag to track initialization state
    bool initialized = false;
};

} // namespace tewduwu