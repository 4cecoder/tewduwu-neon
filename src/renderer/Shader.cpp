#include "Shader.h"
#include "VulkanContext.h"
#include <fstream>
#include <vector>
#include <iostream>

namespace tewduwu {

Shader::Shader() : pipelineLayout(VK_NULL_HANDLE), graphicsPipeline(VK_NULL_HANDLE), 
                   vertShaderModule(VK_NULL_HANDLE), fragShaderModule(VK_NULL_HANDLE),
                   uniformBuffer(VK_NULL_HANDLE), uniformBufferMemory(VK_NULL_HANDLE),
                   descriptorSet(VK_NULL_HANDLE), descriptorSetLayout(VK_NULL_HANDLE),
                   uniformBufferSize(0), uniformsDirty(false) {
}

Shader::~Shader() {
    // Note: Actual cleanup should happen in cleanup()
}

bool Shader::initialize(VulkanContext& context, const std::string& vertPath, const std::string& fragPath) {
    // Read shader files
    auto vertShaderCode = readFile(vertPath);
    auto fragShaderCode = readFile(fragPath);
    
    if (vertShaderCode.empty() || fragShaderCode.empty()) {
        std::cerr << "ERROR: Failed to read shader files: " << vertPath << " or " << fragPath << std::endl;
        return false;
    }
    
    // Create shader modules
    vertShaderModule = createShaderModule(context, vertShaderCode);
    fragShaderModule = createShaderModule(context, fragShaderCode);
    
    if (vertShaderModule == VK_NULL_HANDLE || fragShaderModule == VK_NULL_HANDLE) {
        std::cerr << "ERROR: Failed to create shader modules." << std::endl;
        return false;
    }
    
    // Create descriptor layout, uniform buffers, and pipeline
    if (!createDescriptorSetLayout(context) ||
        !createUniformBuffers(context) ||
        !createDescriptorPool(context) ||
        !createDescriptorSets(context) ||
        !createPipeline(context)) {
        return false;
    }
    
    return true;
}

void Shader::cleanup(VulkanContext& context) {
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

void Shader::bind(VulkanContext& context, VkCommandBuffer cmdBuffer) {
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
}

void Shader::setUniformVec4(const std::string& name, const glm::vec4& value) {
    UniformData data;
    data.type = UniformData::Type::Vec4;
    data.vec4Value = value;
    uniformValues[name] = data;
    uniformsDirty = true;
}

void Shader::setUniformVec3(const std::string& name, const glm::vec3& value) {
    UniformData data;
    data.type = UniformData::Type::Vec3;
    data.vec3Value = value;
    uniformValues[name] = data;
    uniformsDirty = true;
}

void Shader::setUniformVec2(const std::string& name, const glm::vec2& value) {
    UniformData data;
    data.type = UniformData::Type::Vec2;
    data.vec2Value = value;
    uniformValues[name] = data;
    uniformsDirty = true;
}

void Shader::setUniformFloat(const std::string& name, float value) {
    UniformData data;
    data.type = UniformData::Type::Float;
    data.floatValue = value;
    uniformValues[name] = data;
    uniformsDirty = true;
}

void Shader::setUniformInt(const std::string& name, int value) {
    UniformData data;
    data.type = UniformData::Type::Int;
    data.intValue = value;
    uniformValues[name] = data;
    uniformsDirty = true;
}

void Shader::setUniformMat4(const std::string& name, const glm::mat4& value) {
    UniformData data;
    data.type = UniformData::Type::Mat4;
    data.mat4Value = value;
    uniformValues[name] = data;
    uniformsDirty = true;
}

void Shader::updateUniformBuffers(VulkanContext& context) {
    if (!uniformsDirty || uniformBuffer == VK_NULL_HANDLE) {
        return;
    }
    
    // Map memory
    void* data;
    VkDevice device = context.getDevice();
    vkMapMemory(device, uniformBufferMemory, 0, uniformBufferSize, 0, &data);
    
    // Copy uniform values to mapped memory
    char* mappedData = static_cast<char*>(data);
    size_t offset = 0;
    
    for (const auto& pair : uniformValues) {
        const auto& uniformData = pair.second;
        
        switch (uniformData.type) {
            case UniformData::Type::Float:
                *reinterpret_cast<float*>(mappedData + offset) = uniformData.floatValue;
                offset += sizeof(float);
                break;
            case UniformData::Type::Int:
                *reinterpret_cast<int*>(mappedData + offset) = uniformData.intValue;
                offset += sizeof(int);
                break;
            case UniformData::Type::Vec2:
                *reinterpret_cast<glm::vec2*>(mappedData + offset) = uniformData.vec2Value;
                offset += sizeof(glm::vec2);
                break;
            case UniformData::Type::Vec3:
                *reinterpret_cast<glm::vec3*>(mappedData + offset) = uniformData.vec3Value;
                offset += sizeof(glm::vec3);
                break;
            case UniformData::Type::Vec4:
                *reinterpret_cast<glm::vec4*>(mappedData + offset) = uniformData.vec4Value;
                offset += sizeof(glm::vec4);
                break;
            case UniformData::Type::Mat4:
                *reinterpret_cast<glm::mat4*>(mappedData + offset) = uniformData.mat4Value;
                offset += sizeof(glm::mat4);
                break;
        }
    }
    
    // Unmap memory
    vkUnmapMemory(device, uniformBufferMemory);
    uniformsDirty = false;
}

VkShaderModule Shader::createShaderModule(VulkanContext& context, const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
    
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(context.getDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        std::cerr << "ERROR: Failed to create shader module." << std::endl;
        return VK_NULL_HANDLE;
    }
    
    return shaderModule;
}

std::vector<char> Shader::readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        std::cerr << "ERROR: Failed to open shader file: " << filename << std::endl;
        return {};
    }

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}

