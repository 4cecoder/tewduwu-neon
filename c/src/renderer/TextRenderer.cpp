#include "TextRenderer.h"
#include "Shader.h"
#include "Vertex.h"
#include "VulkanContext.h"
#include <SDL.h>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace tewduwu {

TextRenderer::TextRenderer()
  : ftLibrary(nullptr)
  , fontFace(nullptr)
  , textShader(nullptr)
  , vertexBuffer(VK_NULL_HANDLE)
  , vertexBufferMemory(VK_NULL_HANDLE)
  , descriptorSetLayout(VK_NULL_HANDLE)
  , descriptorPool(VK_NULL_HANDLE)
  , textureSampler(VK_NULL_HANDLE)
{ }

TextRenderer::~TextRenderer()
{
  // Note: Actual cleanup should happen in cleanup()
}

bool TextRenderer::initialize(VulkanContext& context)
{
  // Initialize FreeType
  if (FT_Init_FreeType(&ftLibrary)) {
    std::cerr << "ERROR: Failed to initialize FreeType library." << std::endl;
    return false;
  }

  // Create descriptor set layout for the sampler (Set 1, Binding 0)
  if (!createDescriptorSetLayout(context)) {
    std::cerr << "ERROR: Failed to create text sampler descriptor set layout."
              << std::endl;
    return false;
  }

  // Create sampler
  if (!createSampler(context)) {
    std::cerr << "ERROR: Failed to create texture sampler." << std::endl;
    return false;
  }

  // Create shader, passing the sampler layout
  textShader = std::make_shared<Shader>();
  if (!textShader->initialize(
        context,
        "shaders/text.vert.spv",
        "shaders/text.frag.spv",
        descriptorSetLayout)) {  // Pass the created sampler layout
    std::cerr << "ERROR: Failed to initialize text shaders." << std::endl;
    return false;
  }

  // Create vertex buffer
  if (!createVertexBuffer(context)) {
    std::cerr << "ERROR: Failed to create vertex buffer." << std::endl;
    return false;
  }

  // Create descriptor pool for sampler sets (needs to be after layout creation)
  if (!createDescriptorPool(context)) {
    std::cerr << "ERROR: Failed to create descriptor pool." << std::endl;
    return false;
  }

  std::cout << "TextRenderer initialized successfully." << std::endl;
  return true;
}

