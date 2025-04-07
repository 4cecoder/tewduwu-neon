#include "TaskListWidget.h"
#include "GlassPanel.h"
#include "../renderer/TextRenderer.h"
#include "../renderer/VulkanContext.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>

namespace tewduwu {

TaskListWidget::TaskListWidget()
    : primaryColor(1.0f, 0.255f, 0.639f, 1.0f)    // Neon Pink
    , secondaryColor(0.0f, 1.0f, 0.95f, 1.0f)     // Cyan
    , accentColor(0.678f, 0.361f, 1.0f, 1.0f)     // Purple
    , backgroundColor(0.039f, 0.039f, 0.078f, 1.0f) // Dark
    , textColor(0.95f, 0.95f, 1.0f, 1.0f)         // Bright
    , scrollOffset(0.0f)
    , targetScrollOffset(0.0f)
    , scrollVelocity(0.0f)
    , hoverEffectIntensity(0.0f)
{
}

TaskListWidget::~TaskListWidget() {
}

bool TaskListWidget::initialize(VulkanContext& context, std::shared_ptr<TodoList> todoList) {
    this->todoList = todoList;
    
    // Create glass panel
    glassPanel = std::make_shared<GlassPanel>();
    if (!glassPanel->initialize(context)) {
        return false;
    }
    
    // Set glass panel properties
    glassPanel->setGlowColor(primaryColor);
    glassPanel->setEdgeColor(primaryColor);
    glassPanel->setGlowIntensity(0.5f);
    glassPanel->setEdgeThickness(0.02f);
    
    // Create text renderer
    textRenderer = std::make_shared<TextRenderer>();
    if (!textRenderer->initialize(context)) {
        return false;
    }
    
    // Load font
    if (!textRenderer->loadFont("fonts/Inconsolata-Regular.ttf", 18)) {
        // Fallback to a system font
        if (!textRenderer->loadFont("/System/Library/Fonts/Menlo.ttc", 18)) {
            // Try another fallback
            if (!textRenderer->loadFont("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 18)) {
                return false;
            }
        }
    }
    
    return true;
}

void TaskListWidget::render(VulkanContext& context, const glm::vec4& bounds) {
    // Draw main panel
    glassPanel->render(context, bounds, 0.8f, 5.0f);
    
    // Calculate item dimensions
    float itemHeight = 30.0f;
    float itemPadding = 10.0f;
    float maxVisibleItems = std::floor(bounds.w / itemHeight);
    
    // Calculate visible range
    size_t startIdx = static_cast<size_t>(scrollOffset);
    size_t endIdx = std::min(todoList->getItemCount(), 
                           startIdx + static_cast<size_t>(maxVisibleItems));
    
    // Render visible items
    for (size_t i = startIdx; i < endIdx; i++) {
        auto item = todoList->getItem(i);
        if (!item) continue;
        
        // Calculate item bounds
        float itemY = bounds.y + (i - startIdx) * itemHeight;
        glm::vec4 itemBounds(
            bounds.x + itemPadding + item->nestLevel * 20.0f,
            itemY + itemPadding / 2,
            bounds.z - itemPadding * 2 - item->nestLevel * 20.0f,
            itemHeight - itemPadding
        );
        
        // Render item
        renderTaskItem(context, item, itemBounds, i);
    }
    
    // Render scrollbar if needed
    if (todoList->getItemCount() > maxVisibleItems) {
        float scrollbarWidth = 8.0f;
        float scrollbarHeight = bounds.w * (maxVisibleItems / todoList->getItemCount());
        float scrollbarY = bounds.y + (scrollOffset / todoList->getItemCount()) * bounds.w;
        
        glm::vec4 scrollbarBounds(
            bounds.x + bounds.z - scrollbarWidth - 5.0f,
            scrollbarY,
            scrollbarWidth,
            scrollbarHeight
        );
        
        // Draw scrollbar (semi-transparent panel)
        glassPanel->setGlowIntensity(0.2f);
        glassPanel->render(context, scrollbarBounds, 0.5f, 0.0f);
        glassPanel->setGlowIntensity(0.5f);
    }
}

void TaskListWidget::renderTaskItem(VulkanContext& context, 
                                   std::shared_ptr<TodoItem> item, 
                                   const glm::vec4& bounds,
                                   size_t index) {
    // Determine colors based on state
    glm::vec4 itemColor;
    if (item->completed) {
        itemColor = secondaryColor;
        itemColor.a = 0.5f; // More transparent for completed items
    } else {
        switch (item->priority) {
            case Priority::HIGH:
                itemColor = primaryColor;
                break;
            case Priority::MEDIUM:
                itemColor = accentColor;
                break;
            case Priority::LOW:
                itemColor = secondaryColor;
                break;
            default:
                itemColor = textColor;
                itemColor.a = 0.7f;
                break;
        }
    }
    
    // Highlight selected item
    bool isSelected = (index == todoList->getSelectedIndex());
    if (isSelected) {
        // Draw selection indicator
        glm::vec4 selectionBounds(
            bounds.x - 4.0f,
            bounds.y - 2.0f,
            bounds.z + 8.0f,
            bounds.w + 4.0f
        );
        
        // Use a brighter glow for selected item
        glassPanel->setGlowColor(itemColor);
        glassPanel->setGlowIntensity(0.7f + 0.3f * std::sin(item->animationProgress * 6.28318f));
        glassPanel->render(context, selectionBounds, 0.6f, 3.0f);
        glassPanel->setGlowIntensity(0.5f);
        glassPanel->setGlowColor(primaryColor);
    }
    
    // Draw item background (more transparent for unselected items)
    glassPanel->setGlowColor(itemColor);
    glassPanel->setGlowIntensity(isSelected ? 0.5f : 0.2f);
    glassPanel->render(context, bounds, isSelected ? 0.7f : 0.5f, isSelected ? 2.0f : 1.0f);
    glassPanel->setGlowColor(primaryColor);
    glassPanel->setGlowIntensity(0.5f);
    
    // Prepare text with checkbox and priority indicator
    std::string displayText;
    
    // Add checkbox
    displayText += item->completed ? "[x] " : "[ ] ";
    
    // Add priority indicator
    if (!item->completed) {
        switch (item->priority) {
            case Priority::HIGH:
                displayText += "[H] ";
                break;
            case Priority::MEDIUM:
                displayText += "[M] ";
                break;
            case Priority::LOW:
                displayText += "[L] ";
                break;
            default:
                // No priority indicator
                break;
        }
    }
    
    // Add item text
    displayText += item->text;
    
    // Draw text
    glm::vec4 textColor = this->textColor;
    if (item->completed) {
        textColor.a = 0.6f; // Dimmed text for completed items
    }
    
    // TODO: Render text with TextRenderer
    // For now this is just a placeholder
    // textRenderer->renderText(context, displayText, bounds.x + 5.0f, bounds.y + bounds.w/2, 1.0f, textColor);
}

bool TaskListWidget::handleKeyInput(int keyCode) {
    if (!todoList) return false;
    
    switch (keyCode) {
        case SDLK_UP:
        case SDLK_K:
            return todoList->selectPrevious();
            
        case SDLK_DOWN:
        case SDLK_J:
            return todoList->selectNext();
            
        case SDLK_H:
            return todoList->selectParent();
            
        case SDLK_L:
            return todoList->selectFirstChild();
            
        case SDLK_SPACE:
            todoList->toggleItem(todoList->getSelectedIndex());
            return true;
            
        case SDLK_D:
            todoList->removeItem(todoList->getSelectedIndex());
            return true;
            
        case SDLK_1:
            todoList->changePriority(todoList->getSelectedIndex(), Priority::LOW);
            return true;
            
        case SDLK_2:
            todoList->changePriority(todoList->getSelectedIndex(), Priority::MEDIUM);
            return true;
            
        case SDLK_3:
            todoList->changePriority(todoList->getSelectedIndex(), Priority::HIGH);
            return true;
            
        case SDLK_0:
            todoList->changePriority(todoList->getSelectedIndex(), Priority::NONE);
            return true;
            
        case SDLK_A: {
            // Add item - this would need user input
            // For now, add a placeholder item
            todoList->addItem("New Task", 0, Priority::NONE);
            todoList->selectNext();
            return true;
        }
        
        case SDLK_I: {
            // Add subtask - this would need user input
            auto currentIdx = todoList->getSelectedIndex();
            auto currentItem = todoList->getItem(currentIdx);
            if (currentItem) {
                int nestLevel = currentItem->nestLevel + 1;
                todoList->addItem("New Subtask", nestLevel, Priority::NONE);
                todoList->selectNext();
                return true;
            }
            return false;
        }
    }
    
    return false;
}

void TaskListWidget::setPrimaryColor(const glm::vec4& color) {
    primaryColor = color;
    if (glassPanel) {
        glassPanel->setGlowColor(color);
        glassPanel->setEdgeColor(color);
    }
}

void TaskListWidget::setSecondaryColor(const glm::vec4& color) {
    secondaryColor = color;
}

void TaskListWidget::setAccentColor(const glm::vec4& color) {
    accentColor = color;
}

void TaskListWidget::setBackgroundColor(const glm::vec4& color) {
    backgroundColor = color;
}

void TaskListWidget::setTextColor(const glm::vec4& color) {
    textColor = color;
}

void TaskListWidget::update(float deltaTime) {
    // Update target scroll position based on selected item
    if (todoList) {
        size_t selectedIndex = todoList->getSelectedIndex();
        targetScrollOffset = std::max(0.0f, static_cast<float>(selectedIndex) - 5.0f);
        
        // Smooth scrolling
        float scrollDiff = targetScrollOffset - scrollOffset;
        
        // Apply spring physics for smooth scrolling
        float spring = 8.0f;
        float damping = 0.8f;
        
        scrollVelocity += scrollDiff * spring * deltaTime;
        scrollVelocity *= damping;
        
        scrollOffset += scrollVelocity * deltaTime;
        
        // Update glass panel animation
        if (glassPanel) {
            // Animation progress seems handled per-item in renderTaskItem
        }
        
        // Update items
        todoList->update(deltaTime);
    }
}

} // namespace tewduwu