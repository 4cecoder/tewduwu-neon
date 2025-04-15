cmake_minimum_required(VERSION 3.20)
project(tewduwu-neon VERSION 0.1.0 LANGUAGES CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(SDL2 REQUIRED)
find_package(Vulkan REQUIRED)
find_package(Freetype REQUIRED)
find_package(HarfBuzz REQUIRED)
find_package(glm REQUIRED)

# Explicitly set SDL2 paths if find_package doesn't work as expected
if(NOT SDL2_FOUND)
    set(SDL2_INCLUDE_DIRS "/opt/homebrew/opt/sdl2/include")
    set(SDL2_LIBRARIES "/opt/homebrew/opt/sdl2/lib/libSDL2.dylib")
endif()

# Define source files
set(SOURCES
    src/main.cpp
    src/core/TodoItem.cpp
    src/core/TodoList.cpp
    src/renderer/VulkanContext.cpp
    src/renderer/Shader.cpp
    src/renderer/TextRenderer.cpp
    src/ui/GlassPanel.cpp
    src/ui/TaskListWidget.cpp
    # Add implementation files as they're created
)

# Define header files for IDEs
set(HEADERS
    src/core/TodoItem.h
    src/core/TodoList.h
    src/renderer/VulkanContext.h
    src/renderer/Shader.h
    src/renderer/TextRenderer.h
    src/ui/GlassPanel.h
    src/ui/TaskListWidget.h
)

# Define shader files
set(SHADERS
    src/shaders/glass.vert
    src/shaders/glass.frag
    src/shaders/text.vert
    src/shaders/text.frag
    src/shaders/task.vert
    src/shaders/task.frag
)

# Add executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL2::SDL2
    Vulkan::Vulkan
    Freetype::Freetype
    harfbuzz::harfbuzz
    glm::glm
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${SDL2_INCLUDE_DIRS}
)

# Shader compilation (requires glslc from Vulkan SDK)
if(${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Windows")
    find_program(GLSLC_EXECUTABLE NAMES glslc HINTS "$ENV{VULKAN_SDK}/Bin" REQUIRED)
else()
    find_program(GLSLC_EXECUTABLE NAMES glslc HINTS "$ENV{VULKAN_SDK}/bin" REQUIRED)
endif()

# Compile shaders
set(SHADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})

foreach(SHADER ${SHADERS})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    set(SHADER_OUTPUT "${SHADER_OUTPUT_DIR}/${SHADER_NAME}.spv")
    
    add_custom_command(
        OUTPUT ${SHADER_OUTPUT}
        COMMAND ${GLSLC_EXECUTABLE} -o ${SHADER_OUTPUT} ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER}
        COMMENT "Compiling shader: ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER}"
    )
    
    list(APPEND SHADER_OUTPUTS ${SHADER_OUTPUT})
endforeach()

# Add shader compilation as a dependency
add_custom_target(shaders DEPENDS ${SHADER_OUTPUTS})
add_dependencies(${PROJECT_NAME} shaders)

# Copy shaders to build directory
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${SHADER_OUTPUT_DIR} $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
    COMMENT "Copying shaders to output directory"
)

# Create fonts directory and copy example font (Inconsolata is used in the code)
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${PROJECT_NAME}>/fonts
    COMMENT "Creating fonts directory"
)

# Set output directories
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Enable compiler warnings
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()

# Generate compile_commands.json for IDE code completion
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Installation
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(DIRECTORY ${SHADER_OUTPUT_DIR}/ DESTINATION bin/shaders)