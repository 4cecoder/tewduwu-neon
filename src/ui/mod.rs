// UI module for todo list application

// Re-export core types
pub mod button;
pub mod text_input;
pub mod panel;
pub mod todo_item_widget;
pub mod todo_list_widget;
pub mod context;
pub mod theme;
pub mod renderer; // Post-processing renderer
pub mod widgets;

// UI components: Widget trait implementations
pub use button::Button;
pub use text_input::TextInput;
pub use panel::Panel;
pub use todo_item_widget::TodoItemWidget;
pub use todo_list_widget::TodoListWidget;
pub use context::RenderContext;
pub use theme::CyberpunkTheme;
pub use renderer::prelude::*; // Export the renderer types

/// Trait all UI widgets must implement
pub trait Widget {
    /// Update widget state
    fn update(&mut self, delta_time: f32);
    
    /// Render the widget
    fn render(&self, ctx: &mut RenderContext);
    
    /// Get position of widget
    fn position(&self) -> (f32, f32);
    
    /// Get dimensions of widget
    fn dimensions(&self) -> (f32, f32);
    
    /// Set position of widget
    fn set_position(&mut self, x: f32, y: f32);
    
    /// Set dimensions of widget
    fn set_dimensions(&mut self, width: f32, height: f32);
    
    /// Check if point is inside widget
    fn contains_point(&self, x: f32, y: f32) -> bool {
        let (widget_x, widget_y) = self.position();
        let (width, height) = self.dimensions();
        
        x >= widget_x && x <= widget_x + width && y >= widget_y && y <= widget_y + height
    }
}

// Export public types in a prelude module for convenient imports
pub mod prelude {
    pub use super::Widget;
    pub use super::Button;
    pub use super::TextInput;
    pub use super::Panel;
    pub use super::TodoItemWidget;
    pub use super::TodoListWidget;
    pub use super::RenderContext;
    pub use super::CyberpunkTheme;
    pub use super::widgets;
    pub use super::BloomEffect;
    pub use super::NeonGlowEffect;
}