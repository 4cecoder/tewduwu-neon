#pragma once

#include "../core/TodoList.h"
#include <SDL.h>
#include <glm/glm.hpp>
#include <memory>
#include <unordered_map>

namespace tewduwu {

class VulkanContext;
class GlassPanel;
class TextRenderer;
class Shader;

class TaskListWidget {
public:
  TaskListWidget();
  ~TaskListWidget();

  bool initialize(VulkanContext& context, std::shared_ptr<TodoList> list);
  void cleanup(VulkanContext& context);
  void update(float deltaTime);
  void render(VulkanContext& context, const glm::vec4& bounds);
  bool handleKeyInput(SDL_Keycode keyCode);

  // Theme setters
  void setPrimaryColor(const glm::vec4& color) { primaryColor = color; }
  void setSecondaryColor(const glm::vec4& color) { secondaryColor = color; }
  void setAccentColor(const glm::vec4& color) { accentColor = color; }
  void setBackgroundColor(const glm::vec4& color) { backgroundColor = color; }
  void setTextColor(const glm::vec4& color) { textColor = color; }

private:
  std::shared_ptr<TodoList> todoList;
  std::shared_ptr<GlassPanel> glassPanel;
  std::shared_ptr<TextRenderer> textRenderer;
  std::shared_ptr<Shader> taskShader;

  // Theme colors
  glm::vec4 primaryColor;
  glm::vec4 secondaryColor;
  glm::vec4 accentColor;
  glm::vec4 backgroundColor;
  glm::vec4 textColor;

  // Animation state for task completion
  struct TaskAnimation {
    float flashIntensity;
    float flashDuration;
    float flashTimer;
  };
  std::unordered_map<int, TaskAnimation> taskAnimations;  // Keyed by task index

  void triggerCompletionAnimation(int taskIndex);
  void updateAnimations(float deltaTime);

  // Rendering/layout helpers
  void renderTaskItem(VulkanContext& context,
                      std::shared_ptr<TodoItem> item,
                      const glm::vec4& bounds,
                      size_t index);

  // Scrolling state
  float scrollOffset;
  // float targetScrollOffset; // Removed, unused
  // float scrollVelocity;     // Removed, unused

  // Animation
  // float hoverEffectIntensity; // Removed, unused
};

}  // namespace tewduwu