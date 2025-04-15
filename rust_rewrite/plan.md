# tewduwu-neon Development Plan

This document outlines the development roadmap for the tewduwu-neon Rust rewrite, breaking down the work into phases with clear milestones.

## Phase 1: Foundation (Current) ✅

**Goal**: Establish the basic application structure and rendering pipeline.

- [x] Set up project with Cargo
- [x] Basic window creation with winit
- [x] WGPU initialization
- [x] Text rendering with wgpu_glyph
- [x] Event loop handling

## Phase 2: Core Task Data Structure ✅

**Goal**: Implement the foundational data structures for task management.

- [x] Create TodoItem struct
  - [x] Task content (title, description)
  - [x] Status (completed, in progress)
  - [x] Priority level (high, medium, low)
  - [x] Metadata (creation date, due date)
  - [x] Unique identifiers
- [x] Create TodoList struct
  - [x] Vec of TodoItems
  - [x] Methods for add/remove/update tasks
  - [x] Hierarchy support (parent-child relationships)
- [ ] Serialization/deserialization (serde)
  - [ ] Save task list to file (JSON or TOML)
  - [ ] Load task list from file
- [x] Unit tests for core data structures

## Phase 3: UI Framework & Layout 🚀

**Goal**: Build a flexible UI framework that adheres to the cyberpunk aesthetic.

- [x] Define UI component trait system
  - [x] Widget trait for all drawable UI elements
  - [x] Layout engine for component positioning
- [x] Implement basic UI components
  - [x] Panel/container with translucent "frosted glass" effect
  - [x] Button
  - [x] Text input
  - [x] Task item component (interactive)
  - [x] Task list component (scrollable)
- [x] Implement cyberpunk theme
  - [x] Apply color palette (Neon Pink, Cyan, Purple)
  - [ ] Implement border glow effect
  - [ ] Add ambient animations

## Phase 4: Task Management UI ✅

**Goal**: Create the task management interface for adding, editing, and organizing tasks.

- [x] Task viewing
  - [x] Render task list with proper formatting
  - [x] Implement scrolling for long lists
  - [x] Show task hierarchy with indentation
- [x] Task editing
  - [x] Add new tasks
  - [x] Edit existing tasks
  - [x] Delete tasks
  - [x] Mark tasks as complete
  - [x] Reorder tasks (drag and drop)
- [x] Task filtering
  - [x] Filter by status
  - [x] Filter by priority
  - [x] Search by text

## Phase 5: Advanced GPU Effects ✨ (Current)

**Goal**: Implement advanced visual effects using WGPU for the cyberpunk aesthetic.

- [ ] Shader pipeline enhancements
  - [ ] Create custom WGSL shaders
  - [ ] Set up shader modules and pipeline layout
- [ ] Post-processing effects
  - [ ] Bloom/glow effect for neon elements
  - [ ] Optional CRT/scanline effect
  - [ ] Color aberration for highlights
- [ ] Particle system
  - [ ] Define particle struct and behavior
  - [ ] Implement particle rendering
  - [ ] Create particle effects for task completion
- [ ] Animation system
  - [ ] Tween library for smooth transitions
  - [ ] Task completion animations
  - [ ] List update animations

## Phase 6: Input & Interaction Enhancement 🎮

**Goal**: Implement advanced input handling and interaction patterns.

- [x] Keyboard shortcuts
  - [ ] Vim-inspired navigation
  - [ ] Configurable key bindings
- [ ] Mouse interaction polishing
  - [ ] Drag and drop for task organization
  - [x] Hover effects
  - [ ] Context menus
- [x] Focus system
  - [x] Tab navigation between UI elements
  - [x] Visual indicators for focused elements

## Phase 7: Performance Optimization & Polishing 🔧

**Goal**: Ensure the application runs smoothly and looks professional.

- [ ] Performance profiling
  - [ ] Identify and fix performance bottlenecks
  - [ ] Optimize render paths
  - [ ] Minimize unnecessary re-renders
- [ ] Memory optimization
  - [ ] Proper resource cleanup
  - [ ] Texture atlas for improved GPU memory usage
- [ ] Visual polish
  - [ ] Refine animations and transitions
  - [ ] Consistent styling across all UI elements
  - [ ] Add subtle ambient animations

## Phase 8: Cross-Platform & Deployment 🌐

**Goal**: Ensure compatibility across platforms and prepare for distribution.

- [ ] Test and fix platform-specific issues
  - [ ] Windows
  - [ ] macOS
  - [ ] Linux
- [ ] WebAssembly support
  - [ ] Adapt rendering for web context
  - [ ] Handle web-specific storage
- [ ] Create build/release pipeline
  - [ ] Automated builds
  - [ ] Package for distribution
- [ ] Create documentation
  - [ ] User guide
  - [ ] API documentation
  - [ ] Key bindings reference

## Implementation Priorities

For each phase, we'll prioritize in this order:
1. Core functionality (must-have features)
2. Basic visual styling (to match the cyberpunk theme)
3. GPU optimizations and advanced effects
4. Polishing and refinement

## Technology Selection Details

- **Graphics**: wgpu (already implemented)
- **Windowing**: winit (already implemented)
- **Text Rendering**: wgpu_glyph (already implemented)
- **Serialization**: serde + serde_json/toml
- **Math/Geometry**: glam or nalgebra
- **Asset Management**: Consider asset loading crates like `assets_manager`
- **Logging/Debugging**: tracing/tracing-subscriber for more advanced logging

## Next Steps

1. Complete Phase 5 (Advanced GPU Effects)
   - Set up shader pipeline for neon effects
   - Create post-processing effects for cyberpunk aesthetic
2. Begin work on Phase 6 (Input & Interaction Enhancement)
   - Implement Vim-inspired navigation
   - Polish mouse interactions and context menus
3. Continually update this plan as development progresses 