void TextRenderer::cleanup()
{
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

void TextRenderer::cleanupDeviceResources(VulkanContext& context)
{
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

bool TextRenderer::loadFont(VulkanContext& context,
                            const std::string& fontPath,
                            unsigned int fontSize)
{
  // Get the application's base path
  char* basePathC = SDL_GetBasePath();
  if (!basePathC) {
    std::cerr << "ERROR: Could not get application base path: "
              << SDL_GetError() << std::endl;
    // Attempt to load directly from relative path as a last resort
    basePathC = SDL_strdup("./");
  }
  std::string basePath(basePathC);
  SDL_free(basePathC);

  // Construct the full path to the font file
  std::string fullFontPath = basePath + fontPath;
  std::cout << "Attempting to load font from: " << fullFontPath << std::endl;

  // First, try to load the specified font using the full path
  FT_Face face;
  if (FT_New_Face(ftLibrary, fullFontPath.c_str(), 0, &face) == 0) {
    std::cout << "Successfully loaded font: " << fullFontPath << std::endl;
    FT_Set_Pixel_Sizes(face, 0, fontSize);
    if (!loadFontCharacters(
          context, face)) {  // Pass VulkanContext to loadFontCharacters
      FT_Done_Face(face);
      return false;
    }
    FT_Done_Face(face);
    return true;
  }
  else {
    std::cerr << "Failed to load font: " << fullFontPath << std::endl;
    // If the specified font fails, try a system default font as fallback
    const std::string fallbackPath = "/System/Library/Fonts/Menlo.ttc";
    std::cout << "Attempting to load fallback system font: " << fallbackPath
              << std::endl;
    if (FT_New_Face(ftLibrary, fallbackPath.c_str(), 0, &face) == 0) {
      std::cout << "Successfully loaded fallback system font: " << fallbackPath
                << std::endl;
      FT_Set_Pixel_Sizes(face, 0, fontSize);
      if (!loadFontCharacters(
            context, face)) {  // Pass VulkanContext to loadFontCharacters
        FT_Done_Face(face);
        return false;
      }
      FT_Done_Face(face);
      return true;
    }
    else {
      std::cerr << "Failed to load fallback system font: " << fallbackPath
                << std::endl;
      return false;
    }
  }
}

bool TextRenderer::createTextureImage(VulkanContext& context,
                                      FT_Face face,
                                      char c,
                                      Character& character)
{
  // Load glyph
  if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
    std::cerr << "ERROR: Failed to load glyph for character: " << c
              << std::endl;
    return false;
  }

  FT_GlyphSlot glyph = face->glyph;
  FT_Bitmap& bitmap = glyph->bitmap;

  // --- Check for zero-dimension glyphs ---
  if (bitmap.width == 0 || bitmap.rows == 0) {
    // Glyph has no visual representation (e.g., space)
    // Store metrics, but skip Vulkan image creation
    character.image = VK_NULL_HANDLE;
    character.imageView = VK_NULL_HANDLE;
    character.memory = VK_NULL_HANDLE;
    character.size = glm::ivec2(0, 0);
    character.bearing = glm::ivec2(glyph->bitmap_left, glyph->bitmap_top);
    character.advance = static_cast<unsigned int>(glyph->advance.x >> 6);
    return true;  // Successfully handled this non-visual character
  }
  // --- End check ---

  // Create image (Only if dimensions are non-zero)
  VkImageCreateInfo imageInfo {};
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
  imageInfo.usage =
    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

  VkDevice device = context.getDevice();

  if (vkCreateImage(device, &imageInfo, nullptr, &character.image) !=
      VK_SUCCESS) {
    std::cerr << "ERROR: Failed to create texture image for character: " << c
              << std::endl;
    return false;
  }

  // Allocate memory for the image
  // (Note: In a full implementation, we'd need to get memory requirements and find suitable memory type)
  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(device, character.image, &memRequirements);

  VkMemoryAllocateInfo allocInfo {};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex =
    findMemoryType(context,
                   memRequirements.memoryTypeBits,
                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  if (vkAllocateMemory(device, &allocInfo, nullptr, &character.memory) !=
      VK_SUCCESS) {
    std::cerr << "ERROR: Failed to allocate image memory for character: " << c
              << std::endl;
    return false;
  }

  vkBindImageMemory(device, character.image, character.memory, 0);

  // Upload bitmap data to the image via staging buffer
  VkDeviceSize imageSize =
    static_cast<VkDeviceSize>(bitmap.width) * bitmap.rows;
  if (imageSize > 0 && bitmap.buffer != nullptr) {
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    // Create staging buffer
    context.createBuffer(imageSize,
                         VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         stagingBuffer,
                         stagingBufferMemory);

    // Copy data to staging buffer
    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, bitmap.buffer, imageSize);
    vkUnmapMemory(device, stagingBufferMemory);

    // Transition image layout and copy buffer to image
    context.transitionImageLayout(character.image,
                                  VK_FORMAT_R8_UNORM,
                                  VK_IMAGE_LAYOUT_UNDEFINED,
                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    context.copyBufferToImage(stagingBuffer,
                              character.image,
                              static_cast<uint32_t>(bitmap.width),
                              static_cast<uint32_t>(bitmap.rows));
    context.transitionImageLayout(character.image,
                                  VK_FORMAT_R8_UNORM,
                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // Cleanup staging buffer
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
  }

  // Create image view
  VkImageViewCreateInfo viewInfo {};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = character.image;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = VK_FORMAT_R8_UNORM;
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  if (vkCreateImageView(device, &viewInfo, nullptr, &character.imageView) !=
      VK_SUCCESS) {
    std::cerr << "ERROR: Failed to create image view for character: " << c
              << std::endl;
    // Cleanup already created image and memory before returning
    vkDestroyImage(device, character.image, nullptr);
    vkFreeMemory(device, character.memory, nullptr);
    character.image = VK_NULL_HANDLE;
    character.memory = VK_NULL_HANDLE;
    return false;
  }

  // --- Allocate and Update Descriptor Set for this character ---
  VkDescriptorSetAllocateInfo setAllocInfo {};
  setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  setAllocInfo.descriptorPool = descriptorPool;  // Use TextRenderer's pool
  setAllocInfo.descriptorSetCount = 1;
  setAllocInfo.pSetLayouts =
    &descriptorSetLayout;  // Use TextRenderer's sampler layout

  if (vkAllocateDescriptorSets(context.getDevice(),
                               &setAllocInfo,
                               &character.descriptorSet) != VK_SUCCESS) {
    std::cerr << "ERROR: Failed to allocate descriptor set for character '" << c
              << "'" << std::endl;
    // Cleanup image view, image, memory
    vkDestroyImageView(device, character.imageView, nullptr);
    vkDestroyImage(device, character.image, nullptr);
    vkFreeMemory(device, character.memory, nullptr);
    character.imageView = VK_NULL_HANDLE;
    character.image = VK_NULL_HANDLE;
    character.memory = VK_NULL_HANDLE;
    return false;
  }

  VkDescriptorImageInfo imageDescriptorInfo {};
  imageDescriptorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageDescriptorInfo.imageView = character.imageView;
  imageDescriptorInfo.sampler = textureSampler;

  VkWriteDescriptorSet descriptorWrite {};
  descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrite.dstSet =
    character.descriptorSet;       // Write to the character's set
  descriptorWrite.dstBinding = 0;  // Binding 0 for sampler layout
  descriptorWrite.dstArrayElement = 0;
  descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  descriptorWrite.descriptorCount = 1;
  descriptorWrite.pImageInfo = &imageDescriptorInfo;

  vkUpdateDescriptorSets(context.getDevice(), 1, &descriptorWrite, 0, nullptr);
  // --- End Descriptor Set Logic ---

  // Store character metrics
  character.size = glm::ivec2(glyph->bitmap.width, glyph->bitmap.rows);
  character.bearing = glm::ivec2(glyph->bitmap_left, glyph->bitmap_top);
  character.advance = static_cast<unsigned int>(glyph->advance.x >> 6);

  return true;
}

uint32_t TextRenderer::findMemoryType(VulkanContext& context,
                                      uint32_t typeFilter,
                                      VkMemoryPropertyFlags properties)
{
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(context.getPhysicalDevice(),
                                      &memProperties);

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags &
                                    properties) == properties) {
      return i;
    }
  }

  return UINT32_MAX;
}

