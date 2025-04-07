#include "TodoList.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <ctime>

namespace tewduwu {

TodoList::TodoList()
    : selectedIndex(0)
{
}

TodoList::~TodoList() {
    // Clean up is handled by shared_ptr
}

void TodoList::addItem(const std::string& text, int nestLevel, Priority priority) {
    auto item = std::make_shared<TodoItem>(text, nestLevel, priority);
    items.push_back(item);
}

void TodoList::removeItem(size_t index) {
    if (index < items.size()) {
        // Find all children of this item to remove them too
        int levelToRemove = items[index]->nestLevel;
        auto it = items.begin() + index;
        auto itEnd = it + 1;
        
        // Find all child items (they have higher nest level)
        while (itEnd != items.end() && (*itEnd)->nestLevel > levelToRemove) {
            ++itEnd;
        }
        
        // Erase the item and all its children
        items.erase(it, itEnd);
        
        // Adjust selected index if needed
        if (selectedIndex >= items.size()) {
            selectedIndex = items.empty() ? 0 : items.size() - 1;
        }
    }
}

void TodoList::toggleItem(size_t index) {
    if (index < items.size()) {
        items[index]->completed = !items[index]->completed;
    }
}

void TodoList::moveItemUp(size_t index) {
    if (index <= 0 || index >= items.size()) {
        return;
    }
    
    // Find the previous item at the same level
    int level = items[index]->nestLevel;
    int prevIndex = index - 1;
    while (prevIndex >= 0 && items[prevIndex]->nestLevel > level) {
        prevIndex--;
    }
    
    if (prevIndex < 0 || items[prevIndex]->nestLevel != level) {
        return; // No previous item at the same level
    }
    
    // Find the range of items to move (item and its children)
    int i = index + 1;
    while (i < items.size() && items[i]->nestLevel > level) {
        i++;
    }
    int numItemsToMove = i - index;
    
    // Store items to move
    std::vector<std::shared_ptr<TodoItem>> itemsToMove;
    itemsToMove.reserve(numItemsToMove);
    for (int j = 0; j < numItemsToMove; j++) {
        itemsToMove.push_back(items[index + j]);
    }
    
    // Find previous item's children to skip
    int j = prevIndex + 1;
    while (j < index && items[j]->nestLevel > items[prevIndex]->nestLevel) {
        j++;
    }
    int skipCount = j - prevIndex - 1;
    
    // Move items
    items.erase(items.begin() + index, items.begin() + index + numItemsToMove);
    items.insert(items.begin() + prevIndex + 1, itemsToMove.begin(), itemsToMove.end());
    
    // Update selected index
    selectedIndex = prevIndex + 1;
}

void TodoList::moveItemDown(size_t index) {
    if (index >= items.size() - 1) {
        return;
    }
    
    // Find the next item at the same level
    int level = items[index]->nestLevel;
    int nextIndex = index + 1;
    while (nextIndex < items.size() && items[nextIndex]->nestLevel > level) {
        nextIndex++;
    }
    
    if (nextIndex >= items.size() || items[nextIndex]->nestLevel != level) {
        return; // No next item at the same level
    }
    
    // Find the range of our items (item and its children)
    int currEndIdx = index + 1;
    while (currEndIdx < nextIndex && items[currEndIdx]->nestLevel > level) {
        currEndIdx++;
    }
    int numCurrItems = currEndIdx - index;
    
    // Find the range of next items (next item and its children)
    int nextEndIdx = nextIndex + 1;
    while (nextEndIdx < items.size() && items[nextEndIdx]->nestLevel > level) {
        nextEndIdx++;
    }
    int numNextItems = nextEndIdx - nextIndex;
    
    // Store next items
    std::vector<std::shared_ptr<TodoItem>> nextItems;
    nextItems.reserve(numNextItems);
    for (int i = 0; i < numNextItems; i++) {
        nextItems.push_back(items[nextIndex + i]);
    }
    
    // Move items
    items.erase(items.begin() + nextIndex, items.begin() + nextEndIdx);
    items.insert(items.begin() + index, nextItems.begin(), nextItems.end());
    
    // Update selected index
    selectedIndex = index + numNextItems;
}

void TodoList::changePriority(size_t index, Priority newPriority) {
    if (index < items.size()) {
        items[index]->priority = newPriority;
    }
}

bool TodoList::selectPrevious() {
    if (items.empty()) {
        return false;
    }
    
    if (selectedIndex > 0) {
        selectedIndex--;
        return true;
    } else {
        selectedIndex = items.size() - 1;
        return true;
    }
}

bool TodoList::selectNext() {
    if (items.empty()) {
        return false;
    }
    
    if (selectedIndex < items.size() - 1) {
        selectedIndex++;
        return true;
    } else {
        selectedIndex = 0;
        return true;
    }
}

bool TodoList::selectParent() {
    if (items.empty() || selectedIndex >= items.size()) {
        return false;
    }
    
    if (items[selectedIndex]->nestLevel > 0) {
        int level = items[selectedIndex]->nestLevel;
        for (int i = selectedIndex - 1; i >= 0; i--) {
            if (items[i]->nestLevel < level) {
                selectedIndex = i;
                return true;
            }
        }
    }
    
    return false;
}

bool TodoList::selectFirstChild() {
    if (items.empty() || selectedIndex >= items.size() - 1) {
        return false;
    }
    
    int currentLevel = items[selectedIndex]->nestLevel;
    if (items[selectedIndex + 1]->nestLevel > currentLevel) {
        selectedIndex++;
        return true;
    }
    
    return false;
}

size_t TodoList::getSelectedIndex() const {
    return selectedIndex;
}

size_t TodoList::getItemCount() const {
    return items.size();
}

std::shared_ptr<TodoItem> TodoList::getItem(size_t index) {
    if (index < items.size()) {
        return items[index];
    }
    return nullptr;
}

bool TodoList::saveToFile(const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        return false;
    }
    
    for (const auto& item : items) {
        file << item->completed << ","
             << item->nestLevel << ","
             << static_cast<int>(item->priority) << ","
             << item->dueDate << ","
             << item->text << "\n";
    }
    
    return true;
}

bool TodoList::loadFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return false;
    }
    
    items.clear();
    selectedIndex = 0;
    
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string token;
        
        // Parse completed flag
        bool completed = false;
        if (std::getline(iss, token, ',')) {
            completed = (token == "1");
        }
        
        // Parse nest level
        int nestLevel = 0;
        if (std::getline(iss, token, ',')) {
            nestLevel = std::stoi(token);
        }
        
        // Parse priority
        int priorityInt = 0;
        if (std::getline(iss, token, ',')) {
            priorityInt = std::stoi(token);
        }
        Priority priority = static_cast<Priority>(priorityInt);
        
        // Parse due date
        time_t dueDate = 0;
        if (std::getline(iss, token, ',')) {
            dueDate = std::stoll(token);
        }
        
        // Parse text
        std::string text;
        if (std::getline(iss, text)) {
            // Create item
            auto item = std::make_shared<TodoItem>(text, nestLevel, priority);
            item->completed = completed;
            item->dueDate = dueDate;
            items.push_back(item);
        }
    }
    
    return true;
}

void TodoList::update(float deltaTime) {
    for (auto& item : items) {
        item->update(deltaTime);
    }
}

} // namespace tewduwu