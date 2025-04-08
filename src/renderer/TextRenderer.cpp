#include "TextRenderer.h"
#include "VulkanContext.h"
#include "Shader.h"
#include <iostream>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

namespace tewduwu {

struct TextVertex {
    glm::vec2 pos;
    glm::vec2 texCoord;
};

TextRenderer::TextRenderer() : ftLibrary(nullptr), fontFace(nullptr), textShader(nullptr), 
                               vertexBuffer(VK_NULL_HANDLE), vertexBufferMemory(VK_NULL_HANDLE),
                               descriptorSetLayout(VK_NULL_HANDLE), descriptorPool(VK_NULL_HANDLE),
                               textureSampler(VK_NULL_HANDLE) {
}

TextRenderer::~TextRenderer() {
    // Note: Actual cleanup should happen in cleanup()
}

bool TextRenderer::initialize(VulkanContext& context) {
    // Initialize FreeType
    if (FT_Init_FreeType(&ftLibrary)) {
        std::cerr << "ERROR: Failed to initialize FreeType library." << std::endl;
        return false;
    }
    
    // Create shader
    textShader = std::make_shared<Shader>();
    if (!textShader->initialize(context, "shaders/text.vert.spv", "shaders/text.frag.spv")) {
        std::cerr << "ERROR: Failed to load text shaders. Creating default shader bindings." << std::endl;
        // For now, we'll continue without proper shaders for development
    }
    
    // Create vertex buffer
    if (!createVertexBuffer(context)) {
        std::cerr << "ERROR: Failed to create vertex buffer." << std::endl;
        return false;
    }
    
    // Create descriptor set layout
    if (!createDescriptorSetLayout(context)) {
        std::cerr << "ERROR: Failed to create descriptor set layout." << std::endl;
        return false;
    }
    
    // Create descriptor pool
    if (!createDescriptorPool(context)) {
        std::cerr << "ERROR: Failed to create descriptor pool." << std::endl;
        return false;
    }
    
    // Create sampler
    if (!createSampler(context)) {
        std::cerr << "ERROR: Failed to create texture sampler." << std::endl;
        return false;
    }
    
    std::cout << "TextRenderer initialized successfully." << std::endl;
    return true;
}

void TextRenderer::cleanup() {
    // Clean up FreeType resources
    if (fontFace) {
        FT_Done_Face(fontFace);
        fontFace = nullptr;
    }
    
    if (ftLibrary) {
        FT_Done_FreeType(ftLibrary);
        ftLibrary = nullptr;
    }
    
    characters.clear();
}

void TextRenderer::cleanupDeviceResources(VulkanContext& context) {
    VkDevice device = context.getDevice();
    
    // Clean up character textures
    for (auto& pair : characters) {
        Character& ch = pair.second;
        vkDestroyImageView(device, ch.imageView, nullptr);
        vkDestroyImage(device, ch.image, nullptr);
        vkFreeMemory(device, ch.memory, nullptr);
    }
    
    // Clean up sampler
    if (textureSampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, textureSampler, nullptr);
        textureSampler = VK_NULL_HANDLE;
    }
    
    // Clean up descriptor resources
    if (descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        descriptorPool = VK_NULL_HANDLE;
    }
    
    if (descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        descriptorSetLayout = VK_NULL_HANDLE;
    }
    
    // Clean up vertex buffer
    if (vertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, vertexBuffer, nullptr);
        vertexBuffer = VK_NULL_HANDLE;
    }
    
    if (vertexBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, vertexBufferMemory, nullptr);
        vertexBufferMemory = VK_NULL_HANDLE;
    }
    
    // Clean up shader
    if (textShader) {
        textShader->cleanup(context);
        textShader = nullptr;
    }
}

