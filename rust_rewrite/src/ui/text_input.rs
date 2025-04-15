use wgpu::Color;
use crate::ui::{RenderContext, Widget};
use winit::keyboard::KeyCode;

/// A text input widget
pub struct TextInput {
    x: f32,
    y: f32,
    width: f32,
    height: f32,
    text: String,
    placeholder: String,
    background_color: Color,
    text_color: Color,
    placeholder_color: Color,
    border_color: Color,
    border_width: f32,
    is_focused: bool,
    cursor_position: usize,
    cursor_blink_time: f32,
    cursor_visible: bool,
    max_length: Option<usize>,
    on_change: Option<Box<dyn Fn(&str)>>,
    on_submit: Option<Box<dyn Fn(&str)>>,
}

impl TextInput {
    /// Create a new text input
    pub fn new(x: f32, y: f32, width: f32, height: f32, placeholder: impl Into<String>) -> Self {
        Self {
            x,
            y,
            width,
            height,
            text: String::new(),
            placeholder: placeholder.into(),
            background_color: Color {
                r: 0.1,
                g: 0.1,
                b: 0.1,
                a: 1.0,
            },
            text_color: Color {
                r: 0.0,
                g: 0.9,
                b: 0.9,
                a: 1.0,
            },
            placeholder_color: Color {
                r: 0.4,
                g: 0.4,
                b: 0.4,
                a: 1.0,
            },
            border_color: Color {
                r: 0.0,
                g: 0.8,
                b: 0.8,
                a: 1.0,
            },
            border_width: 1.0,
            is_focused: false,
            cursor_position: 0,
            cursor_blink_time: 0.0,
            cursor_visible: true,
            max_length: None,
            on_change: None,
            on_submit: None,
        }
    }

    /// Set the background color
    pub fn with_background_color(mut self, color: Color) -> Self {
        self.background_color = color;
        self
    }

    /// Set the text color
    pub fn with_text_color(mut self, color: Color) -> Self {
        self.text_color = color;
        self
    }

    /// Set the placeholder color
    pub fn with_placeholder_color(mut self, color: Color) -> Self {
        self.placeholder_color = color;
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

    /// Set the maximum text length
    pub fn with_max_length(mut self, max_length: usize) -> Self {
        self.max_length = Some(max_length);
        self
    }

    /// Set the on_change handler
    pub fn with_on_change<F: Fn(&str) + 'static>(mut self, callback: F) -> Self {
        self.on_change = Some(Box::new(callback));
        self
    }

    /// Set the on_submit handler
    pub fn with_on_submit<F: Fn(&str) + 'static>(mut self, callback: F) -> Self {
        self.on_submit = Some(Box::new(callback));
        self
    }

    /// Get the current text
    pub fn text(&self) -> &str {
        &self.text
    }

    /// Set the text
    pub fn set_text(&mut self, text: impl Into<String>) {
        self.text = text.into();
        if let Some(max_length) = self.max_length {
            if self.text.len() > max_length {
                self.text.truncate(max_length);
            }
        }
        self.cursor_position = self.text.len();
        if let Some(on_change) = &self.on_change {
            on_change(&self.text);
        }
    }

    /// Get the focus state
    pub fn is_focused(&self) -> bool {
        self.is_focused
    }

    /// Set the focus state
    pub fn set_focused(&mut self, focused: bool) {
        self.is_focused = focused;
        if focused {
            self.cursor_position = self.text.len();
            self.cursor_visible = true;
            self.cursor_blink_time = 0.0;
        }
    }

    /// Check if a point is inside the text input
    pub fn contains_point(&self, x: f32, y: f32) -> bool {
        x >= self.x && x <= self.x + self.width && y >= self.y && y <= self.y + self.height
    }

    /// Handle mouse click
    pub fn handle_mouse_down(&mut self, x: f32, y: f32) {
        self.is_focused = self.contains_point(x, y);
        // TODO: Position cursor based on click position within text
        if self.is_focused {
            self.cursor_position = self.text.len();
        }
    }

    /// Handle character input
    pub fn handle_char_input(&mut self, c: char) {
        if !self.is_focused {
            return;
        }

        // Ignore control characters
        if c.is_control() {
            return;
        }

        // Check max length
        if let Some(max_length) = self.max_length {
            if self.text.len() >= max_length {
                return;
            }
        }

        // Insert character at cursor position
        self.text.insert(self.cursor_position, c);
        self.cursor_position += 1;

        // Trigger on_change
        if let Some(on_change) = &self.on_change {
            on_change(&self.text);
        }
    }

    /// Handle keyboard input
    pub fn handle_key_press(&mut self, key: KeyCode) {
        if !self.is_focused {
            return;
        }

        match key {
            KeyCode::Backspace => {
                if self.cursor_position > 0 {
                    self.text.remove(self.cursor_position - 1);
                    self.cursor_position -= 1;
                    if let Some(on_change) = &self.on_change {
                        on_change(&self.text);
                    }
                }
            }
            KeyCode::Delete => {
                if self.cursor_position < self.text.len() {
                    self.text.remove(self.cursor_position);
                    if let Some(on_change) = &self.on_change {
                        on_change(&self.text);
                    }
                }
            }
            KeyCode::ArrowLeft => {
                if self.cursor_position > 0 {
                    self.cursor_position -= 1;
                }
            }
            KeyCode::ArrowRight => {
                if self.cursor_position < self.text.len() {
                    self.cursor_position += 1;
                }
            }
            KeyCode::Home => {
                self.cursor_position = 0;
            }
            KeyCode::End => {
                self.cursor_position = self.text.len();
            }
            KeyCode::Enter => {
                if let Some(on_submit) = &self.on_submit {
                    on_submit(&self.text);
                }
            }
            KeyCode::Escape => {
                self.is_focused = false;
            }
            _ => {}
        }
    }
}

