use log::{error, info};
use winit::{
    event::{Event, WindowEvent, KeyEvent, ElementState},
    event_loop::{EventLoop},
    window::{Window, WindowBuilder},
};
use wgpu::{
    Adapter,
    Device,
    Instance,
    InstanceDescriptor,
    Queue,
    RequestAdapterOptions,
    Surface,
    SurfaceConfiguration,
    SurfaceError,
    TextureUsages,
    util::StagingBelt,
    TextureDescriptor,
    Extent3d,
    TextureDimension,
    TextureViewDescriptor,
};
use std::sync::Arc; // Use Arc for window sharing
use std::sync::Mutex;

// Use types from wgpu_glyph
use wgpu_glyph::ab_glyph;
use wgpu_glyph::{GlyphBrush, GlyphBrushBuilder};

// Import our core module
mod core;
use core::prelude::*;

// Import our UI module
mod ui;
use ui::prelude::*;

// We need to create a window wrapper that preserves the window
// for the lifetime of the surface
struct WindowWrapper {
    window: Arc<Window>,
}

impl WindowWrapper {
    fn new(window: Arc<Window>) -> Self {
        Self { window }
    }
    
    fn create_surface(&self, instance: &Instance) -> Surface<'static> {
        // This is unsafe because we're tying the surface lifetime to static,
        // but we're ensuring the window stays alive for the duration of the surface
        // through the WindowWrapper in State
        let surface = unsafe {
            // We're using the WGPU internal API to convert a non-static surface to 'static
            // This is safe because we guarantee the window will live as long as the surface
            let temp_surface = instance.create_surface(self.window.as_ref())
                .expect("Failed to create surface");
            std::mem::transmute::<Surface<'_>, Surface<'static>>(temp_surface)
        };
        surface
    }
    
    fn window(&self) -> &Window {
        &self.window
    }
}

struct State {
    window_wrapper: WindowWrapper, // Wrapper that keeps the window alive
    _instance: Instance,  
    surface: Surface<'static>,
    _adapter: Adapter,    
    device: Device,
    queue: Queue,
    config: SurfaceConfiguration,
    size: winit::dpi::PhysicalSize<u32>,
    
    // Text Rendering State
    glyph_brush: GlyphBrush<()>, 
    staging_belt: StagingBelt, 
    
    // Application State
    todo_list: Arc<Mutex<TodoList>>,
    
    // UI State
    todo_list_widget: TodoListWidget,
    theme: CyberpunkTheme,
    
    // Input State
    mouse_pos: (f32, f32),
    
    // Post-processing effects
    bloom_effect: BloomEffect,
    neon_glow_effect: NeonGlowEffect,
}

