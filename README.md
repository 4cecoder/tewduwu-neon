# tewduwu-neon (Rust Rewrite)

<div align=\"center\">
  <h3>✨ Cyberpunk TODO list in Rust with wgpu, winit, and neon vibes ✨</h3>
</div>

> A rewrite of the original C++ `tewduwu-neon` project, leveraging Rust's safety, modern tooling, and the `wgpu` graphics API for **cross-platform GPU acceleration** and **rich visual effects**.

## 🚀 Motivation

This project aims to rebuild the `tewduwu-neon` concept in Rust. The goals include:
- Exploring Rust for desktop GUI and graphics development.
- Utilizing `wgpu` for a **modern, cross-platform graphics API** (targeting Vulkan, Metal, DX12, OpenGL, and eventually WebAssembly) that is safer than raw graphics programming.
- **Achieving high-performance, visually stunning GPU effects** thanks to `wgpu` and Rust's performance characteristics.
- Benefiting from Rust's strong type system, memory safety guarantees, and excellent tooling (Cargo).
- Re-architecting based on learnings from the C++ implementation, focusing on modularity and maintainability.

## 🌟 Features (Planned/In Progress)

The feature set aims to eventually match or exceed the C++ version:

- **Core Task Management:**
    - [ ] Hierarchical TODO items (nesting).
    - [ ] Item addition, deletion, modification.
    - [ ] Priority levels.
    - [ ] Persistence (saving/loading tasks).
- **Visuals & Aesthetics:**
    - [ ] Neon Cyberpunk Theme (Pink/Cyan/Purple Palette).
    - [ ] **Cross-Platform GPU Accelerated Rendering** (`wgpu`).
    - [ ] Translucent \"Frosted Glass\" UI Panels.
    - [ ] Task Completion Animation (e.g., Neon Flash).
    - [ ] **(Goal) Advanced GPU Effects:**
        - [ ] Bloom/Glow Shaders.
        - [ ] Particle System for Ambient Effects.
        - [ ] Post-processing pipeline for visual enhancements.
- **Interaction:**
    - [ ] (Planned) Vim-inspired Keyboard Controls.
    - [ ] Basic windowing and event handling (`winit`).
- **Text Rendering:**
    - [ ] High-quality text rendering using appropriate Rust crates.

## 🎨 Design Concept

Retains the core visual identity of the C++ version:

### Color Palette
- **Primary**: `#FF41A3` (Neon Pink)
- **Secondary**: `#00FFF3` (Cyan)
- **Accent**: `#AD5CFF` (Purple)
- **Background**: `#0A0A14` (Dark)
- **Text**: `#F2F2FF` (Bright)

## 💻 Technology Stack

- **Language**: Rust (Stable)
- **Graphics**: `wgpu` - Chosen for **cross-platform compatibility** (Vulkan, Metal, DX12, OpenGL, WASM) and modern API design, enabling **advanced GPU effects**.
- **Windowing & Events**: `winit` - Cross-platform window creation and event loop management.
- **Text Rendering**: TBD (Likely `glyph_brush`, `rusttype`, `ab_glyph`, or similar) - Needs investigation for `wgpu` integration.
- **Build System**: `cargo`
- **Logging**: `env_logger` / `tracing` (Recommended)

## 🏗️ Project Status

**Early Development / Rewrite in Progress.**

This is a fresh start. The C++ version serves as a detailed reference for features, architecture, and visual goals. Key challenges identified during C++ development (like complex build systems, raw graphics API intricacies, font rendering details) will inform the design choices in this Rust version. Choosing `wgpu` aims to simplify the graphics backend complexities while enabling powerful features across multiple operating systems.

## 🔧 Building & Running

1.  **Install Rust:** If you don't have it, install Rust via [rustup](https://rustup.rs/).
2.  **Install `wgpu` Dependencies:** Depending on your OS, you might need development libraries for Vulkan, Metal, or DX12. `wgpu` often requires `cmake` as well. Refer to the [official `wgpu` examples README](https://github.com/gfx-rs/wgpu/blob/master/wgpu-examples/README.md) for prerequisite details.
3.  **Clone (if needed):** Ensure you are in the main project directory.
4.  **Navigate:** `cd rust_rewrite`
5.  **Build:** `cargo build` (or `cargo build --release` for optimizations)
6.  **Run:** `cargo run` (or `cargo run --release`)

## 🗂️ Project Structure (Anticipated)

```
rust_rewrite/
├── Cargo.toml        # Rust package manager file
├── assets/           # Fonts, textures, etc.
│   └── fonts/
│       └── Inconsolata-Regular.ttf
├── shaders/          # WGSL shader code
│   ├── text.wgsl
│   └── task.wgsl
├── src/
│   ├── core/         # Core data structures (TodoList, TodoItem)
│   ├── renderer/     # wgpu based rendering system, shaders, pipeline setup
│   ├── ui/           # UI components, widgets, layout
│   ├── main.rs       # Application entry point, event loop
│   └── lib.rs        # Library components (if structured as a library)
└── README.md         # This file
```

## 🤝 Contributing

Contributions are welcome! Please feel free to open issues or pull requests. Adhere to standard Rust coding conventions and formatting (`cargo fmt`).

## 📜 License

*Decision Needed: Please choose an appropriate open-source license (e.g., MIT, Apache-2.0) and add a `LICENSE` file.*

## 🙏 Acknowledgements

- Based on the original C++ project [tewduwu-neon](https://github.com/4cecoder/tewduwu-neon).
- Learnings and debugging efforts from the C++ iteration significantly inform this rewrite. 