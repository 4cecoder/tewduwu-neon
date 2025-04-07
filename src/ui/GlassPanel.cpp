#include "GlassPanel.h"
#include "../renderer/VulkanContext.h"
#include "../renderer/Shader.h"
#include <glm/gtc/matrix_transform.hpp>

namespace tewduwu {

GlassPanel::GlassPanel()
    : glowColor(1.0f, 0.255f, 0.639f, 1.0f)  // Neon pink
    , glowIntensity(0.5f)
    , edgeThickness(0.02f)
    , edgeColor(1.0f, 0.255f, 0.639f, 1.0f)  // Neon pink
    , animationProgress(0.0f)
{
}

GlassPanel::~GlassPanel() {
}

bool GlassPanel::initialize(VulkanContext& context) {
    // Create shaders
    blurShader = std::make_shared<Shader>();
    if (!blurShader->initialize(context, "shaders/glass.vert.spv", "shaders/glass.frag.spv")) {
        return false;
    }
    
    // Set default uniform values
    blurShader->setUniformFloat("blurRadius", 10.0f);
    blurShader->setUniformFloat("opacity", 0.9f);
    blurShader->setUniformVec4("edgeColor", edgeColor);
    blurShader->setUniformFloat("edgeThickness", edgeThickness);
    blurShader->setUniformVec4("glowColor", glowColor);
    blurShader->setUniformFloat("glowIntensity", glowIntensity);
    blurShader->setUniformFloat("animationProgress", animationProgress);
    
    return true;
}

void GlassPanel::render(VulkanContext& context, const glm::vec4& bounds, float opacity, float blurRadius) {
    // Set up vertex data for the panel (positions and texture coordinates)
    struct Vertex {
        glm::vec2 position;
        glm::vec2 texCoord;
        glm::vec4 color;
    };
    
    // Bounds = x, y, width, height
    float x = bounds.x;
    float y = bounds.y;
    float width = bounds.z;
    float height = bounds.w;
    
    // Define the four corners of the panel
    Vertex vertices[6] = {
        // Position              // TexCoord      // Color
        {{x, y},                 {0.0f, 0.0f},    {1.0f, 1.0f, 1.0f, opacity}},  // Top-left
        {{x + width, y},         {1.0f, 0.0f},    {1.0f, 1.0f, 1.0f, opacity}},  // Top-right
        {{x, y + height},        {0.0f, 1.0f},    {1.0f, 1.0f, 1.0f, opacity}},  // Bottom-left
        
        {{x + width, y},         {1.0f, 0.0f},    {1.0f, 1.0f, 1.0f, opacity}},  // Top-right
        {{x + width, y + height},{1.0f, 1.0f},    {1.0f, 1.0f, 1.0f, opacity}},  // Bottom-right
        {{x, y + height},        {0.0f, 1.0f},    {1.0f, 1.0f, 1.0f, opacity}}   // Bottom-left
    };
    
    // TODO: Create and update vertex buffer
    // For now, this is a placeholder for actual rendering
    
    // Update uniform values
    blurShader->setUniformFloat("blurRadius", blurRadius);
    blurShader->setUniformFloat("opacity", opacity);
    blurShader->setUniformVec4("edgeColor", edgeColor);
    blurShader->setUniformFloat("edgeThickness", edgeThickness);
    blurShader->setUniformVec4("glowColor", glowColor);
    blurShader->setUniformFloat("glowIntensity", glowIntensity);
    blurShader->setUniformFloat("animationProgress", animationProgress);
    
    // Model-View-Projection matrix
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 proj = glm::ortho(0.0f, static_cast<float>(context.getSwapChainExtent().width),
                              static_cast<float>(context.getSwapChainExtent().height), 0.0f,
                              -1.0f, 1.0f);
    
    blurShader->setUniformMat4("model", model);
    blurShader->setUniformMat4("view", view);
    blurShader->setUniformMat4("proj", proj);
    
    // Update uniform buffers
    blurShader->updateUniformBuffers(context);
    
    // TODO: Bind shader, vertex buffer, and draw
}

void GlassPanel::setGlowColor(const glm::vec4& color) {
    glowColor = color;
}

void GlassPanel::setGlowIntensity(float intensity) {
    glowIntensity = intensity;
}

void GlassPanel::setEdgeThickness(float thickness) {
    edgeThickness = thickness;
}

void GlassPanel::setEdgeColor(const glm::vec4& color) {
    edgeColor = color;
}

void GlassPanel::setAnimationProgress(float progress) {
    animationProgress = progress;
}

} // namespace tewduwu