bool TextRenderer::loadFont(const std::string& fontPath, unsigned int fontSize) {
    // Clean up previous font if any
    if (fontFace) {
        FT_Done_Face(fontFace);
        fontFace = nullptr;
    }
    
    // Load font face
    if (FT_New_Face(ftLibrary, fontPath.c_str(), 0, &fontFace)) {
        std::cerr << "ERROR: Failed to load font: " << fontPath << std::endl;
        return false;
    }
    
    // Set font size
    FT_Set_Pixel_Sizes(fontFace, 0, fontSize);
    
    // Success message
    std::cout << "Font loaded: " << fontPath << " at size " << fontSize << std::endl;
    
    return true;
}

bool TextRenderer::createTextureImage(VulkanContext& context, FT_Face face, char c, Character& character) {
    // Load glyph
    if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
        std::cerr << "ERROR: Failed to load glyph for character: " << c << std::endl;
        return false;
    }
    
    FT_GlyphSlot glyph = face->glyph;
    FT_Bitmap& bitmap = glyph->bitmap;
    
    // Create image
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = bitmap.width;
    imageInfo.extent.height = bitmap.rows;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8_UNORM;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    
    VkDevice device = context.getDevice();
    
    if (vkCreateImage(device, &imageInfo, nullptr, &character.image) != VK_SUCCESS) {
        std::cerr << "ERROR: Failed to create texture image for character: " << c << std::endl;
        return false;
    }
    
    // Allocate memory for the image
    // (Note: In a full implementation, we'd need to get memory requirements and find suitable memory type)
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, character.image, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(context, memRequirements.memoryTypeBits, 
                                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    if (vkAllocateMemory(device, &allocInfo, nullptr, &character.memory) != VK_SUCCESS) {
        std::cerr << "ERROR: Failed to allocate image memory for character: " << c << std::endl;
        return false;
    }
    
    vkBindImageMemory(device, character.image, character.memory, 0);
    
    // Create image view
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = character.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    
    if (vkCreateImageView(device, &viewInfo, nullptr, &character.imageView) != VK_SUCCESS) {
        std::cerr << "ERROR: Failed to create image view for character: " << c << std::endl;
        return false;
    }
    
    // Store character metrics
    character.size = glm::ivec2(glyph->bitmap.width, glyph->bitmap.rows);
    character.bearing = glm::ivec2(glyph->bitmap_left, glyph->bitmap_top);
    character.advance = static_cast<unsigned int>(glyph->advance.x >> 6);
    
    return true;
}

uint32_t TextRenderer::findMemoryType(VulkanContext& context, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(context.getPhysicalDevice(), &memProperties);
    
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && 
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    
    return UINT32_MAX;
}

