use wgpu::Queue;
use wgpu_glyph::{GlyphBrush, Section, Text};
use wgpu::util::StagingBelt;

/// Represents size information for text measurements
pub struct TextSize {
    pub width: f32,
    pub height: f32,
}

/// Context for rendering UI components
pub struct RenderContext<'a> {
    pub queue: &'a Queue,
    pub staging_belt: &'a mut StagingBelt,
    pub glyph_brush: &'a mut GlyphBrush<()>,
    pub width: f32,
    pub height: f32,
}

impl<'a> RenderContext<'a> {
    /// Create a new render context
    pub fn new(
        queue: &'a Queue,
        staging_belt: &'a mut StagingBelt,
        glyph_brush: &'a mut GlyphBrush<()>,
        width: f32,
        height: f32,
    ) -> Self {
        Self {
            queue,
            staging_belt,
            glyph_brush,
            width,
            height,
        }
    }
    
    /// Draw text at the specified position
    pub fn draw_text(&mut self, text: &str, x: f32, y: f32, size: f32, color: [f32; 4]) {
        let section = Section {
            screen_position: (x, y),
            bounds: (self.width, self.height),
            text: vec![Text::new(text)
                .with_color(color)
                .with_scale(size)],
            ..Section::default()
        };
        
        self.glyph_brush.queue(section);
    }
    
    /// Measure text dimensions (approximate)
    pub fn measure_text(&self, text: &str, size: f32) -> TextSize {
        // This is a very simple approximation
        // In a real app, you would use the font metrics to calculate this properly
        let char_width = size * 0.5; // Approximate width of a character
        let width = text.len() as f32 * char_width;
        let height = size;
        
        TextSize { width, height }
    }
    
    /// Alternative draw_text method that accepts tuple position and wgpu::Color
    pub fn draw_text_with_color(&mut self, text: &str, position: (f32, f32), size: f32, color: wgpu::Color) {
        self.draw_text(
            text,
            position.0,
            position.1,
            size,
            [color.r as f32, color.g as f32, color.b as f32, color.a as f32],
        );
    }

    /// Draw text with tuple position
    pub fn draw_text_tuple(&mut self, text: &str, position: (f32, f32), size: f32, color: [f32; 4]) {
        self.draw_text(text, position.0, position.1, size, color);
    }
    
    /// Draw text with tuple position and wgpu::Color
    pub fn draw_text_tuple_color(&mut self, text: &str, position: (f32, f32), size: f32, color: wgpu::Color) {
        self.draw_text_with_color(text, position, size, color);
    }
    
    /// Draw a colored rectangle
    pub fn draw_rect(&mut self, x: f32, y: f32, width: f32, height: f32, color: [f32; 4]) {
        // Create a "block" character that will be repeated to fill the rectangle
        let block = "█";
        
        // Calculate how many blocks we need to fill the width (assuming monospace font)
        // This is an approximation and may need adjustment based on font size
        let font_size = height;
        let char_width = font_size * 0.6; // Approximate width of a character
        let chars_needed = (width / char_width).ceil() as usize;
        
        // Create a string of blocks
        let block_row = block.repeat(chars_needed);
        
        // Draw the block string with the specified color
        self.draw_text(
            &block_row,
            x,
            y,
            font_size,
            color,
        );
    }
    
    /// Draw a colored rectangle with wgpu::Color
    pub fn draw_rect_with_color(&mut self, x: f32, y: f32, width: f32, height: f32, color: wgpu::Color) {
        self.draw_rect(
            x,
            y,
            width,
            height,
            [color.r as f32, color.g as f32, color.b as f32, color.a as f32],
        );
    }
    
    /// Draw a line from (x1, y1) to (x2, y2) with the specified thickness and color
    pub fn draw_line(&mut self, x1: f32, y1: f32, x2: f32, y2: f32, thickness: f32, color: [f32; 4]) {
        // Calculate the length of the line
        let dx = x2 - x1;
        let dy = y2 - y1;
        let length = (dx * dx + dy * dy).sqrt();
        
        if length < 0.01 {
            return; // Line is too short to draw
        }
        
        // Calculate the number of steps to draw
        let steps = (length / (thickness * 0.5)).max(1.0) as usize;
        
        // Draw a series of small rectangles to represent the line
        for i in 0..=steps {
            let t = i as f32 / steps as f32;
            let x = x1 + t * dx;
            let y = y1 + t * dy;
            
            // Draw a small rect at this position
            self.draw_rect(
                x - thickness / 2.0,
                y - thickness / 2.0,
                thickness,
                thickness,
                color
            );
        }
    }
    
    /// Draw a circle at (x, y) with the specified radius and color
    pub fn draw_circle(&mut self, x: f32, y: f32, radius: f32, color: [f32; 4]) {
        // Approximate a circle using rectangles
        
        // For larger circles, we need finer step to make it smoother
        let step_size = if radius < 10.0 {
            1.0
        } else if radius < 20.0 {
            0.5
        } else {
            0.25
        };
        
        // For each y offset from center
        for y_offset in (-radius as i32)..=(radius as i32) {
            let y_pos = y + y_offset as f32;
            let y_delta = y_pos - y;
            
            // Calculate width at this y using circle equation: x² + y² = r²
            // For a given y, x = sqrt(r² - y²)
            let half_width = (radius * radius - y_delta * y_delta).sqrt().max(0.0);
            
            if half_width > 0.0 {
                // Draw a horizontal line representing this portion of the circle
                self.draw_rect(
                    x - half_width,
                    y_pos,
                    half_width * 2.0,
                    step_size,
                    color
                );
            }
        }
    }
    
    /// Draw a colored rectangle with tuple coordinates
    pub fn draw_rect_tuple(&mut self, position: (f32, f32), size: (f32, f32), color: [f32; 4], corner_radius: f32) {
        self.draw_rect(
            position.0,
            position.1,
            size.0,
            size.1,
            color,
        );
    }
    
    /// Draw a rectangle with tuples and wgpu::Color
    pub fn draw_rect_tuple_color(&mut self, position: (f32, f32), size: (f32, f32), color: wgpu::Color, corner_radius: f32) {
        self.draw_rect_with_color(
            position.0,
            position.1,
            size.0,
            size.1,
            color,
        );
    }
    
    /// Set a clipping rectangle for subsequent rendering
    pub fn scissor_rect(&mut self, position: (f32, f32), size: (f32, f32)) {
        // In real implementation this would set up scissor rectangle
        // For now just call push_clip_rect to maintain the API
        self.push_clip_rect(position.0, position.1, size.0, size.1);
    }
    
    /// Reset scissor rectangle to full screen
    pub fn reset_scissor(&mut self) {
        // In real implementation this would clear the scissor rectangle
        // For now just call pop_clip_rect to maintain the API
        self.pop_clip_rect();
    }
    
    /// Push a clipping rectangle onto the stack (this is a stub for now)
    pub fn push_clip_rect(&mut self, x: f32, y: f32, width: f32, height: f32) {
        // In a real implementation, this would set up a scissor rectangle
        // or another clipping method, but for now it's just a stub
        // since the current renderer doesn't support clipping
    }
    
    /// Pop a clipping rectangle from the stack (this is a stub for now)
    pub fn pop_clip_rect(&mut self) {
        // In a real implementation, this would restore the previous
        // clipping rectangle, but for now it's just a stub
    }
} 