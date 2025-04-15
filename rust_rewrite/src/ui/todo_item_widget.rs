use wgpu::Color;
use std::sync::Arc;
use crate::ui::{RenderContext, Widget, Button, Panel};
use crate::core::prelude::{TodoItem, Status, Priority};
use crate::ui::CyberpunkTheme;

/// A widget for displaying and interacting with a TodoItem
pub struct TodoItemWidget {
    x: f32,
    y: f32,
    width: f32,
    height: f32,
    pub todo_item: TodoItem,
    is_expanded: bool,
    is_hovered: bool,
    hierarchy_level: usize,  // 0 for root items, 1+ for nested items
    
    // UI components
    pub checkbox_button: Button,
    pub edit_button: Button,
    pub delete_button: Button,
    panel: Panel,
    
    // Callbacks
    pub on_status_change: Option<Arc<dyn Fn(Status) + Send + Sync>>,
    pub on_edit: Option<Arc<dyn Fn() + Send + Sync>>,
    pub on_delete: Option<Arc<dyn Fn() + Send + Sync>>,
    
    // Theme
    theme: CyberpunkTheme,
    
    // Close button bounds for modal (x, y, width, height)
    close_button_bounds: Option<(f32, f32, f32, f32)>,
    is_close_button_hovered: bool,
}

// Manual implementation of Clone for TodoItemWidget
impl Clone for TodoItemWidget {
    fn clone(&self) -> Self {
        let mut clone = Self {
            x: self.x,
            y: self.y,
            width: self.width,
            height: self.height,
            todo_item: self.todo_item.clone(),
            is_expanded: self.is_expanded,
            is_hovered: self.is_hovered,
            hierarchy_level: self.hierarchy_level,
            checkbox_button: self.checkbox_button.clone(),
            edit_button: self.edit_button.clone(),
            delete_button: self.delete_button.clone(),
            panel: self.panel.clone(),
            on_status_change: None, // Cannot clone function pointers easily
            on_edit: None,          // Cannot clone function pointers easily
            on_delete: None,        // Cannot clone function pointers easily
            theme: CyberpunkTheme::new(), // Theme is stateless, just create a new one
            close_button_bounds: self.close_button_bounds.clone(),
            is_close_button_hovered: self.is_close_button_hovered,
        };
        
        // Manually clone the function pointers by wrapping them
        if let Some(f) = &self.on_status_change {
            let f_clone = f.clone();
            clone.on_status_change = Some(f_clone);
        }
        
        if let Some(f) = &self.on_edit {
            let f_clone = f.clone();
            clone.on_edit = Some(f_clone);
        }
        
        if let Some(f) = &self.on_delete {
            let f_clone = f.clone();
            clone.on_delete = Some(f_clone);
        }
        
        clone
    }
}

impl TodoItemWidget {
    /// Create a new TodoItemWidget
    pub fn new(x: f32, y: f32, width: f32, todo_item: TodoItem) -> Self {
        let theme = CyberpunkTheme::new();
        let item_height = theme.todo_item_height(); // Use theme value instead of hardcoded
        
        // Create panel with theme values
        let panel_bg = match todo_item.priority() {
            Priority::High => Color {
                r: 0.18,
                g: 0.12,
                b: 0.14,
                a: 0.85,
            },
            Priority::Medium => Color {
                r: 0.16,
                g: 0.14,
                b: 0.12,
                a: 0.85,
            },
            Priority::Low => Color {
                r: 0.12,
                g: 0.16,
                b: 0.12,
                a: 0.85,
            },
            _ => Color {
                r: 0.12,
                g: 0.12,
                b: 0.16,
                a: 0.85,
            }
        };
        
        let panel = Panel::new(x, y, width, item_height)
            .with_background_color(panel_bg);
        
        // Calculate button size based on theme values
        let button_size = item_height * 0.5;
        
        // Create the checkbox button
        let checkbox_button = Button::new(
            x + 10.0,
            y + (item_height - button_size) / 2.0,
            button_size, 
            button_size, 
            if todo_item.is_completed() { "✓" } else { " " }
        ).with_text_color(Color {
            r: 0.0,
            g: 0.9,
            b: 0.6,
            a: 1.0,
        });
        
        // Create the edit button
        let edit_button = Button::new(
            x + width - 66.0,
            y + (item_height - button_size) / 2.0,
            button_size,
            button_size,
            "✎"
        ).with_text_color(Color {
            r: 0.4,
            g: 0.7,
            b: 1.0,
            a: 1.0,
        });
        
        let delete_button = Button::new(
            x + width - 36.0,
            y + (item_height - button_size) / 2.0,
            button_size,
            button_size,
            "✕"
        ).with_text_color(Color {
            r: 1.0,
            g: 0.3,
            b: 0.3,
            a: 1.0,
        });
        
        Self {
            x,
            y,
            width,
            height: item_height,
            todo_item,
            is_expanded: false,
            is_hovered: false,
            hierarchy_level: 0,
            checkbox_button,
            edit_button,
            delete_button,
            panel,
            on_status_change: None,
            on_edit: None,
            on_delete: None,
            theme,
            close_button_bounds: None,
            is_close_button_hovered: false,
        }
    }
    