bool TextRenderer::createVertexBuffer(VulkanContext& context)
{
  // Create a buffer large enough for rendering a quad of text
  VkDeviceSize bufferSize = sizeof(TextVertex) * 6;  // 6 vertices for a quad

  VkBufferCreateInfo bufferInfo {};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = bufferSize;
  bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(
        context.getDevice(), &bufferInfo, nullptr, &vertexBuffer) !=
      VK_SUCCESS) {
    std::cerr << "ERROR: Failed to create vertex buffer for text rendering."
              << std::endl;
    return false;
  }

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(
    context.getDevice(), vertexBuffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo {};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = findMemoryType(
    context,
    memRequirements.memoryTypeBits,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  if (vkAllocateMemory(
        context.getDevice(), &allocInfo, nullptr, &vertexBufferMemory) !=
      VK_SUCCESS) {
    std::cerr
      << "ERROR: Failed to allocate vertex buffer memory for text rendering."
      << std::endl;
    return false;
  }

  vkBindBufferMemory(context.getDevice(), vertexBuffer, vertexBufferMemory, 0);

  return true;
}

bool TextRenderer::createDescriptorSetLayout(VulkanContext& context)
{
  VkDescriptorSetLayoutBinding samplerLayoutBinding {};
  samplerLayoutBinding.binding = 0;
  samplerLayoutBinding.descriptorCount = 1;
  samplerLayoutBinding.descriptorType =
    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  samplerLayoutBinding.pImmutableSamplers = nullptr;

  VkDescriptorSetLayoutCreateInfo layoutInfo {};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = 1;
  layoutInfo.pBindings = &samplerLayoutBinding;

  if (vkCreateDescriptorSetLayout(
        context.getDevice(), &layoutInfo, nullptr, &descriptorSetLayout) !=
      VK_SUCCESS) {
    std::cerr
      << "ERROR: Failed to create descriptor set layout for text sampler."
      << std::endl;
    return false;
  }

  return true;
}

bool TextRenderer::createDescriptorPool(VulkanContext& context)
{
  VkDescriptorPoolSize poolSize {};
  poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSize.descriptorCount = 128;  // Support up to 128 characters

  VkDescriptorPoolCreateInfo poolInfo {};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = 1;
  poolInfo.pPoolSizes = &poolSize;
  poolInfo.maxSets = 128;

  if (vkCreateDescriptorPool(
        context.getDevice(), &poolInfo, nullptr, &descriptorPool) !=
      VK_SUCCESS) {
    std::cerr << "ERROR: Failed to create descriptor pool for text rendering."
              << std::endl;
    return false;
  }

  return true;
}

bool TextRenderer::createSampler(VulkanContext& context)
{
  VkSamplerCreateInfo samplerInfo {};
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

  if (vkCreateSampler(
        context.getDevice(), &samplerInfo, nullptr, &textureSampler) !=
      VK_SUCCESS) {
    std::cerr << "ERROR: Failed to create texture sampler for text rendering."
              << std::endl;
    return false;
  }

  return true;
}

void TextRenderer::renderText(VulkanContext& context,
                              const std::string& text,
                              float x,
                              float y,
                              float scale,
                              const glm::vec4& color)
{
  if (text.empty() || !textShader || descriptorPool == VK_NULL_HANDLE)
    return;

  // Bind shader
  VkCommandBuffer cmdBuffer = context.getCurrentCommandBuffer();
  textShader->bind(cmdBuffer);

  // Set UBO values
  textShader->setUniformVec4("textColor", color);

  // Setup orthographic projection (screen space coordinates)
  glm::mat4 projection =
    glm::ortho(0.0f,
               static_cast<float>(context.getSwapChainExtent().width),
               0.0f,
               static_cast<float>(context.getSwapChainExtent().height));
  textShader->setUniformMat4("projection", projection);

  // Update UBOs AFTER setting all values for this context
  textShader->updateUniformBuffers(context);

  // Set dynamic viewport and scissor state (once before the loop)
  VkExtent2D extent = context.getSwapChainExtent();
  VkViewport viewport {};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)extent.width;
  viewport.height = (float)extent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

  VkRect2D scissor {};
  scissor.offset = { 0, 0 };
  scissor.extent = extent;  // Use swapchain extent
  vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

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
      }
      else {
        continue;  // Skip this character if loading fails
      }
    }

    const Character& ch = it->second;

    // Skip rendering if character has no visual representation (no image)
    if (ch.image == VK_NULL_HANDLE) {
      xPos += (ch.advance) * scale;  // Still advance the cursor
      continue;
    }

    // Bind the Shader's UBO descriptor set to set 0
    VkDescriptorSet uboSet = textShader->getDescriptorSet();
    if (uboSet != VK_NULL_HANDLE) {  // Check if UBO set is valid
      vkCmdBindDescriptorSets(cmdBuffer,
                              VK_PIPELINE_BIND_POINT_GRAPHICS,
                              textShader->getPipelineLayout(),
                              0,
                              1,
                              &uboSet,
                              0,
                              nullptr);
    }
    else {
      std::cerr << "Warning: UBO Descriptor Set is null." << std::endl;
    }

    // Bind Sampler Set (Set 1)
    vkCmdBindDescriptorSets(cmdBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            textShader->getPipelineLayout(),
                            1,
                            1,
                            &ch.descriptorSet,
                            0,
                            nullptr);
    // --- End Sampler Descriptor Set Logic ---

    // Calculate vertex positions for the character quad
    float xpos = xPos + ch.bearing.x * scale;
    float ypos = y - (ch.size.y - ch.bearing.y) * scale;
    float w = ch.size.x * scale;
    float h = ch.size.y * scale;

    // --- DEBUG LOG ---
    std::cout << "Char: '" << c << "' | xPos: " << xPos << " | yPos: " << y
              << " | bearingX: " << ch.bearing.x
              << " | bearingY: " << ch.bearing.y << " | sizeW: " << ch.size.x
              << " | sizeH: " << ch.size.y << " | quadX: " << xpos
              << " | quadY: " << ypos << " | quadW: " << w << " | quadH: " << h
              << " | advance: " << ch.advance << std::endl;
    // --- END DEBUG LOG ---

    // Update vertex buffer data for this character
    TextVertex vertices[6] = {
      { { xpos, ypos + h },
        { 0.0f, 0.0f } },  // Bottom-Left Quad, Top-Left Tex (V=0)
      { { xpos, ypos },
        { 0.0f, 1.0f } },  // Top-Left Quad, Bottom-Left Tex (V=1)
      { { xpos + w, ypos },
        { 1.0f, 1.0f } },  // Top-Right Quad, Bottom-Right Tex (V=1)

      { { xpos, ypos + h },
        { 0.0f, 0.0f } },  // Bottom-Left Quad, Top-Left Tex (V=0)
      { { xpos + w, ypos },
        { 1.0f, 1.0f } },  // Top-Right Quad, Bottom-Right Tex (V=1)
      { { xpos + w, ypos + h },
        { 1.0f, 0.0f } }  // Bottom-Right Quad, Top-Right Tex (V=0)
    };

    // Map and copy vertex data
    void* data;
    vkMapMemory(
      context.getDevice(), vertexBufferMemory, 0, sizeof(vertices), 0, &data);
    memcpy(data, vertices, sizeof(vertices));
    vkUnmapMemory(context.getDevice(), vertexBufferMemory);

    // Bind vertex buffer and draw quad
    VkBuffer vertexBuffers[] = { vertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdDraw(cmdBuffer, 6, 1, 0, 0);

    // Advance cursor for next glyph
    xPos += (ch.advance) * scale;
  }
}

