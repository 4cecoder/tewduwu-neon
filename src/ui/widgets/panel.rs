use crate::ui::{Widget, context::RenderContext, theme::CyberpunkTheme};
use wgpu_glyph::{Section, Text};

/// A panel widget that serves as a container for other widgets
pub struct Panel {
    x: f32,
    y: f32,
    width: f32,
    height: f32,
    title: Option<String>,
    children: Vec<Box<dyn Widget>>,
    theme: CyberpunkTheme,
}

impl Panel {
    /// Create a new panel at position (x, y) with given dimensions
    pub fn new(x: f32, y: f32, width: f32, height: f32) -> Self {
        Self {
            x,
            y,
            width,
            height,
            title: None,
            children: Vec::new(),
            theme: CyberpunkTheme::default(),
        }
    }

    /// Set the panel's title
    pub fn with_title(mut self, title: impl Into<String>) -> Self {
        self.title = Some(title.into());
        self
    }

    /// Add a child widget to the panel
    pub fn add_child(&mut self, widget: Box<dyn Widget>) {
        self.children.push(widget);
    }
}

impl Widget for Panel {
    fn update(&mut self, _delta_time: f32) {
        // Update all child widgets
        for child in &mut self.children {
            child.update(_delta_time);
        }
    }

    fn render(&self, context: &mut RenderContext) {
        // TODO: In a real implementation, we would draw the panel background
        // For now, we'll just handle the text rendering since we don't have a drawing API yet
        
        // Render the panel title if it exists
        if let Some(title) = &self.title {
            let text_size = self.theme.header_text_size();
            
            // Queue title text
            context.queue_text(
                self.x + self.theme.panel_padding()[0],
                self.y + text_size,
                title.as_str(),
                text_size,
                self.theme.bright_text(),
            );
        }
        
        // Render all child widgets
        for child in &self.children {
            child.render(context);
        }
    }

    fn dimensions(&self) -> (f32, f32) {
        (self.width, self.height)
    }

    fn position(&self) -> (f32, f32) {
        (self.x, self.y)
    }

    fn set_position(&mut self, x: f32, y: f32) {
        self.x = x;
        self.y = y;
    }

    fn set_dimensions(&mut self, width: f32, height: f32) {
        self.width = width;
        self.height = height;
    }

    fn contains_point(&self, point_x: f32, point_y: f32) -> bool {
        point_x >= self.x && 
        point_x <= self.x + self.width && 
        point_y >= self.y && 
        point_y <= self.y + self.height
    }
} 