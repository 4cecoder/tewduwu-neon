#include <SDL3/SDL.h>
#include <iostream>
#include <memory>
#include <stdexcept>

#include "renderer/VulkanContext.h"
#include "ui/TaskListWidget.h"
#include "core/TodoList.h"

using namespace tewduwu;

const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;
const char* APP_NAME = "tewduwu-neon";

int main(int argc, char* argv[]) {
    try {
        // Initialize SDL
        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
            std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
            throw std::runtime_error("Failed to initialize SDL: " + std::string(SDL_GetError()));
        }
        
        // Create window
        SDL_Window* window = SDL_CreateWindow(
            APP_NAME,
            WINDOW_WIDTH,
            WINDOW_HEIGHT,
            SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
        );
        
        if (!window) {
            throw std::runtime_error("Failed to create window: " + std::string(SDL_GetError()));
        }
        
        // Initialize Vulkan
        auto vulkanContext = std::make_shared<VulkanContext>();
        if (!vulkanContext->initialize(window)) {
            throw std::runtime_error("Failed to initialize Vulkan");
        }
        
        // Create TODO list
        auto todoList = std::make_shared<TodoList>();
        todoList->loadFromFile("todolist.dat");
        
        // Set up UI
        auto taskListWidget = std::make_shared<TaskListWidget>();
        if (!taskListWidget->initialize(*vulkanContext, todoList)) {
            throw std::runtime_error("Failed to initialize UI");
        }
        
        // Set cyberpunk theme colors
        taskListWidget->setPrimaryColor(glm::vec4(1.0f, 0.255f, 0.639f, 1.0f));   // Neon Pink
        taskListWidget->setSecondaryColor(glm::vec4(0.0f, 1.0f, 0.95f, 1.0f));    // Cyan
        taskListWidget->setAccentColor(glm::vec4(0.678f, 0.361f, 1.0f, 1.0f));    // Purple
        taskListWidget->setBackgroundColor(glm::vec4(0.039f, 0.039f, 0.078f, 1.0f)); // Dark
        taskListWidget->setTextColor(glm::vec4(0.95f, 0.95f, 1.0f, 1.0f));        // Bright
        
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
                if (event.type == SDL_EVENT_QUIT) {
                    running = false;
                } else if (event.type == SDL_EVENT_KEY_DOWN) {
                    if (event.key.key == SDLK_ESCAPE) {
                        running = false;
                    } else {
                        taskListWidget->handleKeyInput(event.key.key);
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
            glm::vec4 bounds(windowWidth * 0.1f, windowHeight * 0.1f, 
                           windowWidth * 0.8f, windowHeight * 0.8f);
            
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
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
}