float TextRenderer::getTextWidth(const std::string& text, float scale)
{
  float width = 0.0f;

  for (const char c : text) {
    auto it = characters.find(c);
    if (it != characters.end()) {
      width += (it->second.advance >> 6) * scale;
    }
    else if (fontFace) {
      // Try to get width from font directly if character not cached
      if (FT_Load_Char(fontFace, c, FT_LOAD_DEFAULT) == 0) {
        width += (fontFace->glyph->advance.x >> 6) * scale;
      }
    }
  }

  return width;
}

float TextRenderer::getTextHeight(const std::string& text, float scale)
{
  if (text.empty())
    return 0.0f;

  float maxHeight = 0.0f;

  for (const char c : text) {
    auto it = characters.find(c);
    float charHeight = 0.0f;

    if (it != characters.end()) {
      charHeight = it->second.size.y * scale;
    }
    else if (fontFace) {
      // Try to get height from font directly if character not cached
      if (FT_Load_Char(fontFace, c, FT_LOAD_DEFAULT) == 0) {
        charHeight = fontFace->glyph->bitmap.rows * scale;
      }
    }

    maxHeight = std::max(maxHeight, charHeight);
  }

  return maxHeight;
}

bool TextRenderer::loadFontCharacters(VulkanContext& context, FT_Face face)
{
  // Load first 128 ASCII characters
  for (unsigned char c = 0; c < 128; c++) {
    // Load character glyph
    // Use FT_LOAD_RENDER to get a renderable bitmap
    if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
      std::cerr << "Warning: Failed to load Glyph: " << c << std::endl;
      continue;
    }

    // Create Vulkan texture for the character
    Character character = {};  // Initialize struct
    if (!createTextureImage(context, face, c, character)) {
      std::cerr << "Warning: Failed to create texture for Glyph: " << c
                << std::endl;
      // Optionally continue, or return false if any character fails
      continue;
    }

    // Store metrics (already done within createTextureImage implicitly if needed,
    // but store explicit metrics here too for immediate access)
    character.size =
      glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows);
    character.bearing =
      glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top);
    // Advance is the horizontal distance to move for the next character
    character.advance = static_cast<unsigned int>(face->glyph->advance.x >> 6);

    characters.insert(std::pair<char, Character>(c, character));
  }

  return true;
}

}  // namespace tewduwu