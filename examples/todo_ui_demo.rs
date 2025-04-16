use log::{error, info};
use std::sync::{Arc, Mutex};
use tewduwu::core::prelude::*;
use tewduwu::ui::prelude::*;
use winit::{
    event::{Event, WindowEvent, ElementState, MouseButton},
    event_loop::EventLoop,
    window::WindowBuilder,
};

fn main() {
    // Initialize logging
    env_logger::init();
    info!("Starting todo UI demo");

    // Create event loop and window
    let event_loop = EventLoop::new().expect("Failed to create event loop");
    let window = Arc::new(WindowBuilder::new()
        .with_title("Todo UI Demo")
        .with_inner_size(winit::dpi::LogicalSize::new(800, 600))
        .build(&event_loop)
        .expect("Failed to create window"));

    // Get initial window size
    let initial_size = window.inner_size();

    // Create a sample todo list
    let mut todo_list = TodoList::new("Demo Tasks");
    let task1_id = todo_list.create_item("Task 1 - High Priority");
    let task2_id = todo_list.create_item("Task 2 - Medium Priority");
    todo_list.create_item("Task 3 - Low Priority");

    // Set priority after creation
    if let Some(item) = todo_list.get_item_mut(task1_id) {
        item.set_priority(Priority::High);
    }
    if let Some(item) = todo_list.get_item_mut(task2_id) {
        item.set_priority(Priority::Medium);
    }
    
    // Wrap the todo_list in Arc<Mutex>
    let todo_list_arc = Arc::new(Mutex::new(todo_list));

    // Create the TodoListWidget using initial size
    let mut todo_list_widget = TodoListWidget::new(
        10.0, 
        10.0, 
        initial_size.width as f32 - 20.0,
        initial_size.height as f32 - 20.0,
        todo_list_arc.clone()
    )
    .with_on_status_change(|item: TodoItem| {
        info!("Item status changed: {:?} - {}", item.id(), item.status());
    })
    .with_on_edit(|item: TodoItem| {
        info!("Item edit requested: {:?} - {}", item.id(), item.title());
        // In a real app, you would open an edit dialog here
    })
    .with_on_delete(|item: TodoItem| {
        info!("Item delete requested: {:?} - {}", item.id(), item.title());
    });
    
    // Track mouse position
    let mut last_mouse_pos: Option<(f32, f32)> = None;

    // Start event loop
    event_loop.run(move |event, elwt| {
        match event {
            Event::WindowEvent { event, window_id } if window_id == window.id() => {
                // Get current size for handle_mouse_down
                let current_size = window.inner_size(); 
                match event {
                    WindowEvent::CloseRequested => {
                        info!("Window close requested");
                        elwt.exit();
                    }
                    WindowEvent::KeyboardInput { 
                        event: winit::event::KeyEvent { 
                            logical_key: winit::keyboard::Key::Named(winit::keyboard::NamedKey::Escape),
                            state: ElementState::Pressed,
                            .. 
                        },
                        ..
                    } => {
                        info!("ESC pressed, exiting");
                        elwt.exit();
                    }
                    WindowEvent::Resized(physical_size) => {
                        // Handle window resize - update widget dimensions
                        todo_list_widget.set_dimensions(physical_size.width as f32 - 20.0, physical_size.height as f32 - 20.0);
                    }
                    WindowEvent::CursorMoved { position, .. } => {
                        last_mouse_pos = Some((position.x as f32, position.y as f32));
                        if let Some(pos) = last_mouse_pos {
                            todo_list_widget.handle_mouse_move(pos.0, pos.1);
                        }
                    }
                    WindowEvent::MouseWheel { delta, .. } => {
                        let scroll_amount = match delta {
                            winit::event::MouseScrollDelta::LineDelta(_, y) => y,
                            winit::event::MouseScrollDelta::PixelDelta(pos) => pos.y as f32 / 20.0,
                        };
                        todo_list_widget.handle_mouse_wheel(scroll_amount);
                    }
                    WindowEvent::MouseInput { state: ElementState::Pressed, button: MouseButton::Left, .. } => {
                        if let Some(pos) = last_mouse_pos {
                            // Pass current width and height to handle_mouse_down
                            todo_list_widget.handle_mouse_down(pos.0, pos.1, current_size.width as f32, current_size.height as f32);
                        }
                    }
                    WindowEvent::MouseInput { state: ElementState::Released, button: MouseButton::Left, .. } => {
                        if let Some(pos) = last_mouse_pos {
                            todo_list_widget.handle_mouse_up(pos.0, pos.1);
                        }
                    }
                    WindowEvent::RedrawRequested => {
                        todo_list_widget.update(0.016); // ~60fps
                        // In a real application, we would render here
                        info!("Redraw requested (no actual rendering in this demo)");
                    }
                    _ => {}
                }
            }
            Event::AboutToWait => {
                // Request redraw continuously for this demo
                window.request_redraw();
            }
            _ => {}
        }
    }).expect("Event loop error");
} 