// Placeholder implementation for finding memory type
uint32_t Shader::findMemoryType(VulkanContext& context, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
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

bool Shader::createDescriptorSetLayout(VulkanContext& context) {
    // Basic implementation
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboLayoutBinding;
    
    if (vkCreateDescriptorSetLayout(context.getDevice(), &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        std::cerr << "ERROR: Failed to create descriptor set layout." << std::endl;
        return false;
    }
    
    return true;
}

bool Shader::createUniformBuffers(VulkanContext& context) {
    // Simplified implementation
    uniformBufferSize = 1024; // 1KB buffer for uniform data
    
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = uniformBufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateBuffer(context.getDevice(), &bufferInfo, nullptr, &uniformBuffer) != VK_SUCCESS) {
        std::cerr << "ERROR: Failed to create uniform buffer." << std::endl;
        return false;
    }
    
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(context.getDevice(), uniformBuffer, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(context, memRequirements.memoryTypeBits, 
                                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                                             VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    if (vkAllocateMemory(context.getDevice(), &allocInfo, nullptr, &uniformBufferMemory) != VK_SUCCESS) {
        std::cerr << "ERROR: Failed to allocate uniform buffer memory." << std::endl;
        return false;
    }
    
    vkBindBufferMemory(context.getDevice(), uniformBuffer, uniformBufferMemory, 0);
    
    return true;
}

bool Shader::createDescriptorPool(VulkanContext& context) {
    // Simple descriptor pool implementation
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = 1;
    
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 1;
    
    // For now, we'll use a local descriptor pool
    VkDescriptorPool descriptorPool;
    if (vkCreateDescriptorPool(context.getDevice(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        std::cerr << "ERROR: Failed to create descriptor pool." << std::endl;
        return false;
    }
    
    // Create descriptor sets
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout;
    
    if (vkAllocateDescriptorSets(context.getDevice(), &allocInfo, &descriptorSet) != VK_SUCCESS) {
        std::cerr << "ERROR: Failed to allocate descriptor sets." << std::endl;
        vkDestroyDescriptorPool(context.getDevice(), descriptorPool, nullptr);
        return false;
    }
    
    return true;
}

bool Shader::createDescriptorSets(VulkanContext& context) {
    // Update descriptor sets with buffer info
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = uniformBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = uniformBufferSize;
    
    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;
    
    vkUpdateDescriptorSets(context.getDevice(), 1, &descriptorWrite, 0, nullptr);
    
    return true;
}

bool Shader::createPipeline(VulkanContext& context) {
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
    
    // Simplified pipeline creation with minimal configuration
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
    
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
    
    if (vkCreatePipelineLayout(context.getDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        std::cerr << "ERROR: Failed to create pipeline layout." << std::endl;
        return false;
    }
    
    // For now, we'll return true without creating the actual pipeline
    // This will need to be completed with viewport, rasterization, and other pipeline stages
    std::cout << "Pipeline creation simplified for initial implementation." << std::endl;
    graphicsPipeline = VK_NULL_HANDLE; // Placeholder
    
    return true;
}

} // namespace tewduwu