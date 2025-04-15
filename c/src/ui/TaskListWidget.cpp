#include "TaskListWidget.h"
#include "../renderer/Shader.h"
#include "../renderer/TextRenderer.h"
#include "../renderer/VulkanContext.h"
#include "GlassPanel.h"
#include <SDL2/SDL.h>
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <memory>

namespace tewduwu {

TaskListWidget::TaskListWidget()
  : todoList(nullptr)
  , glassPanel(nullptr)
  , textRenderer(nullptr)
  , taskShader(nullptr)
  , primaryColor(glm::vec4(1.0f, 0.255f, 0.639f, 1.0f))       // Neon Pink
  , secondaryColor(glm::vec4(0.0f, 1.0f, 0.95f, 1.0f))        // Cyan
  , accentColor(glm::vec4(0.678f, 0.361f, 1.0f, 1.0f))        // Purple
  , backgroundColor(glm::vec4(0.039f, 0.039f, 0.078f, 1.0f))  // Dark
  , textColor(glm::vec4(0.95f, 0.95f, 1.0f, 1.0f))            // Bright
  , scrollOffset(0.0f)
{ }

TaskListWidget::~TaskListWidget()
{
  // Cleanup handled in cleanup()
}

bool TaskListWidget::initialize(VulkanContext& context,
                                std::shared_ptr<TodoList> list)
{
  todoList = list;

  // Initialize text renderer
  textRenderer = std::make_shared<TextRenderer>();
  if (!textRenderer->initialize(context)) {
    std::cerr << "Failed to initialize text renderer" << std::endl;
    return false;
  }

  // Load font
  if (!textRenderer->loadFont(context, "fonts/Inconsolata-Regular.ttf", 24u)) {
    std::cerr << "Failed to load font, using system fallback" << std::endl;
    // Continue anyway since we have a fallback
  }

  // Initialize task shader
  taskShader = std::make_shared<Shader>();
  if (!taskShader->initialize(
        context, "shaders/task.vert.spv", "shaders/task.frag.spv")) {
    std::cerr << "Failed to initialize task shader" << std::endl;
    return false;
  }

  return true;
}

void TaskListWidget::cleanup(VulkanContext& context)
{
  if (textRenderer) {
    textRenderer->cleanupDeviceResources(context);
    textRenderer->cleanup();
    textRenderer = nullptr;
  }

  if (taskShader) {
    taskShader->cleanup(context);
    taskShader = nullptr;
  }

  todoList = nullptr;
  taskAnimations.clear();
}

void TaskListWidget::render(VulkanContext& context, const glm::vec4& bounds)
{
  // Implementation for rendering task list
  // For each task item, check if it has an animation state

  // Get total number of items
  size_t itemCount = todoList->getItemCount();

  // Iterate through items and render them
  for (size_t i = 0; i < itemCount; ++i) {
    auto item = todoList->getItem(i);
    if (item) {
      renderTaskItem(context, item, bounds, i);
    }
  }
}

void TaskListWidget::renderTaskItem(VulkanContext& context,
                                    std::shared_ptr<TodoItem> item,
                                    const glm::vec4& bounds,
                                    size_t index)
{
  // Placeholder for rendering a single task item
  // Bind the task shader
  if (taskShader) {
    VkCommandBuffer cmdBuffer = context.getCurrentCommandBuffer();
    taskShader->bind(cmdBuffer);

    // Check for animation state
    float flashIntensity = 0.0f;
    auto animIt = taskAnimations.find(static_cast<int>(index));
    if (animIt != taskAnimations.end()) {
      flashIntensity = animIt->second.flashIntensity;
    }

    // Set uniforms (now using UBO)
    // TODO: Determine base color for task item (e.g., based on selection/priority)
    glm::vec4 baseColor = textColor;  // Use default text color for now
    taskShader->setUniformVec4(
      "color", baseColor);  // Use "color" as defined in task.vert UBO
    taskShader->setUniformFloat("flashIntensity", flashIntensity);

    // Update the UBO buffer on the GPU
    taskShader->updateUniformBuffers(context);

    // Render the task item quad (placeholder - needs implementation)
    // In a real implementation, bind vertex buffers and draw
    // Example: vkCmdDraw(cmdBuffer, 6, 1, 0, 0); // Assuming quad vertices are set up
  }

  // Render text for the task
  if (textRenderer) {
    // Calculate position based on bounds and index
    float yPos = bounds.y + bounds.w - (index + 1) * 30.0f - scrollOffset;
    if (yPos > bounds.y && yPos < bounds.y + bounds.w) {
      // Adjust text color based on completion status or priority
      glm::vec4 currentTextColor = textColor;
      if (item->completed) {
        currentTextColor.a = 0.6f;  // Dim completed items
      }
      textRenderer->renderText(context,
                               item->text,
                               bounds.x + 20.0f + item->nestLevel * 20.0f,
                               yPos,
                               1.0f,
                               currentTextColor);
    }
  }
}

bool TaskListWidget::handleKeyInput(SDL_Keycode keyCode)
{
  if (!todoList)
    return false;

  // Declare variables used in switch cases outside the switch
  int selectedIndex;
  std::shared_ptr<TodoItem> currentItem;
  int nestLevel;

  switch (keyCode) {
    case SDLK_UP:
    case SDLK_k:
      return todoList->selectPrevious();

    case SDLK_DOWN:
    case SDLK_j:
      return todoList->selectNext();

    case SDLK_h:
      return todoList->selectParent();

    case SDLK_l:
      return todoList->selectFirstChild();

    case SDLK_SPACE: {
      selectedIndex = todoList->getSelectedIndex();
      todoList->toggleItem(selectedIndex);
      // Trigger animation on completion
      auto toggledItem = todoList->getItem(selectedIndex);
      if (toggledItem && toggledItem->completed) {
        triggerCompletionAnimation(selectedIndex);
      }
      return true;
    }

    case SDLK_d: {
      selectedIndex = todoList->getSelectedIndex();
      todoList->removeItem(selectedIndex);
      return true;
    }

    case SDLK_1: {
      selectedIndex = todoList->getSelectedIndex();
      todoList->changePriority(selectedIndex, Priority::LOW);
      return true;
    }

    case SDLK_2: {
      selectedIndex = todoList->getSelectedIndex();
      todoList->changePriority(selectedIndex, Priority::MEDIUM);
      return true;
    }

    case SDLK_3: {
      selectedIndex = todoList->getSelectedIndex();
      todoList->changePriority(selectedIndex, Priority::HIGH);
      return true;
    }

    case SDLK_0: {
      selectedIndex = todoList->getSelectedIndex();
      todoList->changePriority(selectedIndex, Priority::NONE);
      return true;
    }

    case SDLK_a: {
      // Add item - this would need user input
      // For now, add a placeholder item
      todoList->addItem("New Task", 0, Priority::NONE);
      todoList->selectNext();  // Select the newly added item
      return true;
    }

    case SDLK_i: {
      // Add subtask - this would need user input
      selectedIndex = todoList->getSelectedIndex();
      currentItem = todoList->getItem(selectedIndex);
      if (currentItem) {
        nestLevel = currentItem->nestLevel + 1;
        todoList->addItem("New Subtask", nestLevel, Priority::NONE);
        todoList->selectNext();  // Select the newly added subtask
        return true;
      }
      return false;
    }
  }

  return false;
}

void TaskListWidget::update(float deltaTime)
{
  updateAnimations(deltaTime);
}

void TaskListWidget::updateAnimations(float deltaTime)
{
  for (auto it = taskAnimations.begin(); it != taskAnimations.end();) {
    TaskAnimation& anim = it->second;
    anim.flashTimer += deltaTime;
    if (anim.flashTimer >= anim.flashDuration) {
      it = taskAnimations.erase(it);
    }
    else {
      // Update flash intensity (linear fade out)
      anim.flashIntensity = 1.0f - (anim.flashTimer / anim.flashDuration);
      ++it;
    }
  }
}

void TaskListWidget::triggerCompletionAnimation(int taskIndex)
{
  TaskAnimation& anim = taskAnimations[taskIndex];
  anim.flashIntensity = 1.0f;
  anim.flashDuration = 0.5f;  // 0.5 seconds
  anim.flashTimer = 0.0f;
}

}  // namespace tewduwu