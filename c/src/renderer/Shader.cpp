#include "Shader.h"
#include "Vertex.h"
#include "VulkanContext.h"
#include <array>
#include <fstream>
#include <iostream>
#include <vector>

namespace tewduwu {

Shader::Shader()
  : pipelineLayout(VK_NULL_HANDLE)
  , graphicsPipeline(VK_NULL_HANDLE)
  , vertShaderModule(VK_NULL_HANDLE)
  , fragShaderModule(VK_NULL_HANDLE)
  , uniformBuffer(VK_NULL_HANDLE)
  , uniformBufferMemory(VK_NULL_HANDLE)
  , descriptorSet(VK_NULL_HANDLE)
  , descriptorSetLayout(VK_NULL_HANDLE)
  , uniformBufferSize(0)
  , uniformsDirty(false)
  , uboDescriptorPool(VK_NULL_HANDLE)
{ }

Shader::~Shader()
{
  // Note: Actual cleanup should happen in cleanup()
}

bool Shader::initialize(VulkanContext& context,
                        const std::string& vertPath,
                        const std::string& fragPath,
                        VkDescriptorSetLayout externalLayout)
{
  // Read shader files
  auto vertShaderCode = readFile(vertPath);
  auto fragShaderCode = readFile(fragPath);

  if (vertShaderCode.empty() || fragShaderCode.empty()) {
    std::cerr << "ERROR: Failed to read shader files: " << vertPath << " or "
              << fragPath << std::endl;
    return false;
  }

  // Create shader modules
  vertShaderModule = createShaderModule(context, vertShaderCode);
  fragShaderModule = createShaderModule(context, fragShaderCode);

  if (vertShaderModule == VK_NULL_HANDLE ||
      fragShaderModule == VK_NULL_HANDLE) {
    std::cerr << "ERROR: Failed to create shader modules." << std::endl;
    return false;
  }

  // Create descriptor layout, uniform buffers, and pipeline
  if (!createDescriptorSetLayout(context) || !createUniformBuffers(context) ||
      !createDescriptorPool(context) || !createDescriptorSets(context) ||
      !createPipeline(context, externalLayout)) {
    return false;
  }

  return true;
}

void Shader::cleanup(VulkanContext& context)
{
  VkDevice device = context.getDevice();

  // Clean up pipeline
  if (graphicsPipeline != VK_NULL_HANDLE) {
    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    graphicsPipeline = VK_NULL_HANDLE;
  }

  if (pipelineLayout != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    pipelineLayout = VK_NULL_HANDLE;
  }

  // Clean up UBO descriptor pool (which also frees the set)
  if (uboDescriptorPool != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(device, uboDescriptorPool, nullptr);
    uboDescriptorPool = VK_NULL_HANDLE;
    descriptorSet = VK_NULL_HANDLE;  // Set is implicitly freed
  }

  // Clean up descriptor sets and layout
  if (descriptorSetLayout != VK_NULL_HANDLE) {
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
    descriptorSetLayout = VK_NULL_HANDLE;
  }

  // Clean up uniform buffer
  if (uniformBuffer != VK_NULL_HANDLE) {
    vkDestroyBuffer(device, uniformBuffer, nullptr);
    uniformBuffer = VK_NULL_HANDLE;
  }

  if (uniformBufferMemory != VK_NULL_HANDLE) {
    vkFreeMemory(device, uniformBufferMemory, nullptr);
    uniformBufferMemory = VK_NULL_HANDLE;
  }

  // Clean up shader modules
  if (vertShaderModule != VK_NULL_HANDLE) {
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
    vertShaderModule = VK_NULL_HANDLE;
  }

  if (fragShaderModule != VK_NULL_HANDLE) {
    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    fragShaderModule = VK_NULL_HANDLE;
  }
}

void Shader::bind(VkCommandBuffer cmdBuffer)
{
  vkCmdBindPipeline(
    cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
}

void Shader::setUniformVec4(const std::string& name, const glm::vec4& value)
{
  UniformData data;
  data.type = UniformData::Type::Vec4;
  data.vec4Value = value;
  uniformValues[name] = data;
  uniformsDirty = true;
}

void Shader::setUniformVec3(const std::string& name, const glm::vec3& value)
{
  UniformData data;
  data.type = UniformData::Type::Vec3;
  data.vec3Value = value;
  uniformValues[name] = data;
  uniformsDirty = true;
}

void Shader::setUniformVec2(const std::string& name, const glm::vec2& value)
{
  UniformData data;
  data.type = UniformData::Type::Vec2;
  data.vec2Value = value;
  uniformValues[name] = data;
  uniformsDirty = true;
}

void Shader::setUniformFloat(const std::string& name, float value)
{
  UniformData data;
  data.type = UniformData::Type::Float;
  data.floatValue = value;
  uniformValues[name] = data;
  uniformsDirty = true;
}

void Shader::setUniformInt(const std::string& name, int value)
{
  UniformData data;
  data.type = UniformData::Type::Int;
  data.intValue = value;
  uniformValues[name] = data;
  uniformsDirty = true;
}

void Shader::setUniformMat4(const std::string& name, const glm::mat4& value)
{
  UniformData data;
  data.type = UniformData::Type::Mat4;
  data.mat4Value = value;
  uniformValues[name] = data;
  uniformsDirty = true;
}

void Shader::updateUniformBuffers(VulkanContext& context)
{
  if (!uniformsDirty || uniformBuffer == VK_NULL_HANDLE) {
    return;
  }

  // Map memory
  void* data;
  VkDevice device = context.getDevice();
  // Use actual size of UBO struct for mapping
  vkMapMemory(
    device, uniformBufferMemory, 0, sizeof(UniformBufferObject), 0, &data);

  // Create a local UBO struct and populate it from the map
  UniformBufferObject ubo {};
  if (uniformValues.count("projection"))
    ubo.projection = uniformValues["projection"].mat4Value;
  if (uniformValues.count("textColor"))
    ubo.textColor = uniformValues["textColor"].vec4Value;
  if (uniformValues.count("flashIntensity"))
    ubo.flashIntensity = uniformValues["flashIntensity"].floatValue;

  // Copy the whole struct to the mapped buffer
  memcpy(data, &ubo, sizeof(ubo));

  // Unmap memory
  vkUnmapMemory(device, uniformBufferMemory);
  uniformsDirty = false;
}

VkShaderModule Shader::createShaderModule(VulkanContext& context,
                                          const std::vector<char>& code)
{
  VkShaderModuleCreateInfo createInfo {};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

  VkShaderModule shaderModule;
  if (vkCreateShaderModule(
        context.getDevice(), &createInfo, nullptr, &shaderModule) !=
      VK_SUCCESS) {
    std::cerr << "ERROR: Failed to create shader module." << std::endl;
    return VK_NULL_HANDLE;
  }

  return shaderModule;
}

std::vector<char> Shader::readFile(const std::string& filename)
{
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    std::cerr << "ERROR: Failed to open shader file: " << filename << std::endl;
    return {};
  }

  size_t fileSize = (size_t)file.tellg();
  std::vector<char> buffer(fileSize);

  file.seekg(0);
  file.read(buffer.data(), fileSize);

  file.close();

  return buffer;
}

// Placeholder implementation for finding memory type
uint32_t Shader::findMemoryType(VulkanContext& context,
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

bool Shader::createDescriptorSetLayout(VulkanContext& context)
{
  // Define layout for UBO (Set 0, Binding 0)
  VkDescriptorSetLayoutBinding uboLayoutBinding {};
  uboLayoutBinding.binding = 0;
  uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uboLayoutBinding.descriptorCount = 1;
  uboLayoutBinding.stageFlags =
    VK_SHADER_STAGE_VERTEX_BIT;  // UBO primarily used in Vertex shader
  uboLayoutBinding.pImmutableSamplers = nullptr;

  VkDescriptorSetLayoutCreateInfo layoutInfo {};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = 1;
  layoutInfo.pBindings = &uboLayoutBinding;

  if (vkCreateDescriptorSetLayout(
        context.getDevice(), &layoutInfo, nullptr, &descriptorSetLayout) !=
      VK_SUCCESS) {
    std::cerr << "ERROR: Failed to create UBO descriptor set layout."
              << std::endl;
    return false;
  }

  return true;
}

bool Shader::createUniformBuffers(VulkanContext& context)
{
  // Set uniform buffer size to match the UBO struct
  uniformBufferSize = sizeof(UniformBufferObject);

  VkBufferCreateInfo bufferInfo {};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = uniformBufferSize;  // Use correct size
  bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(
        context.getDevice(), &bufferInfo, nullptr, &uniformBuffer) !=
      VK_SUCCESS) {
    std::cerr << "ERROR: Failed to create uniform buffer." << std::endl;
    return false;
  }

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(
    context.getDevice(), uniformBuffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo {};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = findMemoryType(
    context,
    memRequirements.memoryTypeBits,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  if (vkAllocateMemory(
        context.getDevice(), &allocInfo, nullptr, &uniformBufferMemory) !=
      VK_SUCCESS) {
    std::cerr << "ERROR: Failed to allocate uniform buffer memory."
              << std::endl;
    return false;
  }

  vkBindBufferMemory(
    context.getDevice(), uniformBuffer, uniformBufferMemory, 0);

  return true;
}

bool Shader::createDescriptorPool(VulkanContext& context)
{
  VkDescriptorPoolSize poolSize {};
  poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSize.descriptorCount = 1;

  VkDescriptorPoolCreateInfo poolInfo {};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = 1;
  poolInfo.pPoolSizes = &poolSize;
  poolInfo.maxSets = 1;  // Only one set (for UBO) needed from this pool

  // Create the pool and store the handle in the member variable
  if (vkCreateDescriptorPool(
        context.getDevice(), &poolInfo, nullptr, &uboDescriptorPool) !=
      VK_SUCCESS) {  // Store in member
    std::cerr << "ERROR: Failed to create UBO descriptor pool." << std::endl;
    return false;
  }

  // Don't allocate set here, move to createDescriptorSets
  return true;
}

bool Shader::createDescriptorSets(VulkanContext& context)
{
  // Allocate the UBO descriptor set from the member pool
  VkDescriptorSetAllocateInfo allocInfo {};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = uboDescriptorPool;  // Use member pool
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = &descriptorSetLayout;

  if (vkAllocateDescriptorSets(
        context.getDevice(), &allocInfo, &descriptorSet) !=
      VK_SUCCESS) {  // Use member descriptorSet
    std::cerr << "ERROR: Failed to allocate UBO descriptor set." << std::endl;
    // Pool will be cleaned up in Shader::cleanup
    return false;
  }

  // Update descriptor sets with buffer info
  VkDescriptorBufferInfo bufferInfo {};
  bufferInfo.buffer = uniformBuffer;
  bufferInfo.offset = 0;
  bufferInfo.range = uniformBufferSize;

  VkWriteDescriptorSet descriptorWrite {};
  descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrite.dstSet = descriptorSet;  // Use member descriptorSet
  descriptorWrite.dstBinding = 0;
  descriptorWrite.dstArrayElement = 0;
  descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  descriptorWrite.descriptorCount = 1;
  descriptorWrite.pBufferInfo = &bufferInfo;

  vkUpdateDescriptorSets(context.getDevice(), 1, &descriptorWrite, 0, nullptr);

  return true;
}

bool Shader::createPipeline(VulkanContext& context,
                            VkDescriptorSetLayout externalLayout)
{
  // Basic pipeline implementation
  VkPipelineShaderStageCreateInfo shaderStages[2];

  shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  shaderStages[0].module = vertShaderModule;
  shaderStages[0].pName = "main";
  shaderStages[0].flags = 0;
  shaderStages[0].pNext = nullptr;
  shaderStages[0].pSpecializationInfo = nullptr;

  shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  shaderStages[1].module = fragShaderModule;
  shaderStages[1].pName = "main";
  shaderStages[1].flags = 0;
  shaderStages[1].pNext = nullptr;
  shaderStages[1].pSpecializationInfo = nullptr;

  // == Vertex Input State ==
  VkVertexInputBindingDescription bindingDescription {};
  bindingDescription.binding = 0;  // We use one binding for all vertex data
  bindingDescription.stride =
    sizeof(TextVertex);  // Matches the struct in TextRenderer.cpp
  bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions {};
  // Position attribute (location = 0)
  attributeDescriptions[0].binding = 0;
  attributeDescriptions[0].location = 0;
  attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;  // vec2
  attributeDescriptions[0].offset = offsetof(TextVertex, pos);
  // TexCoord attribute (location = 1)
  attributeDescriptions[1].binding = 0;
  attributeDescriptions[1].location = 1;
  attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;  // vec2
  attributeDescriptions[1].offset = offsetof(TextVertex, texCoord);
  // Color and Flash Intensity now come from UBO

  VkPipelineVertexInputStateCreateInfo vertexInputInfo {};
  vertexInputInfo.sType =
    VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
  // Only 2 attributes (pos, texCoord)
  vertexInputInfo.vertexAttributeDescriptionCount = 2;
  vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

  VkPipelineInputAssemblyStateCreateInfo inputAssembly {};
  inputAssembly.sType =
    VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  // Viewport and Scissor (Dynamic state)
  VkPipelineViewportStateCreateInfo viewportState {};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.scissorCount = 1;

  VkPipelineRasterizationStateCreateInfo rasterizer {};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = VK_CULL_MODE_NONE;  // No culling for 2D
  rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;

  VkPipelineMultisampleStateCreateInfo multisampling {};
  multisampling.sType =
    VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineColorBlendAttachmentState colorBlendAttachment {};
  colorBlendAttachment.colorWriteMask =
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
    VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_TRUE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  colorBlendAttachment.dstColorBlendFactor =
    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

  VkPipelineColorBlendStateCreateInfo colorBlending {};
  colorBlending.sType =
    VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;

  VkPipelineDynamicStateCreateInfo dynamicState {};
  std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT,
                                                VK_DYNAMIC_STATE_SCISSOR };
  dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
  dynamicState.pDynamicStates = dynamicStates.data();

  // Combine layouts: UBO (Set 0) and optionally Sampler (Set 1)
  std::vector<VkDescriptorSetLayout> setLayouts;
  setLayouts.push_back(descriptorSetLayout);  // UBO layout is always set 0
  if (externalLayout !=
      VK_NULL_HANDLE) {  // Check if external layout was provided
    setLayouts.push_back(externalLayout);  // Add sampler layout as set 1
  }

  VkPipelineLayoutCreateInfo pipelineLayoutInfo {};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
  pipelineLayoutInfo.pSetLayouts = setLayouts.data();

  if (vkCreatePipelineLayout(
        context.getDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) !=
      VK_SUCCESS) {
    std::cerr << "ERROR: Failed to create pipeline layout."
              << std::endl;  // Generic error message
    return false;
  }

  // --- Actually create the pipeline ---
  VkGraphicsPipelineCreateInfo pipelineInfo {};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pDepthStencilState = nullptr;  // No depth testing for 2D text
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = &dynamicState;
  pipelineInfo.layout = pipelineLayout;
  pipelineInfo.renderPass = context.getRenderPass();
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

  if (vkCreateGraphicsPipelines(context.getDevice(),
                                VK_NULL_HANDLE,
                                1,
                                &pipelineInfo,
                                nullptr,
                                &graphicsPipeline) != VK_SUCCESS) {
    std::cerr << "ERROR: Failed to create graphics pipeline." << std::endl;
    // graphicsPipeline will be VK_NULL_HANDLE if creation failed
    return false;
  }

  // Pipeline created successfully
  std::cout << "Graphics pipeline created successfully."
            << std::endl;  // Added success message
  return true;
}

}  // namespace tewduwu