impl State {
    // Creating some of the wgpu types requires async code
    async fn new(window: Arc<Window>) -> Self {
        let size = window.inner_size();
        
        info!("Creating wgpu instance...");
        let instance = Instance::new(InstanceDescriptor::default());
        
        // Create our window wrapper which guarantees the window stays alive
        let window_wrapper = WindowWrapper::new(window);
        
        info!("Creating surface from window...");
        // Create the surface using our wrapper which handles the lifetime properly
        let surface = window_wrapper.create_surface(&instance);
        
        info!("Selecting GPU adapter...");
        let adapter = instance.request_adapter(
            &RequestAdapterOptions {
                power_preference: wgpu::PowerPreference::default(),
                force_fallback_adapter: false,
                compatible_surface: Some(&surface),
            },
        ).await.expect("Failed to find an appropriate adapter");
        
        info!("Selected adapter: {:?}", adapter.get_info().name);
        
        let (device, queue) = adapter.request_device(
            &wgpu::DeviceDescriptor {
                label: Some("Device"),
                required_features: wgpu::Features::empty(),
                required_limits: wgpu::Limits::default(),
                memory_hints: wgpu::MemoryHints::default(),
            },
            None, // Trace path
        ).await.expect("Failed to create device");
        
        // Configure the surface
        let surface_caps = surface.get_capabilities(&adapter);
        // We'll use sRGB for better color accuracy
        let surface_format = surface_caps.formats.iter()
            .copied()
            .filter(|f| f.is_srgb())
            .next()
            .unwrap_or(surface_caps.formats[0]);
        
        let config = SurfaceConfiguration {
            usage: TextureUsages::RENDER_ATTACHMENT,
            format: surface_format,
            width: size.width,
            height: size.height,
            present_mode: wgpu::PresentMode::Fifo, // VSync
            alpha_mode: surface_caps.alpha_modes[0],
            view_formats: vec![],
            desired_maximum_frame_latency: 2,
        };
        
        info!("Configuring surface...");
        surface.configure(&device, &config);
        
        // --- Text Rendering Setup --- 
        // Load the font
        let font_data = std::fs::read("fonts/Inconsolata-Regular.ttf").expect("Failed to read font file");
        // wgpu_glyph uses FontArc directly in the builder
        let font = ab_glyph::FontArc::try_from_vec(font_data).expect("Failed to load font from data");
        info!("Font loaded successfully.");
        
        // Create glyph_brush and staging belt
        info!("Creating GlyphBrush...");
        let glyph_brush = GlyphBrushBuilder::using_font(font)
            .build(&device, surface_format);
            
        info!("Creating StagingBelt...");
        // Create a staging belt for the text rendering pipeline
        let staging_belt = StagingBelt::new(1024); // 1KB staging belt
        
        // --- Todo List Setup ---
        info!("Setting up todo list...");
        let mut todo_list_inner = TodoList::new("Project Tasks");
        
        // Create some example tasks
        let project_tasks_id = todo_list_inner.add_item(TodoItem::new("Project Management"));
        
        // Create GPU Effects section
        let gpu_effects_id = todo_list_inner.add_item(TodoItem::new("GPU Effects")
            .with_priority(Priority::High));
        todo_list_inner.add_item(TodoItem::new("Implement bloom/glow shader")
            .with_parent(gpu_effects_id)
            .with_priority(Priority::High));
        todo_list_inner.add_item(TodoItem::new("Create custom WGSL shaders")
            .with_parent(gpu_effects_id)
            .with_priority(Priority::High));
        todo_list_inner.add_item(TodoItem::new("Add particle system for task completion")
            .with_parent(gpu_effects_id)
            .with_priority(Priority::Medium));
        
        // Create Input section
        let input_id = todo_list_inner.add_item(TodoItem::new("Input Improvements")
            .with_priority(Priority::Medium));
        todo_list_inner.add_item(TodoItem::new("Implement Vim-inspired navigation")
            .with_parent(input_id)
            .with_priority(Priority::Medium));
        todo_list_inner.add_item(TodoItem::new("Add context menus")
            .with_parent(input_id)
            .with_priority(Priority::Low));
        
        // Create Polishing section
        let polish_id = todo_list_inner.add_item(TodoItem::new("Visual Polish")
            .with_priority(Priority::Low));
        todo_list_inner.add_item(TodoItem::new("Refine animations and transitions")
            .with_parent(polish_id)
            .with_priority(Priority::Low));
        
        // Create Completed section
        let completed_id = todo_list_inner.add_item(TodoItem::new("Completed Features"));
        let ui_comp_id = todo_list_inner.add_item(TodoItem::new("UI Components")
            .with_parent(completed_id)
            .with_priority(Priority::Medium));
        let filtering_id = todo_list_inner.add_item(TodoItem::new("Task filtering")
            .with_parent(completed_id)
            .with_priority(Priority::Medium));
        let hierarchy_id = todo_list_inner.add_item(TodoItem::new("Task hierarchy visualization")
            .with_parent(completed_id)
            .with_priority(Priority::Medium));
        
        // Mark completed tasks
        todo_list_inner.get_item_mut(ui_comp_id).unwrap().mark_completed();
        todo_list_inner.get_item_mut(filtering_id).unwrap().mark_completed();
        todo_list_inner.get_item_mut(hierarchy_id).unwrap().mark_completed();
        
        info!("Todo list initialized with {} items", todo_list_inner.len());
        
        // Wrap the TodoList in an Arc<Mutex>
        let todo_list = Arc::new(Mutex::new(todo_list_inner));
        
        // Initialize the CyberpunkTheme
        let theme = CyberpunkTheme::new();
        
        // Create the TodoListWidget
        let todo_list_widget = TodoListWidget::new(
            50.0, // x
            100.0, // y
            size.width as f32 - 100.0, // width
            size.height as f32 - 200.0, // height
            todo_list.clone()
        )
        .with_on_status_change(|item| {
            info!("Status changed for item {}: {:?}", item.id(), item.status());
        })
        .with_on_edit(|item| {
            info!("Edit requested for item {}: {}", item.id(), item.title());
        })
        .with_on_delete(|item| {
            info!("Delete requested for item {}", item.id());
        });
        
        // Create post-processing effects
        let bloom_effect = BloomEffect::new(
            Arc::new(device.clone()),
            Arc::new(queue.clone()),
            config.format
        );

        let neon_glow_effect = NeonGlowEffect::new(
            Arc::new(device.clone()),
            Arc::new(queue.clone()),
            config.format,
            &theme
        );

        // Initialize effects with the window size
        bloom_effect.resize(size.width, size.height);

        info!("WGPU state initialized successfully.");
        
        Self {
            window_wrapper,
            _instance: instance,
            surface,
            _adapter: adapter,
            device,
            queue,
            config,
            size,
            glyph_brush,
            staging_belt,
            todo_list,
            todo_list_widget,
            theme,
            mouse_pos: (0.0, 0.0),
            bloom_effect,
            neon_glow_effect,
        }
    }