bool TextRenderer::createVertexBuffer(VulkanContext& context) {
    // Create a buffer large enough for rendering a quad of text
    VkDeviceSize bufferSize = sizeof(TextVertex) * 6; // 6 vertices for a quad
    
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateBuffer(context.getDevice(), &bufferInfo, nullptr, &vertexBuffer) != VK_SUCCESS) {
        std::cerr << "ERROR: Failed to create vertex buffer for text rendering." << std::endl;
        return false;
    }
    
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(context.getDevice(), vertexBuffer, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(context, memRequirements.memoryTypeBits, 
                                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                                             VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    if (vkAllocateMemory(context.getDevice(), &allocInfo, nullptr, &vertexBufferMemory) != VK_SUCCESS) {
        std::cerr << "ERROR: Failed to allocate vertex buffer memory for text rendering." << std::endl;
        return false;
    }
    
    vkBindBufferMemory(context.getDevice(), vertexBuffer, vertexBufferMemory, 0);
    
    return true;
}

bool TextRenderer::createDescriptorSetLayout(VulkanContext& context) {
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 0;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &samplerLayoutBinding;
    
    if (vkCreateDescriptorSetLayout(context.getDevice(), &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        std::cerr << "ERROR: Failed to create descriptor set layout for text rendering." << std::endl;
        return false;
    }
    
    return true;
}

bool TextRenderer::createDescriptorPool(VulkanContext& context) {
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 128; // Support up to 128 characters
    
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 128;
    
    if (vkCreateDescriptorPool(context.getDevice(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        std::cerr << "ERROR: Failed to create descriptor pool for text rendering." << std::endl;
        return false;
    }
    
    return true;
}

bool TextRenderer::createSampler(VulkanContext& context) {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    
    if (vkCreateSampler(context.getDevice(), &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
        std::cerr << "ERROR: Failed to create texture sampler for text rendering." << std::endl;
        return false;
    }
    
    return true;
}

void TextRenderer::renderText(VulkanContext& context, const std::string& text, float x, float y, 
                            float scale, const glm::vec4& color) {
    if (text.empty() || !textShader) return;
    
    // Bind shader
    VkCommandBuffer cmdBuffer = context.getCurrentCommandBuffer();
    textShader->bind(context, cmdBuffer);
    
    // Set text color uniform
    textShader->setUniformVec4("textColor", color);
    textShader->updateUniformBuffers(context);
    
    // Setup orthographic projection (screen space coordinates)
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(context.getSwapChainExtent().width),
                                      0.0f, static_cast<float>(context.getSwapChainExtent().height));
    textShader->setUniformMat4("projection", projection);
    
    // Render each character
    float xPos = x;
    for (const char c : text) {
        auto it = characters.find(c);
        if (it == characters.end()) {
            // Character not loaded yet, try to load it now
            Character character;
            if (createTextureImage(context, fontFace, c, character)) {
                characters[c] = character;
                it = characters.find(c);
            } else {
                continue;  // Skip this character if loading fails
            }
        }
        
        const Character& ch = it->second;
        
        float xpos = xPos + ch.bearing.x * scale;
        float ypos = y - (ch.size.y - ch.bearing.y) * scale;
        
        float w = ch.size.x * scale;
        float h = ch.size.y * scale;
        
        // Generate quad vertices for this character
        TextVertex vertices[6] = {
            {{xpos,     ypos + h}, {0.0f, 0.0f}},
            {{xpos,     ypos},     {0.0f, 1.0f}},
            {{xpos + w, ypos},     {1.0f, 1.0f}},
            
            {{xpos,     ypos + h}, {0.0f, 0.0f}},
            {{xpos + w, ypos},     {1.0f, 1.0f}},
            {{xpos + w, ypos + h}, {1.0f, 0.0f}}
        };
        
        // Update vertex buffer with this character's vertices
        void* data;
        vkMapMemory(context.getDevice(), vertexBufferMemory, 0, sizeof(vertices), 0, &data);
        memcpy(data, vertices, sizeof(vertices));
        vkUnmapMemory(context.getDevice(), vertexBufferMemory);
        
        // Bind descriptor set with this character's texture
        // Note: This part would need proper descriptor set management
        
        // Bind vertex buffer and draw quad
        VkBuffer vertexBuffers[] = {vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(cmdBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdDraw(cmdBuffer, 6, 1, 0, 0);
        
        // Advance cursor for next glyph
        xPos += (ch.advance >> 6) * scale;
    }
}

float TextRenderer::getTextWidth(const std::string& text, float scale) {
    float width = 0.0f;
    
    for (const char c : text) {
        auto it = characters.find(c);
        if (it != characters.end()) {
            width += (it->second.advance >> 6) * scale;
        } else if (fontFace) {
            // Try to get width from font directly if character not cached
            if (FT_Load_Char(fontFace, c, FT_LOAD_DEFAULT) == 0) {
                width += (fontFace->glyph->advance.x >> 6) * scale;
            }
        }
    }
    
    return width;
}

float TextRenderer::getTextHeight(const std::string& text, float scale) {
    if (text.empty()) return 0.0f;
    
    float maxHeight = 0.0f;
    
    for (const char c : text) {
        auto it = characters.find(c);
        float charHeight = 0.0f;
        
        if (it != characters.end()) {
            charHeight = it->second.size.y * scale;
        } else if (fontFace) {
            // Try to get height from font directly if character not cached
            if (FT_Load_Char(fontFace, c, FT_LOAD_DEFAULT) == 0) {
                charHeight = fontFace->glyph->bitmap.rows * scale;
            }
        }
        
        maxHeight = std::max(maxHeight, charHeight);
    }
    
    return maxHeight;
}

} // namespace tewduwu