    /// Set the hierarchy level for this item
    pub fn with_hierarchy_level(mut self, level: usize) -> Self {
        self.hierarchy_level = level;
        
        // Adjust the panel style based on the hierarchy level
        if level > 0 {
            let r = 0.12 + (level as f32 * 0.01).min(0.05);
            let g = 0.12 + (level as f32 * 0.01).min(0.05);
            let b = 0.22 + (level as f32 * 0.02).min(0.1);
            
            // More purple tint for nested items
            let bg_color = Color {
                r: r as f64,
                g: g as f64,
                b: b as f64,
                a: 0.9 + (level as f32 * 0.02).min(0.1) as f64,
            };
            self.panel = self.panel.with_background_color(bg_color);
        }
        
        self
    }
    
    /// Set callback for when status changes
    pub fn with_on_status_change<F: Fn(Status) + Send + Sync + 'static>(mut self, callback: F) -> Self {
        self.on_status_change = Some(Arc::new(callback));
        self
    }
    
    /// Set callback for when edit button is clicked
    pub fn with_on_edit<F: Fn() + Send + Sync + 'static>(mut self, callback: F) -> Self {
        self.on_edit = Some(Arc::new(callback));
        self
    }
    
    /// Set callback for when delete button is clicked
    pub fn with_on_delete<F: Fn() + Send + Sync + 'static>(mut self, callback: F) -> Self {
        self.on_delete = Some(Arc::new(callback));
        self
    }
    
    /// Check if the widget is currently expanded
    pub fn is_expanded(&self) -> bool {
        self.is_expanded
    }
    
    /// Toggle expanded state
    pub fn toggle_expanded(&mut self) {
        self.is_expanded = !self.is_expanded;
    }
    
    /// Handle mouse move event
    pub fn handle_mouse_move(&mut self, x: f32, y: f32) {
        // Update hover state
        self.is_hovered = self.contains_point(x, y);
        
        // Check if hovering over the close button
        if let Some((bx, by, bw, bh)) = self.close_button_bounds {
            self.is_close_button_hovered = x >= bx && x <= bx + bw && y >= by && y <= by + bh;
        } else {
            self.is_close_button_hovered = false;
        }
        
        // Update other button states
        self.checkbox_button.handle_mouse_move(x, y);
        self.edit_button.handle_mouse_move(x, y);
        self.delete_button.handle_mouse_move(x, y);
    }
    
    /// Handle mouse down event
    pub fn handle_mouse_down(&mut self, x: f32, y: f32) {
        // Propagate to child buttons
        self.checkbox_button.handle_mouse_down(x, y);
        self.edit_button.handle_mouse_down(x, y);
        self.delete_button.handle_mouse_down(x, y);
        
        // Toggle expanded state when clicking on the main item area
        // (but not on the buttons)
        if self.is_hovered && 
           !self.checkbox_button.contains_point(x, y) &&
           !self.edit_button.contains_point(x, y) &&
           !self.delete_button.contains_point(x, y) {
            self.toggle_expanded();
        }
    }
    
    /// Handle mouse up event
    pub fn handle_mouse_up(&mut self, x: f32, y: f32) {
        // Check if clicking the close button
        if self.is_expanded && self.is_close_button_hovered {
            if let Some((bx, by, bw, bh)) = self.close_button_bounds {
                if x >= bx && x <= bx + bw && y >= by && y <= by + bh {
                    self.is_expanded = false;
                    return;
                }
            }
        }
        
        // Check if checkbox was clicked
        let checkbox_clicked = self.checkbox_button.contains_point(x, y);
        let edit_clicked = self.edit_button.contains_point(x, y);
        let delete_clicked = self.delete_button.contains_point(x, y);
        
        // Propagate to child buttons
        self.checkbox_button.handle_mouse_up(x, y);
        self.edit_button.handle_mouse_up(x, y);
        self.delete_button.handle_mouse_up(x, y);
        
        // Handle checkbox click
        if checkbox_clicked {
            // Toggle completion status
            if self.todo_item.is_completed() {
                // Mark as not started (opposite of completed)
                self.todo_item.set_status(Status::NotStarted);
                self.checkbox_button = Button::new(
                    self.checkbox_button.position().0,
                    self.checkbox_button.position().1,
                    20.0,
                    20.0,
                    " "
                );
            } else {
                self.todo_item.mark_completed();
                self.checkbox_button = Button::new(
                    self.checkbox_button.position().0,
                    self.checkbox_button.position().1,
                    20.0,
                    20.0,
                    "✓"
                );
            }
            
            // Trigger callback
            if let Some(on_status_change) = &self.on_status_change {
                on_status_change(self.todo_item.status());
            }
        }
        
        // Handle edit click
        if edit_clicked {
            if let Some(on_edit) = &self.on_edit {
                on_edit();
            }
        }
        
        // Handle delete click
        if delete_clicked {
            if let Some(on_delete) = &self.on_delete {
                on_delete();
            }
        }
    }
    
    /// Get a color based on priority
    fn priority_color(&self) -> Color {
        match self.todo_item.priority() {
            Priority::High => Color { r: 1.0, g: 0.3, b: 0.3, a: 1.0 }, // Red for high
            Priority::Medium => Color { r: 1.0, g: 0.8, b: 0.0, a: 1.0 }, // Yellow for medium
            Priority::Low => Color { r: 0.3, g: 0.8, b: 0.3, a: 1.0 }, // Green for low
            _ => Color { r: 0.5, g: 0.5, b: 0.5, a: 0.5 }, // Grey for none
        }
    }

    /// Update the close button bounds (called during update)
    fn update_close_button_bounds(&mut self) {
        if self.is_expanded {
            // Only update when modal is visible
            let modal_width = self.width * 0.8;
            let close_button_size = 24.0;
            let close_button_x = self.x + (self.width - modal_width) / 2.0 + modal_width - close_button_size - 10.0;
            let close_button_y = self.y + self.theme.todo_item_height() + 5.0 + 10.0;
            
            self.close_button_bounds = Some((
                close_button_x,
                close_button_y,
                close_button_size,
                close_button_size
            ));
        }
    }

    /// Render only the base widget (first pass)
    pub fn render_base(&self, ctx: &mut RenderContext) {
        // Skip rendering the expanded view in the base pass
        if self.is_expanded {
            return;
        }

        // Get color as [f32; 4] (fix the type issue)
        let priority_color = match self.todo_item.priority() {
            Priority::High => [1.0, 0.3, 0.3, 1.0],    // Red
            Priority::Medium => [1.0, 0.8, 0.0, 1.0],  // Yellow/gold
            Priority::Low => [0.3, 0.8, 0.3, 1.0],     // Green
        };

        // Draw the card background
        ctx.draw_rect(
            self.x, self.y,
            self.width, self.height,
            self.theme.get_card_background_color(),
        );

        // Draw priority indicator
        ctx.draw_rect(
            self.x, self.y,
            5.0, self.height,
            priority_color,
        );

        // Draw hierarchy indent if needed
        if self.hierarchy_level > 0 {
            ctx.draw_rect(
                self.x + 5.0, self.y,
                self.hierarchy_level as f32 * 15.0, self.height, // Use fixed value 15.0 instead of method
                self.theme.get_hierarchy_indent_color(),
            );
        }

        // Draw checkbox
        self.checkbox_button.render(ctx);

        // Draw checkbox
        let checkbox_x = self.x + 10.0 + (self.hierarchy_level as f32 * 15.0);
        let checkbox_y = self.y + (self.height - 20.0) / 2.0;
        let checkbox_color = match self.todo_item.status() {
            Status::Completed => self.theme.get_checkbox_checked_color(),
            _ => self.theme.get_checkbox_unchecked_color(),
        };

        ctx.draw_rect(
            checkbox_x, checkbox_y,
            20.0, 20.0,
            checkbox_color,
        );

        if self.todo_item.status() == Status::Completed {
            // Draw checkmark
            ctx.draw_text(
                "✓",
                checkbox_x + 3.0, checkbox_y - 2.0,
                24.0,
                self.theme.get_text_color(),
            );
        }

        // Draw title
        let title_x = checkbox_x + 30.0;
        let title_y = self.y + (self.height - 24.0) / 2.0 - 2.0;
        let title_color = if self.todo_item.status() == Status::Completed {
            self.theme.get_completed_text_color()
        } else {
            self.theme.get_text_color()
        };

        ctx.draw_text(
            &self.todo_item.title(),
            title_x, title_y,
            24.0,
            title_color,
        );

        // Draw delete button
        let delete_btn_x = self.x + self.width - 30.0;
        let delete_btn_y = self.y + (self.height - 20.0) / 2.0;
        ctx.draw_text(
            "×",
            delete_btn_x, delete_btn_y - 2.0,
            24.0,
            self.theme.get_delete_button_color(),
        );

        // Draw edit button
        let edit_btn_x = delete_btn_x - 30.0;
        let edit_btn_y = delete_btn_y;
        ctx.draw_text(
            "✎",
            edit_btn_x, edit_btn_y - 2.0,
            20.0,
            self.theme.get_edit_button_color(),
        );

        // Draw expand button
        let expand_btn_x = edit_btn_x - 30.0;
        let expand_btn_y = edit_btn_y;
        let expand_symbol = if self.is_expanded { "▼" } else { "▶" };
        ctx.draw_text(
            expand_symbol,
            expand_btn_x, expand_btn_y - 2.0,
            16.0,
            self.theme.get_expand_button_color(),
        );

        // Draw due date if exists
        if let Some(due_date) = self.todo_item.due_date() {
            let date_str = time_to_string(due_date);
            let is_overdue = self.todo_item.is_overdue();
            let date_color = if is_overdue {
                self.theme.get_overdue_color()
            } else {
                self.theme.get_due_date_color()
            };

            // Due date icon
            ctx.draw_text(
                "🕒",
                expand_btn_x - 50.0, expand_btn_y - 2.0,
                16.0,
                date_color,
            );

            // Date text
            ctx.draw_text(
                &date_str,
                expand_btn_x - 30.0, expand_btn_y,
                16.0,
                date_color,
            );
        }
    }

    /// Render modal for expanded view (second pass)
    pub fn render_modal(&self, ctx: &mut RenderContext) {
        if !self.is_expanded {
            return;
        }

        // Draw modal overlay
        ctx.draw_rect(
            0.0, 0.0,
            ctx.width, ctx.height,
            self.theme.get_modal_overlay_color(),
        );

        // Calculate modal dimensions
        let modal_width = ctx.width.min(600.0);
        let modal_height = ctx.height.min(400.0);
        let modal_x = (ctx.width - modal_width) / 2.0;
        let modal_y = (ctx.height - modal_height) / 2.0;

        // Draw modal background
        ctx.draw_rect(
            modal_x, modal_y,
            modal_width, modal_height,
            self.theme.get_modal_bg_color(),
        );

        // Draw modal header
        ctx.draw_rect(
            modal_x, modal_y,
            modal_width, 40.0,
            self.theme.get_modal_header_color(),
        );

        // Draw title
        ctx.draw_text(
            &self.todo_item.title(),
            modal_x + 20.0, modal_y + 8.0,
            24.0,
            self.theme.get_modal_text_color(),
        );

        // Draw close button
        ctx.draw_text(
            "×",
            modal_x + modal_width - 30.0, modal_y + 8.0,
            24.0,
            self.theme.get_modal_close_button_color(),
        );

        // Draw content
        let content_y = modal_y + 60.0;

        // Draw status
        ctx.draw_text(
            &format!("Status: {:?}", self.todo_item.status()),
            modal_x + 20.0, content_y,
            18.0,
            self.theme.get_modal_text_color(),
        );

        // Draw priority
        ctx.draw_text(
            &format!("Priority: {:?}", self.todo_item.priority()),
            modal_x + 20.0, content_y + 30.0,
            18.0,
            self.theme.get_modal_text_color(),
        );

        // Draw created date
        let created_str = time_to_string(self.todo_item.created_at());
        ctx.draw_text(
            &format!("Created: {}", created_str),
            modal_x + 20.0, content_y + 60.0,
            18.0,
            self.theme.get_modal_text_color(),
        );

        // Draw due date if exists
        if let Some(due_date) = self.todo_item.due_date() {
            let date_str = time_to_string(due_date);
            let is_overdue = self.todo_item.is_overdue();
            let date_color = if is_overdue {
                self.theme.get_overdue_color()
            } else {
                self.theme.get_modal_text_color()
            };

            ctx.draw_text(
                &format!("Due: {}", date_str),
                modal_x + 20.0, content_y + 90.0,
                18.0,
                date_color,
            );
        }

        // Draw description
        ctx.draw_text(
            "Description:",
            modal_x + 20.0, content_y + 130.0,
            18.0,
            self.theme.get_modal_text_color(),
        );

        let description = if let Some(desc) = self.todo_item.description() {
            if desc.is_empty() {
                "No description".to_string()
            } else {
                desc.to_string()
            }
        } else {
            "No description".to_string()
        };

        ctx.draw_text(
            &description,
            modal_x + 20.0, content_y + 155.0,
            16.0,
            self.theme.get_modal_text_color(),
        );
    }

    /// Handle mouse down event on the modal
    pub fn handle_modal_mouse_down(&mut self, x: f32, y: f32, ctx_width: f32, ctx_height: f32) -> bool {
        if !self.is_expanded {
            return false;
        }

        // Calculate modal dimensions and position
        let modal_width = ctx_width * 0.6;
        let modal_height = ctx_height * 0.7;
        let modal_x = (ctx_width - modal_width) / 2.0;
        let modal_y = (ctx_height - modal_height) / 2.0;

        // Check if close button was clicked
        let close_btn_x = modal_x + modal_width - 30.0;
        let close_btn_y = modal_y + 8.0;
        
        if x >= close_btn_x - 10.0 && x <= close_btn_x + 20.0 &&
           y >= close_btn_y - 10.0 && y <= close_btn_y + 24.0 {
            self.is_expanded = false;
            return true;
        }

        // Check if clicked inside modal to consume the event
        if x >= modal_x && x <= modal_x + modal_width &&
           y >= modal_y && y <= modal_y + modal_height {
            return true;
        }

        // If clicked outside modal, close it
        self.is_expanded = false;
        return true;
    }
    
    /// Check if a point is inside the modal
    pub fn modal_contains_point(&self, x: f32, y: f32, ctx_width: f32, ctx_height: f32) -> bool {
        if !self.is_expanded {
            return false;
        }

        // Calculate modal dimensions and position
        let modal_width = ctx_width * 0.6;
        let modal_height = ctx_height * 0.7;
        let modal_x = (ctx_width - modal_width) / 2.0;
        let modal_y = (ctx_height - modal_height) / 2.0;

        // Check if point is inside modal
        x >= modal_x && x <= modal_x + modal_width &&
        y >= modal_y && y <= modal_y + modal_height
    }

    /// Render the widget (for backwards compatibility)
    pub fn render(&self, ctx: &mut RenderContext) {
        self.render_base(ctx);
        
        // Only render the modal if expanded and this is a legacy call
        if self.is_expanded {
            self.render_modal(ctx);
        }
    }
}

