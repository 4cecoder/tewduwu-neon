#!/bin/bash
set -e

# Color definitions
RED='\033[0;31m'
GREEN='\033[0;32m'
CYAN='\033[0;36m'
YELLOW='\033[1;33m'
MAGENTA='\033[0;35m'
NC='\033[0m' # No Color

echo -e "${MAGENTA}=====================================${NC}"
echo -e "${MAGENTA}     tewduwu-neon build script      ${NC}"
echo -e "${MAGENTA}=====================================${NC}"

# Check for required dependencies
echo -e "${CYAN}Checking dependencies...${NC}"

# Check for cmake
if ! command -v cmake &> /dev/null; then
    echo -e "${RED}ERROR: cmake not found. Please install it:${NC}"
    echo "    brew install cmake"
    exit 1
fi

# Check for Vulkan SDK
if [ -z "$VULKAN_SDK" ]; then
    echo -e "${RED}ERROR: VULKAN_SDK environment variable not set${NC}"
    echo "Please install the Vulkan SDK from https://vulkan.lunarg.com/"
    echo "Then set VULKAN_SDK environment variable, e.g.:"
    echo "    export VULKAN_SDK=~/VulkanSDK/1.3.xxx.0/macOS"
    exit 1
fi

# Check for other dependencies
echo -e "${CYAN}Checking build dependencies...${NC}"
DEPS_TO_INSTALL=""

# Check for SDL3
if ! brew list --formula | grep -q "sdl3"; then
    DEPS_TO_INSTALL="$DEPS_TO_INSTALL sdl3"
fi

# Check for freetype
if ! brew list --formula | grep -q "freetype"; then
    DEPS_TO_INSTALL="$DEPS_TO_INSTALL freetype"
fi

# Check for harfbuzz
if ! brew list --formula | grep -q "harfbuzz"; then
    DEPS_TO_INSTALL="$DEPS_TO_INSTALL harfbuzz"
fi

# Check for glm
if ! brew list --formula | grep -q "glm"; then
    DEPS_TO_INSTALL="$DEPS_TO_INSTALL glm"
fi

# Install missing dependencies
if [ -n "$DEPS_TO_INSTALL" ]; then
    echo -e "${YELLOW}Installing missing dependencies:${NC}$DEPS_TO_INSTALL"
    brew install $DEPS_TO_INSTALL
fi

# Create build directory
echo -e "${CYAN}Creating build directory...${NC}"
mkdir -p build
cd build

# Configure with CMake
echo -e "${CYAN}Configuring with CMake...${NC}"
cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DSDL3_DIR=$(brew --prefix sdl3)/lib/cmake/SDL3 \
    -DFREETYPE_DIR=$(brew --prefix freetype) \
    -DHARFBUZZ_DIR=$(brew --prefix harfbuzz) \
    -DGLM_DIR=$(brew --prefix glm)

# Build
echo -e "${CYAN}Building...${NC}"
cmake --build . -- -j$(sysctl -n hw.physicalcpu)

echo -e "${GREEN}Build complete!${NC}"

# Check if build was successful
if [ -f "bin/tewduwu-neon" ]; then
    echo -e "${GREEN}Successfully built tewduwu-neon!${NC}"
    echo -e "${CYAN}Run the application? [y/N]${NC}"
    read -r answer
    if [[ "$answer" =~ ^[Yy]$ ]]; then
        echo -e "${MAGENTA}Running tewduwu-neon...${NC}"
        ./bin/tewduwu-neon
    fi
else
    echo -e "${RED}Build seems to have failed. Check the errors above.${NC}"
    exit 1
fi
