#pragma once

#include <glm/glm.hpp>
#include <memory>

namespace tewduwu {

class VulkanContext;
class Shader;
class Texture;

class GlassPanel {
public:
    GlassPanel();
    ~GlassPanel();
    
    bool initialize(VulkanContext& context);
    void render(VulkanContext& context, const glm::vec4& bounds, float opacity = 0.9f, float blurRadius = 10.0f);
    
    // Visual properties
    void setGlowColor(const glm::vec4& color);
    void setGlowIntensity(float intensity);
    void setEdgeThickness(float thickness);
    void setEdgeColor(const glm::vec4& color);
    
    // Animation
    void setAnimationProgress(float progress); // 0.0 to 1.0
    
private:
    std::shared_ptr<Shader> blurShader;
    std::shared_ptr<Shader> edgeShader;
    std::shared_ptr<Shader> glowShader;
    
    // Appearance properties
    glm::vec4 glowColor;
    float glowIntensity;
    float edgeThickness;
    glm::vec4 edgeColor;
    float animationProgress;
};

} // namespace tewduwu