// Helper function to convert a timestamp to a string
fn time_to_string(timestamp: u64) -> String {
    // Basic formatting, could be improved with proper date/time library
    format!("{}", timestamp)
}

impl Widget for TodoItemWidget {
    fn update(&mut self, _delta_time: f32) {
        // Update child components
        self.checkbox_button.update(_delta_time);
        self.edit_button.update(_delta_time);
        self.delete_button.update(_delta_time);
        
        // Update close button bounds if expanded
        if self.is_expanded {
            self.update_close_button_bounds();
        }
    }
    
    fn render(&self, ctx: &mut RenderContext) {
        self.render(ctx);
    }
    
    fn position(&self) -> (f32, f32) {
        (self.x, self.y)
    }
    
    fn dimensions(&self) -> (f32, f32) {
        (self.width, self.height)
    }
    
    fn set_position(&mut self, x: f32, y: f32) {
        let dx = x - self.x;
        let dy = y - self.y;
        
        self.x = x;
        self.y = y;
        
        // Update child components
        let (checkbox_x, checkbox_y) = self.checkbox_button.position();
        self.checkbox_button.set_position(checkbox_x + dx, checkbox_y + dy);
        
        let (edit_x, edit_y) = self.edit_button.position();
        self.edit_button.set_position(edit_x + dx, edit_y + dy);
        
        let (delete_x, delete_y) = self.delete_button.position();
        self.delete_button.set_position(delete_x + dx, delete_y + dy);
        
        let (panel_x, panel_y) = self.panel.position();
        self.panel.set_position(panel_x + dx, panel_y + dy);
    }
    
    fn set_dimensions(&mut self, width: f32, height: f32) {
        self.width = width;
        self.height = height;
        
        // Update panel dimensions
        self.panel.set_dimensions(width, height);
        
        // Recalculate positions of buttons
        let button_size = height * 0.5;
        
        self.checkbox_button.set_position(
            self.x + 10.0,
            self.y + (height - button_size) / 2.0
        );
        
        self.edit_button.set_position(
            self.x + width - 66.0,
            self.y + (height - button_size) / 2.0
        );
        
        self.delete_button.set_position(
            self.x + width - 36.0,
            self.y + (height - button_size) / 2.0
        );
    }
    
    fn contains_point(&self, x: f32, y: f32) -> bool {
        x >= self.x && x <= self.x + self.width && y >= self.y && y <= self.y + self.height
    }
} 