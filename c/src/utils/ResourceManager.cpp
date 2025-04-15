#include "ResourceManager.h"
#include "../renderer/Shader.h"
#include "../renderer/TextRenderer.h"
#include "../renderer/VulkanContext.h"
#include <iostream>

namespace tewduwu {

void ResourceManager::initialize(VulkanContext& context) {
    if (initialized) return;
    
    std::cout << "Initializing ResourceManager..." << std::endl;
    
    // Load default shaders
    loadShader(context, "glass", "shaders/glass.vert.spv", "shaders/glass.frag.spv");
    
    // Create default text renderer
    auto textRenderer = std::make_shared<TextRenderer>();
    if (textRenderer->initialize(context)) {
        textRenderers["default"] = textRenderer;
    } else {
        std::cerr << "ERROR: Failed to initialize default text renderer." << std::endl;
    }
    
    initialized = true;
    std::cout << "ResourceManager initialized successfully." << std::endl;
}

void ResourceManager::cleanup(VulkanContext& context) {
    if (!initialized) return;
    
    std::cout << "Cleaning up ResourceManager resources..." << std::endl;
    
    // Clean up shaders
    for (auto& pair : shaders) {
        if (pair.second) {
            pair.second->cleanup(context);
        }
    }
    shaders.clear();
    
    // Clean up text renderers
    for (auto& pair : textRenderers) {
        if (pair.second) {
            pair.second->cleanup();
        }
    }
    textRenderers.clear();
    
    initialized = false;
    std::cout << "ResourceManager cleanup complete." << std::endl;
}

std::shared_ptr<Shader> ResourceManager::loadShader(VulkanContext& context, const std::string& name, 
                                                 const std::string& vertPath, const std::string& fragPath) {
    // Check if shader already exists
    auto it = shaders.find(name);
    if (it != shaders.end()) {
        return it->second;
    }
    
    // Create new shader
    auto shader = std::make_shared<Shader>();
    if (shader->initialize(context, vertPath, fragPath)) {
        shaders[name] = shader;
        std::cout << "Loaded shader: " << name << std::endl;
        return shader;
    } else {
        std::cerr << "ERROR: Failed to load shader: " << name << std::endl;
        return nullptr;
    }
}

std::shared_ptr<Shader> ResourceManager::getShader(const std::string& name) {
    auto it = shaders.find(name);
    if (it != shaders.end()) {
        return it->second;
    }
    
    std::cerr << "WARNING: Shader not found: " << name << std::endl;
    return nullptr;
}

bool ResourceManager::loadFont(VulkanContext& context, const std::string& name, 
                            const std::string& path, unsigned int size) {
    // Check if we already have a text renderer for this font
    auto it = textRenderers.find(name);
    if (it != textRenderers.end()) {
        // Update existing text renderer with new font
        return it->second->loadFont(path, size);
    }
    
    // Create new text renderer for this font
    auto textRenderer = std::make_shared<TextRenderer>();
    if (!textRenderer->initialize(context)) {
        std::cerr << "ERROR: Failed to initialize text renderer for font: " << name << std::endl;
        return false;
    }
    
    if (!textRenderer->loadFont(path, size)) {
        std::cerr << "ERROR: Failed to load font: " << path << std::endl;
        return false;
    }
    
    textRenderers[name] = textRenderer;
    std::cout << "Loaded font: " << name << " (" << path << " at " << size << "pt)" << std::endl;
    return true;
}

std::shared_ptr<TextRenderer> ResourceManager::getTextRenderer(const std::string& fontName) {
    auto it = textRenderers.find(fontName);
    if (it != textRenderers.end()) {
        return it->second;
    }
    
    // Try to return default text renderer if the requested one is not found
    it = textRenderers.find("default");
    if (it != textRenderers.end()) {
        std::cerr << "WARNING: Font not found: " << fontName << ", using default instead." << std::endl;
        return it->second;
    }
    
    std::cerr << "ERROR: No text renderers available." << std::endl;
    return nullptr;
}

} // namespace tewduwu