    fn resize(&mut self, new_size: winit::dpi::PhysicalSize<u32>) {
        if new_size.width > 0 && new_size.height > 0 {
            self.size = new_size;
            self.config.width = new_size.width;
            self.config.height = new_size.height;
            self.surface.configure(&self.device, &self.config);
            info!("Surface reconfigured for resize: {:?}", self.config);
            
            // Resize post-processing effects
            self.bloom_effect.resize(new_size.width, new_size.height);
            
            // Update UI components with new size
            self.todo_list_widget.set_dimensions(
                new_size.width as f32 - 100.0,
                new_size.height as f32 - 200.0
            );
        }
    }

    fn update(&mut self, delta_time: f32) {
        // Update UI widgets
        self.todo_list_widget.update(delta_time);
    }

    fn render(&mut self) -> Result<(), SurfaceError> {
        let output = self.surface.get_current_texture()?;
        let view = output.texture.create_view(&wgpu::TextureViewDescriptor::default());

        // Create temporary textures for post-processing
        let scene_buffer_desc = wgpu::TextureDescriptor {
            label: Some("Scene Buffer"),
            size: wgpu::Extent3d {
                width: self.size.width,
                height: self.size.height,
                depth_or_array_layers: 1,
            },
            mip_level_count: 1,
            sample_count: 1,
            dimension: wgpu::TextureDimension::D2,
            format: self.config.format,
            usage: wgpu::TextureUsages::TEXTURE_BINDING | wgpu::TextureUsages::RENDER_ATTACHMENT,
            view_formats: &[],
        };
        
        let scene_buffer = self.device.create_texture(&scene_buffer_desc);
        let scene_view = scene_buffer.create_view(&wgpu::TextureViewDescriptor::default());
        
        let bloom_buffer = self.device.create_texture(&scene_buffer_desc);
        let bloom_view = bloom_buffer.create_view(&wgpu::TextureViewDescriptor::default());

        let mut encoder = self.device.create_command_encoder(&wgpu::CommandEncoderDescriptor {
            label: Some("Render Encoder"),
        });

        // --- First render pass - render scene to scene_buffer ---
        {
            let mut render_pass = encoder.begin_render_pass(&wgpu::RenderPassDescriptor {
                label: Some("Scene Pass"),
                color_attachments: &[Some(wgpu::RenderPassColorAttachment {
                    view: &scene_view,
                    resolve_target: None,
                    ops: wgpu::Operations {
                        load: wgpu::LoadOp::Clear(wgpu::Color {
                            r: 0.039, // Very dark blue/purple background (#0A0A14)
                            g: 0.039,
                            b: 0.078,
                            a: 1.0,
                        }),
                        store: wgpu::StoreOp::Store,
                    },
                })],
                depth_stencil_attachment: None,
                occlusion_query_set: None,
                timestamp_writes: None,
            });
        }

        // Create a render context for this frame
        let mut render_ctx = RenderContext::new(
            &self.queue,
            &mut self.staging_belt,
            &mut self.glyph_brush,
            self.size.width as f32,
            self.size.height as f32,
        );
        
        // --- Render base widgets to scene_buffer ---
        
        // Render the application title
        render_ctx.draw_text(
            "✨ tewduwu ✨",
            30.0,
            30.0,
            48.0,
            [1.0, 0.255, 0.639, 1.0] // Neon Pink
        );

        // Render the base TodoListWidget elements (without modals)
        self.todo_list_widget.render_base(&mut render_ctx);
        
        // Render instructions
        render_ctx.draw_text(
            "Press ESC to exit",
            30.0,
            self.size.height as f32 - 50.0,
            20.0,
            [0.5, 0.5, 0.5, 1.0]
        );
        
        // --- Draw Text to scene_buffer --- 
        self.glyph_brush
            .draw_queued(
                &self.device,
                &mut self.staging_belt,
                &mut encoder,
                &scene_view,
                self.size.width,
                self.size.height,
            )
            .expect("Draw queued glyphs failed");
        
        // --- Apply Bloom Effect ---
        self.bloom_effect.apply(&mut encoder, &scene_view, &bloom_view);
        
        // --- Render modals and other UI overlays ---
        // Draw the modals on top of the bloom result
        self.todo_list_widget.render_modals(&mut render_ctx);
        
        self.glyph_brush
            .draw_queued(
                &self.device,
                &mut self.staging_belt,
                &mut encoder,
                &bloom_view,
                self.size.width,
                self.size.height,
            )
            .expect("Draw queued modal glyphs failed");
        
        // --- Apply Neon Glow Effect and output to the screen ---
        self.neon_glow_effect.apply(&mut encoder, &bloom_view, &view);
        
        // Finish the staging belt BEFORE submitting the commands
        self.staging_belt.finish();
        
        // Submit commands and present
        self.queue.submit(std::iter::once(encoder.finish()));
        output.present();

        Ok(())
    }

