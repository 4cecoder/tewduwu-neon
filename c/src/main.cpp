#include <SDL2/SDL.h>
#include <iostream>
#include <memory>
#include <stdexcept>

#include "core/TodoList.h"
#include "renderer/VulkanContext.h"
#include "ui/TaskListWidget.h"

using namespace tewduwu;

const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;
const char* APP_NAME = "tewduwu-neon";

int main(int argc, char* argv[])
{
  try {
    // Initialize SDL (start with 0 to check basic init)
    if (SDL_Init(0) != 0) {
      const char* sdlError = SDL_GetError();
      std::string errorMsg =
        "Unknown SDL basic initialization (SDL_Init(0)) error";
      if (sdlError && *sdlError) {  // Check for non-NULL and non-empty string
        errorMsg = std::string("SDL_Init(0) failed: ") + sdlError;
      }
      std::cerr << "Error: " << errorMsg << std::endl;
      throw std::runtime_error(errorMsg);
    }

    // Now initialize video subsystem
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
      const char* sdlError = SDL_GetError();
      std::string errorMsg = "Unknown SDL video subsystem initialization error";
      if (sdlError && *sdlError) {  // Check for non-NULL and non-empty string
        errorMsg =
          std::string("SDL_InitSubSystem(SDL_INIT_VIDEO) failed: ") + sdlError;
      }
      std::cerr << "Error: " << errorMsg << std::endl;
      // Clean up basic SDL init before throwing
      SDL_Quit();
      throw std::runtime_error(errorMsg);
    }

    // Create window
    SDL_Window* window =
      SDL_CreateWindow(APP_NAME,
                       SDL_WINDOWPOS_CENTERED,
                       SDL_WINDOWPOS_CENTERED,
                       WINDOW_WIDTH,
                       WINDOW_HEIGHT,
                       SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

    if (!window) {
      throw std::runtime_error("Failed to create window: " +
                               std::string(SDL_GetError()));
    }

    // Initialize Vulkan
    auto vulkanContext = std::make_shared<VulkanContext>();
    if (!vulkanContext->initialize(window)) {
      throw std::runtime_error("Failed to initialize Vulkan");
    }

    // Create TODO list
    auto todoList = std::make_shared<TodoList>();
    if (!todoList->loadFromFile("todolist.dat")) {
      std::cout
        << "Note: todolist.dat not found or failed to load. Starting with empty list."
        << std::endl;
    }
    // Add a test item if the list is empty
    if (todoList->getItemCount() == 0) {
      todoList->addItem("Test Item 1", 0, Priority::NONE);
      todoList->addItem("Test Item 2", 0, Priority::MEDIUM);
      todoList->addItem("  Subtask", 1, Priority::LOW);
      todoList->addItem("Test Item 3", 0, Priority::HIGH);
    }
    std::cout << "Loaded " << todoList->getItemCount()
              << " items into the TODO list." << std::endl;

    // Set up UI
    auto taskListWidget = std::make_shared<TaskListWidget>();
    if (!taskListWidget->initialize(*vulkanContext, todoList)) {
      throw std::runtime_error("Failed to initialize UI");
    }

    // Set cyberpunk theme colors
    taskListWidget->setPrimaryColor(
      glm::vec4(1.0f, 0.255f, 0.639f, 1.0f));  // Neon Pink
    taskListWidget->setSecondaryColor(
      glm::vec4(0.0f, 1.0f, 0.95f, 1.0f));  // Cyan
    taskListWidget->setAccentColor(
      glm::vec4(0.678f, 0.361f, 1.0f, 1.0f));  // Purple
    taskListWidget->setBackgroundColor(
      glm::vec4(0.039f, 0.039f, 0.078f, 1.0f));  // Dark
    taskListWidget->setTextColor(
      glm::vec4(0.95f, 0.95f, 1.0f, 1.0f));  // Bright

    // Main loop
    bool running = true;
    Uint64 lastTime = SDL_GetTicks();

    while (running) {
      // Calculate delta time
      Uint64 currentTime = SDL_GetTicks();
      float deltaTime = (currentTime - lastTime) / 1000.0f;
      lastTime = currentTime;

      // Handle events
      SDL_Event event;
      while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
          running = false;
        }
        else if (event.type == SDL_KEYDOWN) {
          if (event.key.keysym.sym == SDLK_ESCAPE) {
            running = false;
          }
          else {
            taskListWidget->handleKeyInput(event.key.keysym.sym);
          }
        }
      }

      // Update
      todoList->update(deltaTime);
      taskListWidget->update(deltaTime);

      // Render
      vulkanContext->beginFrame();

      // Calculate bounds for the task list (centered with padding)
      int windowWidth, windowHeight;
      SDL_GetWindowSize(window, &windowWidth, &windowHeight);
      glm::vec4 bounds(windowWidth * 0.1f,
                       windowHeight * 0.1f,
                       windowWidth * 0.8f,
                       windowHeight * 0.8f);

      taskListWidget->render(*vulkanContext, bounds);

      vulkanContext->endFrame();
    }

    // Save before exit
    todoList->saveToFile("todolist.dat");

    // Cleanup
    vulkanContext->waitIdle();
    vulkanContext->cleanup();
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
  }
  catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return -1;
  }
}