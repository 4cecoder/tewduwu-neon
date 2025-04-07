#pragma once

#include <memory>
#include <glm/glm.hpp>
#include "../core/TodoList.h"

namespace tewduwu {

class VulkanContext;
class GlassPanel;
class TextRenderer;

class TaskListWidget {
public:
    TaskListWidget();
    ~TaskListWidget();
    
    bool initialize(VulkanContext& context, std::shared_ptr<TodoList> todoList);
    void render(VulkanContext& context, const glm::vec4& bounds);
    
    // Input handling
    bool handleKeyInput(int keyCode);
    
    // Appearance
    void setPrimaryColor(const glm::vec4& color);
    void setSecondaryColor(const glm::vec4& color);
    void setAccentColor(const glm::vec4& color);
    void setBackgroundColor(const glm::vec4& color);
    void setTextColor(const glm::vec4& color);
    
    // Animation and updates
    void update(float deltaTime);
    
private:
    std::shared_ptr<TodoList> todoList;
    std::shared_ptr<GlassPanel> glassPanel;
    std::shared_ptr<TextRenderer> textRenderer;
    
    // Rendering/layout helpers
    void renderTaskItem(VulkanContext& context, std::shared_ptr<TodoItem> item, 
                        const glm::vec4& bounds, size_t index);
    
    // Appearance
    glm::vec4 primaryColor;
    glm::vec4 secondaryColor;
    glm::vec4 accentColor;
    glm::vec4 backgroundColor;
    glm::vec4 textColor;
    
    // Scrolling state
    float scrollOffset;
    float targetScrollOffset;
    float scrollVelocity;
    
    // Animation
    float hoverEffectIntensity;
};

} // namespace tewduwu