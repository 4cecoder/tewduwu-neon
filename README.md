# tewduwu-neon

<div align="center">
  <img src="docs/images/logo.png" alt="tewduwu-neon logo" width="250" height="250" style="display: none;">
  <h3>✨ Cyberpunk TODO list with glassy aesthetics and neon vibes ✨</h3>
</div>

> Sleek, GPU-accelerated task management with Vim-inspired controls

## 🌟 Features

- **Neon Cyberpunk Theme** - Eye-catching pink/cyan/purple aesthetic
- **GPU Accelerated** - Vulkan rendering pipeline for smooth performance
- **Glassy UI** - Translucent frosted glass panels with bloom effects
- **Vim-inspired Controls** - Fast keyboard-centric navigation
- **Hierarchical Tasks** - Organize with nested subtasks
- **Animated Transitions** - Smooth motion between UI states

## 🖥️ Technology

- **Core**: C++17
- **Graphics**: SDL3 + Vulkan
- **Text**: FreeType + HarfBuzz
- **Build**: CMake

## 🚀 Roadmap

- **Phase 1**: Core Framework
  - [ ] SDL3 + Vulkan initialization
  - [ ] Basic rendering pipeline
  - [ ] Text rendering with FreeType
  - [ ] Data model port

- **Phase 2**: UI Components
  - [ ] Glass panel renderer
  - [ ] Task list widget
  - [ ] Status bar
  - [ ] Animations system

- **Phase 3**: Visual Effects
  - [ ] Bloom shader for neon glow
  - [ ] Particle system for ambient effects
  - [ ] Post-processing pipeline

- **Phase 4**: UX Refinement
  - [ ] Vim-style input handling
  - [ ] Custom cursor effects
  - [ ] Sound effects

## 🎨 Design

### Color Palette
- **Primary**: `#FF41A3` (Neon Pink)
- **Secondary**: `#00FFF3` (Cyan)
- **Accent**: `#AD5CFF` (Purple)
- **Background**: `#0A0A14` (Dark)
- **Text**: `#F2F2FF` (Bright)

## 🏗️ Project Structure

```
src/
├── core/             # Core data structures
├── renderer/         # Vulkan-based rendering system
├── ui/               # UI components and widgets
├── shaders/          # GLSL shader code
└── resources/        # Fonts, textures, assets
```

## 🔧 Building

```bash
# Clone the repository
git clone https://github.com/4cecoder/tewduwu-neon.git
cd tewduwu-neon

# Create build directory
mkdir build && cd build

# Configure and build
cmake ..
make

# Run
./tewduwu-neon
```

## 📝 Dependencies

- SDL3
- Vulkan SDK
- FreeType
- HarfBuzz

## 💖 Credits

- Based on the terminal version [tewduwu](https://github.com/4cecoder/tewduwu)
