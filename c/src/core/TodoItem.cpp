#include "TodoItem.h"
#include <algorithm>
#include <cmath>

namespace tewduwu {

TodoItem::TodoItem(const std::string& text, int nestLevel, Priority priority)
    : text(text)
    , completed(false)
    , nestLevel(nestLevel)
    , priority(priority)
    , dueDate(0)
    , glowIntensity(0.0f)
    , animationProgress(0.0f)
{
}

void TodoItem::addChild(std::shared_ptr<TodoItem> child) {
    children.push_back(child);
}

const std::vector<std::shared_ptr<TodoItem>>& TodoItem::getChildren() const {
    return children;
}

void TodoItem::update(float deltaTime) {
    // Update animation
    animationProgress += deltaTime * 2.0f;
    if (animationProgress > 1.0f) {
        animationProgress = 0.0f;
    }
    
    // Update glow intensity based on priority
    float targetGlow = 0.0f;
    if (!completed) {
        switch (priority) {
            case Priority::HIGH:
                // Pulsing effect for high priority
                targetGlow = 0.7f + 0.3f * std::sin(animationProgress * 6.28318f);
                break;
            case Priority::MEDIUM:
                targetGlow = 0.5f;
                break;
            case Priority::LOW:
                targetGlow = 0.2f;
                break;
            default:
                targetGlow = 0.0f;
                break;
        }
    }
    
    // Smooth transition for glow effect
    glowIntensity = glowIntensity + (targetGlow - glowIntensity) * std::min(1.0f, deltaTime * 5.0f);
    
    // Update children
    for (auto& child : children) {
        child->update(deltaTime);
    }
}

} // namespace tewduwu