impl Clone for TextInput {
    fn clone(&self) -> Self {
        // We can't clone the callbacks directly, so we create a new instance without them
        Self {
            x: self.x,
            y: self.y,
            width: self.width,
            height: self.height,
            text: self.text.clone(),
            placeholder: self.placeholder.clone(),
            background_color: self.background_color,
            text_color: self.text_color,
            placeholder_color: self.placeholder_color,
            border_color: self.border_color,
            border_width: self.border_width,
            is_focused: self.is_focused,
            cursor_position: self.cursor_position,
            cursor_blink_time: self.cursor_blink_time,
            cursor_visible: self.cursor_visible,
            max_length: self.max_length,
            on_change: None, // Can't clone the callbacks
            on_submit: None, // Can't clone the callbacks
        }
    }
}

impl Widget for TextInput {
    fn update(&mut self, delta_time: f32) {
        // Update cursor blink
        if self.is_focused {
            self.cursor_blink_time += delta_time;
            if self.cursor_blink_time >= 0.5 {
                self.cursor_blink_time = 0.0;
                self.cursor_visible = !self.cursor_visible;
            }
        } else {
            self.cursor_visible = false;
        }
    }

    fn render(&self, ctx: &mut RenderContext) {
        // TODO: Draw text input background and border
        // For now, just draw the text/placeholder and cursor

        // Calculate text position
        let text_x = self.x + 5.0;  // Small padding
        let text_y = self.y + (self.height / 2.0) - 8.0;  // Rough vertical centering

        // Convert wgpu::Color to [f32; 4] array
        let placeholder_color_array = [
            self.placeholder_color.r as f32,
            self.placeholder_color.g as f32,
            self.placeholder_color.b as f32,
            self.placeholder_color.a as f32,
        ];

        let text_color_array = [
            self.text_color.r as f32,
            self.text_color.g as f32,
            self.text_color.b as f32,
            self.text_color.a as f32,
        ];

        // Draw the text or placeholder
        if self.text.is_empty() {
            ctx.draw_text(&self.placeholder, text_x, text_y, 16.0, placeholder_color_array);
        } else {
            ctx.draw_text(&self.text, text_x, text_y, 16.0, text_color_array);
        }

        // Draw cursor if focused and visible
        if self.is_focused && self.cursor_visible {
            // Calculate cursor position (assume monospace font with 8px width)
            let cursor_x = text_x + (self.cursor_position as f32 * 8.0);
            ctx.draw_text("|", cursor_x, text_y, 16.0, text_color_array);
        }
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