#pragma once

#include "TodoItem.h"
#include <vector>
#include <memory>
#include <string>
#include <functional>

namespace tewduwu {

class TodoList {
public:
    TodoList();
    ~TodoList();
    
    // Item management
    void addItem(const std::string& text, int nestLevel = 0, Priority priority = Priority::NONE);
    void removeItem(size_t index);
    void toggleItem(size_t index);
    void moveItemUp(size_t index);
    void moveItemDown(size_t index);
    void changePriority(size_t index, Priority newPriority);
    
    // Navigation
    bool selectPrevious();
    bool selectNext();
    bool selectParent();
    bool selectFirstChild();
    
    // Access
    size_t getSelectedIndex() const;
    size_t getItemCount() const;
    std::shared_ptr<TodoItem> getItem(size_t index);
    
    // Persistence
    bool saveToFile(const std::string& filepath);
    bool loadFromFile(const std::string& filepath);
    
    // Update for animations
    void update(float deltaTime);
    
private:
    std::vector<std::shared_ptr<TodoItem>> items;
    size_t selectedIndex;
};

} // namespace tewduwu