    fn handle_mouse_input(&mut self, event: &WindowEvent) -> bool {
        match event {
            WindowEvent::CursorMoved { position, .. } => {
                // Convert screen coordinates to logical
                self.mouse_pos = (position.x as f32, position.y as f32);
                
                // Forward to TodoListWidget
                self.todo_list_widget.handle_mouse_move(self.mouse_pos.0, self.mouse_pos.1);
                true
            },
            WindowEvent::MouseWheel { delta, .. } => {
                let scroll_amount = match delta {
                    winit::event::MouseScrollDelta::LineDelta(_, y) => *y,
                    winit::event::MouseScrollDelta::PixelDelta(pos) => pos.y as f32 / 20.0,
                };
                
                // Forward scroll to TodoListWidget
                self.todo_list_widget.handle_mouse_wheel(scroll_amount);
                true
            },
            WindowEvent::MouseInput { state, button, .. } => {
                match (button, state) {
                    (winit::event::MouseButton::Left, winit::event::ElementState::Pressed) => {
                        // Pass screen dimensions to handle expanded item modals correctly
                        self.todo_list_widget.handle_mouse_down(
                            self.mouse_pos.0, 
                            self.mouse_pos.1, 
                            self.size.width as f32,
                            self.size.height as f32
                        );
                        true
                    },
                    (winit::event::MouseButton::Left, winit::event::ElementState::Released) => {
                        self.todo_list_widget.handle_mouse_up(self.mouse_pos.0, self.mouse_pos.1);
                        true
                    },
                    _ => false,
                }
            },
            _ => false,
        }
    }

    fn handle_keyboard_input(&mut self, event: &KeyEvent) -> bool {
        match &event.logical_key {
            winit::keyboard::Key::Character(c) if c.len() == 1 => {
                // Get the first character
                if let Some(ch) = c.chars().next() {
                    self.todo_list_widget.handle_char_input(ch);
                    true
                } else {
                    false
                }
            },
            winit::keyboard::Key::Named(key) => {
                if let Some(code) = key_to_keycode(key) {
                    self.todo_list_widget.handle_key_press(code);
                    true
                } else {
                    false
                }
            },
            _ => false,
        }
    }
}

// Helper function to convert winit::keyboard::NamedKey to winit::keyboard::KeyCode
fn key_to_keycode(key: &winit::keyboard::NamedKey) -> Option<winit::keyboard::KeyCode> {
    use winit::keyboard::{NamedKey, KeyCode};
    
    match key {
        NamedKey::Escape => Some(KeyCode::Escape),
        NamedKey::Enter => Some(KeyCode::Enter),
        NamedKey::Delete => Some(KeyCode::Delete),
        NamedKey::Backspace => Some(KeyCode::Backspace),
        NamedKey::ArrowUp => Some(KeyCode::ArrowUp),
        NamedKey::ArrowDown => Some(KeyCode::ArrowDown),
        NamedKey::ArrowLeft => Some(KeyCode::ArrowLeft),
        NamedKey::ArrowRight => Some(KeyCode::ArrowRight),
        NamedKey::Tab => Some(KeyCode::Tab),
        NamedKey::Space => Some(KeyCode::Space),
        NamedKey::Home => Some(KeyCode::Home),
        NamedKey::End => Some(KeyCode::End),
        _ => None,
    }
}

