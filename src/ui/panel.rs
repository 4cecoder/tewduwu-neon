use wgpu::Color;
use std::sync::Arc;
use crate::ui::{RenderContext, Widget};

/// A basic panel widget that can contain other widgets
pub struct Panel {
    x: f32,
    y: f32,
    width: f32,
    height: f32,
    background_color: Color,
    border_color: Color,
    border_width: f32,
    children: Vec<Arc<dyn Widget + Send + Sync>>,
}

impl Clone for Panel {
    fn clone(&self) -> Self {
        Panel {
            x: self.x,
            y: self.y,
            width: self.width,
            height: self.height,
            background_color: self.background_color,
            border_color: self.border_color,
            border_width: self.border_width,
            children: self.children.clone(),
        }
    }
}

impl Panel {
    /// Create a new panel
    pub fn new(x: f32, y: f32, width: f32, height: f32) -> Self {
        Self {
            x,
            y,
            width,
            height,
            background_color: Color {
                r: 0.1,
                g: 0.1,
                b: 0.1,
                a: 0.8,
            },
            border_color: Color {
                r: 0.0,
                g: 0.8,
                b: 0.8,
                a: 1.0,
            },
            border_width: 2.0,
            children: Vec::new(),
        }
    }

    /// Set the background color
    pub fn with_background_color(mut self, color: Color) -> Self {
        self.background_color = color;
        self
    }

    /// Set the border color
    pub fn with_border_color(mut self, color: Color) -> Self {
        self.border_color = color;
        self
    }

    /// Set the border width
    pub fn with_border_width(mut self, width: f32) -> Self {
        self.border_width = width;
        self
    }

    /// Add a child widget to this panel
    pub fn add_child<W: Widget + Send + Sync + 'static>(&mut self, widget: W) {
        self.children.push(Arc::new(widget));
    }
}

impl Widget for Panel {
    fn update(&mut self, _delta_time: f32) {
        // Update all children
        for _child_arc in &self.children {
            // Unfortunately we can't update children through Arc references directly
            // This would require interior mutability in the Widget trait
            // For now, we just don't update children through Panels
        }
    }

    fn render(&self, ctx: &mut RenderContext) {
        // TODO: Draw panel background and borders using a renderer
        // For now, we can use placeholder logic
        
        // Render all children
        for child_arc in &self.children {
            child_arc.render(ctx);
        }
    }

    fn position(&self) -> (f32, f32) {
        (self.x, self.y)
    }

    fn dimensions(&self) -> (f32, f32) {
        (self.width, self.height)
    }

    /// Set the position of the panel and adjust children appropriately
    fn set_position(&mut self, x: f32, y: f32) {
        // Calculate offset for children
        let dx = x - self.x;
        let dy = y - self.y;
        
        // Update our position
        self.x = x;
        self.y = y;
        
        // Note: Since we have Arc references to children, we can't directly update them
        // In a real implementation, we would need to use interior mutability or
        // other patterns to allow updating children's positions
        
        // Log the position change for debugging
        log::debug!("Panel moved by ({}, {})", dx, dy);
    }

    fn set_dimensions(&mut self, width: f32, height: f32) {
        self.width = width;
        self.height = height;
    }
} 