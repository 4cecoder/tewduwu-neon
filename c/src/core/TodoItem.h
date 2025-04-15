#pragma once

#include <string>
#include <vector>
#include <memory>
#include <ctime>

namespace tewduwu {

// Priority levels
enum class Priority {
    NONE = 0,
    LOW = 1,
    MEDIUM = 2,
    HIGH = 3
};

class TodoItem {
public:
    TodoItem(const std::string& text, int nestLevel = 0, Priority priority = Priority::NONE);
    
    // Core properties
    std::string text;
    bool completed;
    int nestLevel;
    Priority priority;
    time_t dueDate;
    
    // UI-specific properties
    float glowIntensity;
    float animationProgress;
    
    // Child management
    void addChild(std::shared_ptr<TodoItem> child);
    const std::vector<std::shared_ptr<TodoItem>>& getChildren() const;
    
    // Animation and update functions
    void update(float deltaTime);
    
private:
    std::vector<std::shared_ptr<TodoItem>> children;
};

} // namespace tewduwu