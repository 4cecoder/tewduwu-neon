#pragma once

#include <ft2build.h>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include FT_FREETYPE_H
#include <vulkan/vulkan.h>

namespace tewduwu {

class VulkanContext;
class Shader;

// Character glyph data
struct Character {
  VkImage image = VK_NULL_HANDLE;
  VkImageView imageView = VK_NULL_HANDLE;
  VkDeviceMemory memory = VK_NULL_HANDLE;
  VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
  glm::ivec2 size;
  glm::ivec2 bearing;
  unsigned int advance;
};

class TextRenderer {
public:
  TextRenderer();
  ~TextRenderer();

  bool initialize(VulkanContext& context);
  void cleanup();

  // Render text
  void renderText(VulkanContext& context,
                  const std::string& text,
                  float x,
                  float y,
                  float scale,
                  const glm::vec4& color);

  // Font management
  bool loadFont(VulkanContext& context,
                const std::string& fontPath,
                unsigned int fontSize = 24);

  // Text measurement
  float getTextWidth(const std::string& text, float scale = 1.0f);
  float getTextHeight(const std::string& text, float scale = 1.0f);

  // Make cleanupDeviceResources public for use by other classes
  void cleanupDeviceResources(VulkanContext& context);

private:
  FT_Library ftLibrary;
  FT_Face fontFace;

  std::unordered_map<char, Character> characters;
  std::shared_ptr<Shader> textShader;

  VkBuffer vertexBuffer;
  VkDeviceMemory vertexBufferMemory;
  VkDescriptorSetLayout descriptorSetLayout;
  VkDescriptorPool descriptorPool;
  VkSampler textureSampler;

  // Helper function to load character glyphs from a font face
  bool loadFontCharacters(VulkanContext& context, FT_Face face);

  uint32_t findMemoryType(VulkanContext& context,
                          uint32_t typeFilter,
                          VkMemoryPropertyFlags properties);

  bool createTextureImage(VulkanContext& context,
                          FT_Face face,
                          char c,
                          Character& character);
  bool createVertexBuffer(VulkanContext& context);
  bool createDescriptorSetLayout(VulkanContext& context);
  bool createDescriptorPool(VulkanContext& context);
  bool createSampler(VulkanContext& context);
};

}  // namespace tewduwu