fn main() {
    // Setup logging with environment variables
    // Use RUST_LOG=debug if you want to see all logs
    env_logger::Builder::from_env(env_logger::Env::default().default_filter_or("info")).init();

    info!("Initializing tewduwu-neon (Rust)");

    // 1. Create Event Loop and Window Builder
    let event_loop = EventLoop::new().expect("Failed to create event loop");
    let window_builder = WindowBuilder::new() // Store builder, not window yet
        .with_title("tewduwu-neon (Rust)")
        .with_inner_size(winit::dpi::LogicalSize::new(1280, 720));

    // Initialize state outside the loop closure
    let mut state_option: Option<State> = None;

    info!("Entering event loop...");

    // 4. Main Event Loop
    // Closure takes event and event_loop_target
    event_loop.run(move |event, event_loop_target| {
        match event {
            Event::Resumed => {
                if state_option.is_none() {
                    // Clone the window_builder before building to avoid ownership issues
                    let window_arc = Arc::new(window_builder.clone().build(event_loop_target).expect("Failed to build window"));
                    info!("Window created successfully on Resumed event");
                    // Now that window is created, create the state
                    state_option = Some(pollster::block_on(State::new(window_arc.clone())));
                    info!("WGPU Initialized successfully on Resumed event.");
                }
            }
            Event::WindowEvent { event, window_id } => {
                if let Some(state) = state_option.as_mut() { 
                    if window_id == state.window_wrapper.window().id() {
                        match event {
                            WindowEvent::CloseRequested => {
                                info!("Close requested");
                                event_loop_target.exit();
                            }
                            WindowEvent::Resized(physical_size) => {
                                info!("Window resized to: {:?}", physical_size);
                                state.resize(physical_size);
                                
                                // Update UI components with new size
                                state.todo_list_widget.set_dimensions(
                                    physical_size.width as f32 - 100.0,
                                    physical_size.height as f32 - 200.0
                                );
                            }
                            WindowEvent::ScaleFactorChanged { .. } => {
                                info!("Scale factor changed.");
                                state.window_wrapper.window().request_redraw(); 
                            }
                            WindowEvent::KeyboardInput { event: key_event, .. } => {
                                if key_event.state == ElementState::Pressed {
                                    info!("Key pressed: {:?}", key_event.logical_key);
                                    
                                    // Check for ESC to exit first - highest priority
                                    if let winit::keyboard::Key::Named(winit::keyboard::NamedKey::Escape) = key_event.logical_key {
                                        info!("Escape key pressed, exiting application");
                                        event_loop_target.exit();
                                    } else {
                                        // Handle other keyboard input in the UI
                                        state.handle_keyboard_input(&key_event);
                                    }
                                }
                            }
                            
                            // Handle mouse input
                            WindowEvent::CursorMoved { .. } |
                            WindowEvent::MouseWheel { .. } |
                            WindowEvent::MouseInput { .. } => {
                                state.handle_mouse_input(&event);
                            }
                            
                            WindowEvent::RedrawRequested => {
                                state.update(0.016); // Assume ~60fps for now
                                match state.render() {
                                    Ok(_) => {}
                                    Err(wgpu::SurfaceError::Lost) => state.resize(state.size),
                                    Err(wgpu::SurfaceError::OutOfMemory) => event_loop_target.exit(),
                                    Err(e) => error!("Render error: {:?}", e),
                                }
                            }
                            _ => {}
                        }
                    }
                }
            }
            Event::LoopExiting => { // Handle cleanup if needed
                info!("Exiting event loop.");
            }
            Event::AboutToWait => {
                 if let Some(state) = state_option.as_mut() { 
                    state.staging_belt.recall();
                    state.window_wrapper.window().request_redraw();
                 }
            }
            _ => {}
        }
    })
    .expect("Event loop error");
}
