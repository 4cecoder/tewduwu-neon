#pragma once

#include <string>
#include <unordered_map>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

namespace tewduwu {

class VulkanContext;

class Shader {
public:
    Shader();
    ~Shader();
    
    bool initialize(VulkanContext& context, const std::string& vertPath, const std::string& fragPath);
    void cleanup(VulkanContext& context);
    
    // Bind shader for rendering
    void bind(VulkanContext& context, VkCommandBuffer cmdBuffer);
    
    // Pipeline management
    VkPipeline getPipeline() const { return graphicsPipeline; }
    VkPipelineLayout getPipelineLayout() const { return pipelineLayout; }
    
    // Set uniform values
    void setUniformVec4(const std::string& name, const glm::vec4& value);
    void setUniformVec3(const std::string& name, const glm::vec3& value);
    void setUniformVec2(const std::string& name, const glm::vec2& value);
    void setUniformFloat(const std::string& name, float value);
    void setUniformInt(const std::string& name, int value);
    void setUniformMat4(const std::string& name, const glm::mat4& value);
    
    // Update uniform buffers (called before rendering)
    void updateUniformBuffers(VulkanContext& context);
    
private:
    // Pipeline objects
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    
    // Shader modules
    VkShaderModule vertShaderModule;
    VkShaderModule fragShaderModule;
    
    // Uniform buffers
    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBufferMemory;
    VkDescriptorSet descriptorSet;
    VkDescriptorSetLayout descriptorSetLayout;
    size_t uniformBufferSize;
    
    // Uniform data storage
    struct UniformData {
        enum class Type { Float, Int, Vec2, Vec3, Vec4, Mat4 };
        Type type;
        union {
            float floatValue;
            int intValue;
            glm::vec2 vec2Value;
            glm::vec3 vec3Value;
            glm::vec4 vec4Value;
            glm::mat4 mat4Value;
        };
    };
    
    std::unordered_map<std::string, UniformData> uniformValues;
    bool uniformsDirty;
    
    // Helper methods
    VkShaderModule createShaderModule(VulkanContext& context, const std::vector<char>& code);
    bool createDescriptorSetLayout(VulkanContext& context);
    bool createUniformBuffers(VulkanContext& context);
    bool createDescriptorPool(VulkanContext& context);
    bool createDescriptorSets(VulkanContext& context);
    bool createPipeline(VulkanContext& context);
    
    // Read shader file
    static std::vector<char> readFile(const std::string& filename);
};

} // namespace tewduwu