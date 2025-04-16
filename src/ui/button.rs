use wgpu::Color;
use std::sync::Arc;
use crate::ui::{RenderContext, Widget};

/// A clickable button widget
pub struct Button {
    x: f32,
    y: f32,
    width: f32,
    height: f32,
    label: String,
    background_color: Color,
    hover_color: Color,
    text_color: Color,
    border_color: Color,
    border_width: f32,
    is_hovered: bool,
    is_pressed: bool,
    on_click: Option<Arc<dyn Fn() + Send + Sync>>,
}

impl Clone for Button {
    fn clone(&self) -> Self {
        Button {
            x: self.x,
            y: self.y,
            width: self.width,
            height: self.height,
            label: self.label.clone(),
            background_color: self.background_color,
            hover_color: self.hover_color,
            text_color: self.text_color,
            border_color: self.border_color,
            border_width: self.border_width,
            is_hovered: self.is_hovered,
            is_pressed: self.is_pressed,
            on_click: self.on_click.clone(),
        }
    }
}

impl Button {
    /// Create a new button
    pub fn new(x: f32, y: f32, width: f32, height: f32, label: impl Into<String>) -> Self {
        Self {
            x,
            y,
            width,
            height,
            label: label.into(),
            background_color: Color {
                r: 0.2,
                g: 0.2,
                b: 0.2,
                a: 1.0,
            },
            hover_color: Color {
                r: 0.3,
                g: 0.3,
                b: 0.3,
                a: 1.0,
            },
            text_color: Color {
                r: 0.0,
                g: 0.9,
                b: 0.9,
                a: 1.0,
            },
            border_color: Color {
                r: 0.0,
                g: 0.8,
                b: 0.8,
                a: 1.0,
            },
            border_width: 1.0,
            is_hovered: false,
            is_pressed: false,
            on_click: None,
        }
    }

    /// Set the background color
    pub fn with_background_color(mut self, color: Color) -> Self {
        self.background_color = color;
        self
    }

    /// Set the hover color
    pub fn with_hover_color(mut self, color: Color) -> Self {
        self.hover_color = color;
        self
    }

    /// Set the text color
    pub fn with_text_color(mut self, color: Color) -> Self {
        self.text_color = color;
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

    /// Set the on_click handler
    pub fn with_on_click<F: Fn() + Send + Sync + 'static>(mut self, callback: F) -> Self {
        self.on_click = Some(Arc::new(callback));
        self
    }

    /// Check if a point is inside the button
    pub fn contains_point(&self, x: f32, y: f32) -> bool {
        x >= self.x && x <= self.x + self.width && y >= self.y && y <= self.y + self.height
    }

    /// Handle mouse move event
    pub fn handle_mouse_move(&mut self, x: f32, y: f32) {
        self.is_hovered = self.contains_point(x, y);
    }

    /// Handle mouse button press
    pub fn handle_mouse_down(&mut self, x: f32, y: f32) {
        if self.contains_point(x, y) {
            self.is_pressed = true;
        }
    }

    /// Handle mouse button release
    pub fn handle_mouse_up(&mut self, x: f32, y: f32) {
        if self.is_pressed && self.contains_point(x, y) {
            if let Some(on_click) = &self.on_click {
                on_click();
            }
        }
        self.is_pressed = false;
    }
}

impl Widget for Button {
    fn update(&mut self, _delta_time: f32) {
        // Update logic if needed
    }

    fn render(&self, ctx: &mut RenderContext) {
        // TODO: Draw button background, border and text
        // For now, just draw the label as text
        let _color = if self.is_pressed {
            // Darker when pressed
            Color {
                r: self.background_color.r * 0.8,
                g: self.background_color.g * 0.8,
                b: self.background_color.b * 0.8,
                a: self.background_color.a,
            }
        } else if self.is_hovered {
            self.hover_color
        } else {
            self.background_color
        };

        // Future: Draw background and border here

        // Draw the button text
        let text_x = self.x + (self.width / 2.0) - (self.label.len() as f32 * 8.0 / 2.0);  // Rough centering
        let text_y = self.y + (self.height / 2.0) - 8.0;  // Rough centering
        
        // Convert wgpu::Color to [f32; 4] array
        let text_color = [
            self.text_color.r as f32,
            self.text_color.g as f32,
            self.text_color.b as f32,
            self.text_color.a as f32,
        ];
        
        ctx.draw_text(&self.label, text_x, text_y, 16.0, text_color);
    }

    fn position(&self) -> (f32, f32) {
        (self.x, self.y)
    }

    fn dimensions(&self) -> (f32, f32) {
        (self.width, self.height)
    }

    fn set_position(&mut self, x: f32, y: f32) {
        self.x = x;
        self.y = y;
    }

    fn set_dimensions(&mut self, width: f32, height: f32) {
        self.width = width;
        self.height = height;
    }
} 