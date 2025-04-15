use crate::ui::{RenderContext, Widget, Button, Panel, TextInput, CyberpunkTheme};
use crate::ui::todo_item_widget::TodoItemWidget;
use crate::core::prelude::{TodoList, TodoItem, Status, Priority};
use uuid::Uuid;
use std::sync::Arc;
use std::sync::Mutex;

/// Filter settings for displaying todo items
#[derive(Debug, Clone, Copy, PartialEq)]
pub enum StatusFilter {
    All,
    Active,
    Completed,
}

/// Filter settings for priority
#[derive(Debug, Clone, Copy, PartialEq)]
pub enum PriorityFilter {
    All,
    High,
    Medium,
    Low,
}

/// Type of filter applied
#[derive(Debug, Clone, Copy, PartialEq)]
pub enum FilterType {
    None,
    Title,
    Description,
    Status,
    Priority,
    Combined,
}

/// Convert [f32; 4] RGBA values to wgpu::Color
fn to_color(rgba: [f32; 4]) -> wgpu::Color {
    wgpu::Color {
        r: rgba[0] as f64,
        g: rgba[1] as f64,
        b: rgba[2] as f64,
        a: rgba[3] as f64,
    }
}

/// A widget for displaying and managing a TodoList
pub struct TodoListWidget {
    x: f32,
    y: f32,
    width: f32,
    height: f32,
    todo_list: Arc<Mutex<TodoList>>,
    
    // UI components
    panel: Panel,
    add_button: Button,
    title_input: TextInput,
    filter_buttons: Vec<Button>,
    search_input: TextInput,
    
    // Scrolling
    scroll_offset: f32,
    max_scroll: f32,
    
    // Todo item widgets
    todo_item_widgets: Vec<Arc<Mutex<TodoItemWidget>>>,
    
    // Filter state
    show_completed: bool,
    filter_priority: Option<Priority>,
    filter_status: Option<Status>,
    search_text: String,
    
    // Callbacks
    on_item_status_change: Option<Arc<dyn Fn(TodoItem) + Send + Sync>>,
    on_item_edit: Option<Arc<dyn Fn(TodoItem) + Send + Sync>>,
    on_item_delete: Option<Arc<dyn Fn(TodoItem) + Send + Sync>>,
    
    // Theme
    theme: CyberpunkTheme,
    
    // Track which item has a modal open (if any)
    modal_open_index: Option<usize>,
    
    // New fields
    expanded_items: Vec<usize>, // Track expanded item indices
    visible_items: Vec<usize>,
    filter_value: String,
    filter_type: FilterType,
    status_filter: Option<Status>,
    priority_filter: Option<Priority>,
}

impl TodoListWidget {
    /// Create a new TodoListWidget with the given todo list and position
    pub fn new(x: f32, y: f32, width: f32, height: f32, todo_list: Arc<Mutex<TodoList>>) -> Self {
        let theme = CyberpunkTheme::new();
        
        // Create panel
        let panel = Panel::new(x, y, width, height)
            .with_background_color(to_color(theme.panel_background()))
            .with_border_color(to_color(theme.border()));
        
        // Create add button
        let button_width = 80.0;
        let button_height = 30.0;
        let button_padding = 10.0;
        let add_button = Button::new(
            x + width - button_width - button_padding,
            y + button_padding,
            button_width,
            button_height,
            "Add Task"
        ).with_text_color(to_color(theme.bright_text()))
         .with_background_color(to_color(theme.neon_pink()));
        
        // Create title input
        let input_width = width - button_width - button_padding * 3.0;
        let title_input = TextInput::new(
            x + button_padding,
            y + button_padding,
            input_width,
            button_height,
            "New task..."
        ).with_text_color(to_color(theme.bright_text()))
         .with_background_color(to_color(theme.background()))
         .with_border_color(to_color(theme.border()));
        
        // Create filter buttons
        let filter_buttons = Self::create_filter_buttons(x, y, width, &theme);
        
        // Create search input
        let search_input_width = 200.0;
        let search_input = TextInput::new(
            x + width - search_input_width - button_padding,
            y + button_padding * 2.0 + button_height,
            search_input_width,
            button_height,
            "Search..."
        ).with_text_color(to_color(theme.bright_text()))
         .with_background_color(to_color(theme.background()))
         .with_border_color(to_color(theme.border()));
        
        // Calculate the appropriate area for todo items
        let top_controls_height = button_height + button_padding * 2.0; // Add button + title input
        let filter_controls_height = button_height + button_padding; // Filter controls
        
        let mut widget = Self {
            x,
            y,
            width,
            height,
            todo_list,
            panel,
            add_button,
            title_input,
            filter_buttons,
            search_input,
            scroll_offset: 0.0,
            max_scroll: 0.0,
            todo_item_widgets: Vec::new(),
            show_completed: true,
            filter_priority: None,
            filter_status: None,
            search_text: String::new(),
            on_item_status_change: None,
            on_item_edit: None,
            on_item_delete: None,
            theme,
            modal_open_index: None,
            expanded_items: Vec::new(),
            visible_items: Vec::new(),
            filter_value: String::new(),
            filter_type: FilterType::None,
            status_filter: None,
            priority_filter: None,
        };
        
        // Generate initial todo item widgets
        widget.update_todo_items();
        
        widget
    }
    
    /// Get the todo list
    pub fn todo_list(&self) -> Arc<Mutex<TodoList>> {
        self.todo_list.clone()
    }
    
    /// Set a new todo_list
    pub fn set_todo_list(&mut self, todo_list: Arc<Mutex<TodoList>>) {
        self.todo_list = todo_list;
        
        // Reset filters and search
        self.show_completed = true;
        self.filter_priority = None;
        self.filter_status = None;
        self.search_text = String::new();
        self.search_input.set_text("Search...");
        
        // Regenerate todo item widgets
        self.update_todo_items();
    }
    
    /// Create filter buttons with proper layout
    fn create_filter_buttons(x: f32, y: f32, width: f32, theme: &CyberpunkTheme) -> Vec<Button> {
        let button_height = 30.0;
        let button_padding = 10.0;
        let button_width = 140.0;
        let button_margin = 5.0;
        let button_y = y + button_padding * 2.0 + button_height;
        let mut buttons = Vec::new();
        
        // All button
        buttons.push(
            Button::new(
                x + button_padding,
                button_y,
                button_width,
                button_height,
                "All Tasks"
            ).with_text_color(to_color(theme.bright_text()))
             .with_background_color(to_color(theme.filter_button_selected_bg())) // Start with "All" selected
        );
        
        // Active button
        buttons.push(
            Button::new(
                x + button_padding + button_width + button_margin,
                button_y,
                button_width,
                button_height,
                "Active"
            ).with_text_color(to_color(theme.bright_text()))
             .with_background_color(to_color(theme.filter_button_bg()))
        );
        
        // Completed button
        buttons.push(
            Button::new(
                x + button_padding + (button_width + button_margin) * 2.0,
                button_y,
                button_width,
                button_height,
                "Completed"
            ).with_text_color(to_color(theme.bright_text()))
             .with_background_color(to_color(theme.filter_button_bg()))
        );
        
        buttons
    }
    
    /// Update the todo item widgets based on current state and filters
    fn update_todo_items(&mut self) {
        // Clear current todo item widgets
        self.todo_item_widgets.clear();
        
        // Get filtered items
        let items = {
            let todo_list = self.todo_list.lock().unwrap();
            self.filter_items(&todo_list.all_items())
        };
        
        // Calculate the appropriate area for todo items
        let top_controls_height = 30.0 + 10.0 * 2.0; // Add button + title input
        let filter_controls_height = 30.0 + 10.0; // Filter controls
        let visible_area_y = self.y + top_controls_height + filter_controls_height;
        let visible_area_height = self.height - top_controls_height - filter_controls_height;
        
        // Generate todo item widgets with hierarchy
        self.setup_todo_item_widgets();
    }
    
    /// Filter todo items based on current filter settings
    fn filter_items(&self, items: &Vec<&TodoItem>) -> Vec<TodoItem> {
        items.iter()
            .filter(|item| {
                // Text filter
                let text_match = if !self.filter_value.is_empty() {
                    let search_text = self.filter_value.to_lowercase();
                    
                    match self.filter_type {
                        FilterType::Title => item.title().to_lowercase().contains(&search_text),
                        FilterType::Description => {
                            if let Some(desc) = item.description() {
                                desc.to_lowercase().contains(&search_text)
                            } else {
                                false
                            }
                        },
                        _ => true
                    }
                } else {
                    true
                };
                
                // Status filter
                let status_match = match self.status_filter {
                    Some(Status::Completed) => item.status() == Status::Completed,
                    Some(Status::InProgress) => item.status() == Status::InProgress,
                    Some(Status::NotStarted) => item.status() == Status::NotStarted,
                    None => true,
                };
                
                // Priority filter
                let priority_match = match self.priority_filter {
                    Some(Priority::High) => item.priority() == Priority::High,
                    Some(Priority::Medium) => item.priority() == Priority::Medium,
                    Some(Priority::Low) => item.priority() == Priority::Low,
                    None => true,
                };
                
                text_match && status_match && priority_match
            })
            .map(|&item| item.clone())
            .collect()
    }
    
    /// Set up callbacks for a TodoItem widget
    fn setup_todo_item_callbacks(&self, widget: Arc<Mutex<TodoItemWidget>>, item: TodoItem) {
        let todo_list_clone = self.todo_list.clone();
        let item_id = item.id();
        
        // --- Create status change callback --- 
        let status_callback = {
            let list_for_status = todo_list_clone.clone(); // Clone Arc for this closure
            let on_status_change = self.on_item_status_change.clone();
            let item_for_status = item.clone();
            Arc::new(move |status: Status| {
                if let Ok(mut todo_list) = list_for_status.lock() { // Use the cloned Arc
                    if let Some(item) = todo_list.get_item_mut(item_id) {
                        item.set_status(status);
                        
                        // Call external callback if provided
                        if let Some(callback) = &on_status_change {
                            callback(item.clone());
                        }
                    }
                }
            })
        };
        
        // --- Create edit callback --- 
        let edit_callback = {
            // No need to clone todo_list here
            let on_item_edit = self.on_item_edit.clone();
            let item_for_edit = item.clone();
            Arc::new(move || {
                if let Some(callback) = &on_item_edit {
                    callback(item_for_edit.clone());
                }
            })
        };
        
        // --- Create delete callback --- 
        let delete_callback = {
            let list_for_delete = todo_list_clone.clone(); // Clone Arc again for this closure
            let on_item_delete = self.on_item_delete.clone();
            let item_for_delete = item.clone(); 
            Arc::new(move || {
                if let Ok(mut todo_list) = list_for_delete.lock() { // Use the cloned Arc
                    todo_list.remove_item(item_id);
                    
                    // Call external callback if provided
                    if let Some(callback) = &on_item_delete {
                        callback(item_for_delete.clone());
                    }
                }
            })
        };
        
        // --- Set callbacks on the widget --- 
        if let Ok(mut widget_guard) = widget.lock() {
            // Clone the widget data to modify it, as `with_on_*` consumes self
            let mut temp_widget = (*widget_guard).clone();

            let status_cb = status_callback.clone();
            temp_widget = temp_widget.with_on_status_change(move |status| {
                status_cb(status);
            });
            
            let edit_cb = edit_callback.clone();
            temp_widget = temp_widget.with_on_edit(move || {
                edit_cb();
            });
            
            let delete_cb = delete_callback.clone();
            temp_widget = temp_widget.with_on_delete(move || {
                delete_cb();
            });
            
            // Assign the modified widget back to the MutexGuard
            *widget_guard = temp_widget;
        }
    }

    /// Set up todo item widgets based on the filtered and visible items
    fn setup_todo_item_widgets(&mut self) {
        // Get filtered items first, releasing the lock on todo_list immediately
        let filtered_items = {
            let todo_list_guard = match self.todo_list.lock() {
                Ok(guard) => guard,
                Err(_) => {
                    // Log error or handle appropriately
                    return; 
                }
            };
            self.filter_items(&todo_list_guard.all_items())
            // Lock is released here
        };

        // Preserve expansion state *before* clearing widgets
        let expanded_item_ids: Vec<Uuid> = self.expanded_items.iter()
            .filter_map(|&idx| {
                if idx < self.todo_item_widgets.len() {
                    if let Ok(widget) = self.todo_item_widgets[idx].lock() {
                        return Some(widget.todo_item.id());
                    }
                }
                None
            })
            .collect();

        // Clear existing widgets and state
        self.todo_item_widgets.clear();
        self.visible_items.clear();
        self.expanded_items.clear();
        
        // Calculate starting position for items
        let items_start_y = self.y + 50.0; // Below filter controls
        let item_height = 40.0; // Standard height for todo items
        let mut current_y = items_start_y - self.scroll_offset; // Apply initial scroll offset

        // Create widgets for each filtered item
        for (i, item) in filtered_items.into_iter().enumerate() {
            let todo_item_widget = TodoItemWidget::new(
                self.x, // Position relative to parent TodoListWidget X
                current_y, // Set the calculated Y position
                self.width, 
                item.clone()
            );
            
            let widget_arc = Arc::new(Mutex::new(todo_item_widget));
            
            // Set up callbacks (this function handles its own locking)
            self.setup_todo_item_callbacks(widget_arc.clone(), item.clone());
            
            self.todo_item_widgets.push(widget_arc);
            self.visible_items.push(i);
            
            // Restore expansion state using the preserved IDs
            if expanded_item_ids.contains(&item.id()) {
                self.expanded_items.push(i);
            }
            
            // Update Y for the next item
            current_y += item_height; 
        }
        
        // Calculate max scroll after all modifications to self are done
        self.calculate_max_scroll();
    }
    
    /// Render the filter controls
    fn render_filter_controls(&self, ctx: &mut RenderContext) {
        // Filter controls at the top
        let filter_y = self.y + 10.0;
        
        // Draw search box
        ctx.draw_rect(
            self.x + 10.0, filter_y,
            150.0, 30.0,
            self.theme.get_background_color(),
        );
        
        // Text input placeholder or value
        let search_text = if self.filter_value.is_empty() { "Search..." } else { &self.filter_value };
        ctx.draw_text(
            search_text,
            self.x + 15.0, filter_y + 5.0,
            self.theme.small_text_size(),
            self.theme.get_text_color(),
        );
        
        // Draw filter type dropdown
        let filter_type_x = self.x + 170.0;
        ctx.draw_rect(
            filter_type_x, filter_y,
            120.0, 30.0,
            self.theme.get_background_color(),
        );
        
        // Filter type text
        let filter_type_text = match self.filter_type {
            FilterType::Title => "Title",
            FilterType::Description => "Description",
            _ => "All Fields",
        };
        
        ctx.draw_text(
            filter_type_text,
            filter_type_x + 10.0, filter_y + 5.0,
            self.theme.small_text_size(),
            self.theme.get_text_color(),
        );
        
        // Status filter
        let status_x = self.x + 300.0;
        ctx.draw_rect(
            status_x, filter_y,
            120.0, 30.0,
            self.theme.get_background_color(),
        );
        
        // Status text
        let status_text = match self.status_filter {
            Some(Status::NotStarted) => "Not Started",
            Some(Status::InProgress) => "In Progress",
            Some(Status::Completed) => "Completed",
            None => "All Status",
        };
        
        ctx.draw_text(
            status_text,
            status_x + 10.0, filter_y + 5.0,
            self.theme.small_text_size(),
            self.theme.get_text_color(),
        );
        
        // Priority filter
        let priority_x = self.x + 430.0;
        ctx.draw_rect(
            priority_x, filter_y,
            120.0, 30.0,
            self.theme.get_background_color(),
        );
        
        // Priority text
        let priority_text = match self.priority_filter {
            Some(Priority::Low) => "Low",
            Some(Priority::Medium) => "Medium",
            Some(Priority::High) => "High",
            None => "All Priority",
        };
        
        ctx.draw_text(
            priority_text,
            priority_x + 10.0, filter_y + 5.0,
            self.theme.small_text_size(),
            self.theme.get_text_color(),
        );
    }
    
    /// Handle mouse wheel for scrolling
    pub fn handle_mouse_wheel(&mut self, delta: f32) {
        // Update scroll offset with the mouse wheel delta
        self.scroll_offset = (self.scroll_offset + delta * 20.0)
            .max(0.0)
            .min(self.max_scroll);
        
        // Update positions of todo item widgets based on new scroll offset
        let top_controls_height = 50.0; // Height of the filter controls area
        let visible_area_y = self.y + top_controls_height;
        
        // Reposition all visible todo item widgets based on scroll offset
        let mut y_position = visible_area_y - self.scroll_offset;
        let item_height = 40.0; // Standard height for todo items
        
        for &item_idx in &self.visible_items {
            if item_idx < self.todo_item_widgets.len() {
                if let Ok(mut widget) = self.todo_item_widgets[item_idx].lock() {
                    widget.set_position(self.x, y_position);
                    y_position += item_height;
                }
            }
        }
    }
    
    /// Set a callback for when an item's status changes
    pub fn with_on_status_change<F>(mut self, callback: F) -> Self
    where
        F: Fn(TodoItem) + Send + Sync + 'static,
    {
        self.on_item_status_change = Some(Arc::new(callback));
        self
    }
    
    /// Set a callback for when an item is edited
    pub fn with_on_edit<F>(mut self, callback: F) -> Self
    where
        F: Fn(TodoItem) + Send + Sync + 'static,
    {
        self.on_item_edit = Some(Arc::new(callback));
        self
    }
    
    /// Set a callback for when an item is deleted
    pub fn with_on_delete<F>(mut self, callback: F) -> Self
    where
        F: Fn(TodoItem) + Send + Sync + 'static,
    {
        self.on_item_delete = Some(Arc::new(callback));
        self
    }
    
    /// Handle mouse movement for hover effects
    pub fn handle_mouse_move(&mut self, x: f32, y: f32) {
        // Handle mouse movement in filter buttons
        for button in &mut self.filter_buttons {
            if button.contains_point(x, y) {
                button.handle_mouse_move(x, y);
            }
        }
        
        // Handle mouse movement in add button
        if self.add_button.contains_point(x, y) {
            self.add_button.handle_mouse_move(x, y);
        }
        
        // No handle_mouse_move method in TextInput, so we'll skip these
        // Handle mouse movement in title input and search input
    }
    
    /// Handle mouse button up
    pub fn handle_mouse_up(&mut self, x: f32, y: f32) {
        // Handle mouse up in filter buttons
        for button in &mut self.filter_buttons {
            button.handle_mouse_up(x, y);
        }
        
        // Handle mouse up in add button
        self.add_button.handle_mouse_up(x, y);
        
        // Handle mouse up in title input
        if self.title_input.contains_point(x, y) {
            self.title_input.handle_mouse_down(x, y);
            self.title_input.set_focused(true);
            self.search_input.set_focused(false);
        }
        
        // Handle mouse up in search input
        if self.search_input.contains_point(x, y) {
            self.search_input.handle_mouse_down(x, y);
            self.search_input.set_focused(true);
            self.title_input.set_focused(false);
        }
        
        // Handle mouse up in todo item widgets
        for widget in &mut self.todo_item_widgets {
            if let Ok(mut widget) = widget.lock() {
                widget.handle_mouse_up(x, y);
            }
        }
    }
    
    /// Handle character input for text fields
    pub fn handle_char_input(&mut self, c: char) {
        // Update title input if it has focus
        if self.title_input.is_focused() {
            self.title_input.handle_char_input(c);
        }
        
        // Update search input if it has focus
        if self.search_input.is_focused() {
            self.search_input.handle_char_input(c);
            
            // Update the search text and regenerate widgets
            self.search_text = self.search_input.text().to_string();
            if self.search_text == "Search..." {
                self.search_text = String::new();
            }
            
            self.update_todo_items();
        }
    }
    
    /// Handle keyboard input
    pub fn handle_key_press(&mut self, key_code: winit::keyboard::KeyCode) {
        // Handle keyboard input in title input
        if self.title_input.is_focused() {
            match key_code {
                winit::keyboard::KeyCode::Escape => {
                    // Clear focus
                    self.title_input.set_focused(false);
                },
                winit::keyboard::KeyCode::Enter => {
                    // Add a new task if Enter is pressed
                    let title = self.title_input.text().trim();
                    if !title.is_empty() && title != "New task..." {
                        if let Ok(mut todo_list) = self.todo_list.lock() {
                            todo_list.create_item(title);
                        }
                        
                        // Clear the input field
                        self.title_input.set_text("New task...");
                        
                        // Regenerate todo item widgets
                        self.update_todo_items();
                    }
                    
                    // Clear focus
                    self.title_input.set_focused(false);
                },
                _ => {
                    // Let the text input handle other keys
                    self.title_input.handle_key_press(key_code);
                }
            }
        }
        
        // Handle keyboard input in search input
        if self.search_input.is_focused() {
            match key_code {
                winit::keyboard::KeyCode::Escape => {
                    // Clear focus and search
                    self.search_input.set_focused(false);
                    self.search_input.set_text("Search...");
                    self.search_text = String::new();
                    
                    // Regenerate todo item widgets with no search filter
                    self.update_todo_items();
                },
                _ => {
                    // Let the text input handle other keys
                    self.search_input.handle_key_press(key_code);
                    
                    // Update search text (except for special keys)
                    match key_code {
                        winit::keyboard::KeyCode::Backspace
                        | winit::keyboard::KeyCode::Delete => {
                            // Update search text after handling key press
                            self.search_text = self.search_input.text().to_string();
                            if self.search_text == "Search..." {
                                self.search_text = String::new();
                            }
                            
                            self.update_todo_items();
                        },
                        _ => {}
                    }
                }
            }
        }
    }

    /// Handle mouse down event - use one implementation with context dimensions
    pub fn handle_mouse_down(&mut self, x: f32, y: f32, ctx_width: f32, ctx_height: f32) -> bool {
        // Check if we clicked on any expanded modals first
        for (i, widget) in self.todo_item_widgets.iter().enumerate() {
            if let Ok(widget_mut) = widget.lock() { // Changed to immutable lock as we only read state
                // Check if click is in a modal
                if self.expanded_items.contains(&i) && 
                   widget_mut.modal_contains_point(x, y, ctx_width, ctx_height) {
                    // If click is inside an expanded modal, consume the event but don't change state here
                    return true; 
                }
            }
        }
        
        // If not in a modal, check regular widgets
        for (i, widget) in self.todo_item_widgets.iter().enumerate() {
            if let Ok(mut widget_mut) = widget.lock() {
                if widget_mut.contains_point(x, y) {
                    widget_mut.handle_mouse_down(x, y); // Call handle_mouse_down, ignore return value
                    let is_expanded_now = widget_mut.is_expanded(); // Use getter
                    
                    // Check if the item was expanded *after* handling the click
                    if is_expanded_now {
                        if !self.expanded_items.contains(&i) {
                            self.expanded_items.push(i);
                        }
                    } else {
                        self.expanded_items.retain(|&idx| idx != i);
                    }
                    return true; // Indicate the event was handled by this widget
                }
            }
        }
        
        // Check filter controls
        self.handle_filter_controls_click(x, y)
    }
    
    /// Render base widgets (first pass rendering)
    pub fn render_base(&self, ctx: &mut RenderContext) {
        // Draw background
        ctx.draw_rect(
            self.x, self.y,
            self.width, self.height,
            self.theme.get_background_color(),
        );
        
        // Render filter controls at top
        self.render_filter_controls(ctx);
        
        // Calculate areas for todo items
        let items_y = self.y + 50.0; // Below filter controls
        let items_height = self.height - 50.0;
        
        // Create clipping rectangle for todo items area
        ctx.push_clip_rect(self.x, items_y, self.width, items_height);
        
        // Render visible todo items
        for &widget_idx in &self.visible_items {
            if widget_idx < self.todo_item_widgets.len() {
                let widget = &self.todo_item_widgets[widget_idx];
                if let Ok(widget) = widget.lock() {
                    widget.render_base(ctx);
                }
            }
        }
        
        // Render scrollbar if needed
        if self.max_scroll > 0.0 {
            let scrollbar_width = 8.0;
            let scrollbar_x = self.x + self.width - scrollbar_width - 5.0;
            let scrollbar_y = items_y;
            let scrollbar_height = items_height;
            
            // Draw scrollbar background
            ctx.draw_rect(
                scrollbar_x, scrollbar_y,
                scrollbar_width, scrollbar_height,
                self.theme.get_scrollbar_bg_color(),
            );
            
            // Calculate handle position and size
            let visible_ratio = items_height / (items_height + self.max_scroll);
            let handle_height = items_height * visible_ratio;
            let handle_y = scrollbar_y + (self.scroll_offset / self.max_scroll) * (items_height - handle_height);
            
            // Draw scrollbar handle
            ctx.draw_rect(
                scrollbar_x, handle_y,
                scrollbar_width, handle_height,
                self.theme.get_scrollbar_handle_color(),
            );
        }
        
        // Remove clipping rectangle
        ctx.pop_clip_rect();
    }
    
    /// Render modals (second pass rendering)
    pub fn render_modals(&self, ctx: &mut RenderContext) {
        // Render expanded item modals (second pass)
        for &widget_idx in &self.expanded_items {
            if widget_idx < self.todo_item_widgets.len() {
                let widget = &self.todo_item_widgets[widget_idx];
                // Lock the widget before calling render_modal
                if let Ok(widget) = widget.lock() {
                    widget.render_modal(ctx);
                }
            }
        }
    }

    /// Render the widget
    pub fn render(&self, ctx: &mut RenderContext) {
        self.render_base(ctx);
        self.render_modals(ctx);
    }

    /// Get the theme's hierarchy indent value (since it's missing from the theme)
    fn get_hierarchy_indent(&self) -> f32 {
        15.0 // Default indent value for hierarchy levels
    }

    /// Calculate the maximum scroll value based on the number of items
    fn calculate_max_scroll(&mut self) {
        let items_height = self.visible_items.len() as f32 * 40.0; // 40.0 is the standard item height
        let visible_area_height = self.height - 50.0; // Subtract height of filter controls
        
        self.max_scroll = (items_height - visible_area_height).max(0.0);
        self.scroll_offset = self.scroll_offset.min(self.max_scroll);
    }

    /// Handle clicks on filter controls
    fn handle_filter_controls_click(&mut self, x: f32, y: f32) -> bool {
        // Status dropdown
        let status_dropdown_width = 120.0;
        let status_dropdown_x = self.x + 300.0;  // Match values from render_filter_controls
        let status_dropdown_y = self.y + 10.0;   // Match values from render_filter_controls
        
        if x >= status_dropdown_x && x <= status_dropdown_x + status_dropdown_width &&
           y >= status_dropdown_y && y <= status_dropdown_y + 30.0 {
            // Cycle through status options
            self.status_filter = match self.status_filter {
                None => Some(Status::NotStarted),
                Some(Status::NotStarted) => Some(Status::InProgress),
                Some(Status::InProgress) => Some(Status::Completed),
                Some(Status::Completed) => None,
            };
            
            // Update todo item widgets
            self.setup_todo_item_widgets();
            return true;
        }
        
        // Filter type dropdown
        let filter_dropdown_width = 120.0;
        let filter_dropdown_x = self.x + 170.0;  // Match values from render_filter_controls
        let filter_dropdown_y = status_dropdown_y;
        
        if x >= filter_dropdown_x && x <= filter_dropdown_x + filter_dropdown_width &&
           y >= filter_dropdown_y && y <= filter_dropdown_y + 30.0 {
            // Cycle through filter type options
            self.filter_type = match self.filter_type {
                FilterType::None => FilterType::Title,
                FilterType::Title => FilterType::Description,
                FilterType::Description => FilterType::None,
                _ => FilterType::None,
            };
            
            // Update todo item widgets
            self.setup_todo_item_widgets();
            return true;
        }
        
        // Priority dropdown
        let priority_dropdown_width = 120.0;
        let priority_dropdown_x = self.x + 430.0;  // Match values from render_filter_controls
        let priority_dropdown_y = status_dropdown_y;
        
        if x >= priority_dropdown_x && x <= priority_dropdown_x + priority_dropdown_width &&
           y >= priority_dropdown_y && y <= priority_dropdown_y + 30.0 {
            // Cycle through priority options
            self.priority_filter = match self.priority_filter {
                None => Some(Priority::Low),
                Some(Priority::Low) => Some(Priority::Medium),
                Some(Priority::Medium) => Some(Priority::High),
                Some(Priority::High) => None,
            };
            
            // Update todo item widgets
            self.setup_todo_item_widgets();
            return true;
        }
        
        // Search box
        let search_box_width = 150.0;
        let search_box_x = self.x + 10.0;  // Match values from render_filter_controls
        let search_box_y = status_dropdown_y;
        
        if x >= search_box_x && x <= search_box_x + search_box_width &&
           y >= search_box_y && y <= search_box_y + 30.0 {
            // Toggle search input active state (in a real app, this would open a text input)
            // Here we'll just clear the search text to demonstrate
            if !self.filter_value.is_empty() {
                self.filter_value = String::new();
                self.setup_todo_item_widgets();
            }
            return true;
        }
        
        false
    }
}

impl Widget for TodoListWidget {
    fn update(&mut self, delta_time: f32) {
        // Update child components
        self.panel.update(delta_time);
        self.add_button.update(delta_time);
        self.title_input.update(delta_time);
        self.search_input.update(delta_time);
        
        for button in &mut self.filter_buttons {
            button.update(delta_time);
        }
        
        for widget in &mut self.todo_item_widgets {
            if let Ok(mut widget) = widget.lock() {
                widget.update(delta_time);
            }
        }
    }
    
    fn render(&self, ctx: &mut RenderContext) {
        self.render_base(ctx);
        self.render_modals(ctx);
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
        
        // Update positions of child components
        let (panel_x, panel_y) = self.panel.position();
        self.panel.set_position(panel_x + dx, panel_y + dy);
        
        let (add_x, add_y) = self.add_button.position();
        self.add_button.set_position(add_x + dx, add_y + dy);
        
        let (input_x, input_y) = self.title_input.position();
        self.title_input.set_position(input_x + dx, input_y + dy);
        
        for button in &mut self.filter_buttons {
            let (btn_x, btn_y) = button.position();
            button.set_position(btn_x + dx, btn_y + dy);
        }
        
        let (search_x, search_y) = self.search_input.position();
        self.search_input.set_position(search_x + dx, search_y + dy);
        
        // Update positions of todo item widgets
        for widget in &mut self.todo_item_widgets {
            if let Ok(mut widget) = widget.lock() {
                let (widget_x, widget_y) = widget.position();
                widget.set_position(widget_x + dx, widget_y + dy);
            }
        }
    }
    
    fn set_dimensions(&mut self, width: f32, height: f32) {
        self.width = width;
        self.height = height;
        
        // Update panel dimensions
        self.panel.set_dimensions(width, height);
        
        // Update positions and dimensions of child components
        let button_width = 80.0;
        let button_height = 30.0;
        let button_padding = 10.0;
        
        self.add_button.set_position(
            self.x + width - button_width - button_padding,
            self.y + button_padding
        );
        
        let input_width = width - button_width - button_padding * 3.0;
        self.title_input.set_position(
            self.x + button_padding,
            self.y + button_padding
        );
        self.title_input.set_dimensions(input_width, button_height);
        
        // Reposition filter buttons
        let new_filter_buttons = Self::create_filter_buttons(self.x, self.y, width, &self.theme);
        self.filter_buttons = new_filter_buttons;
        
        // Reposition search input
        let search_input_width = 200.0;
        self.search_input.set_position(
            self.x + width - search_input_width - button_padding,
            self.y + button_padding * 2.0 + button_height
        );
        
        // Regenerate todo item widgets
        self.update_todo_items();
    }
    
    fn contains_point(&self, x: f32, y: f32) -> bool {
        x >= self.x && x <= self.x + self.width && y >= self.y && y <= self.y + self.height
    }
}

impl Clone for TodoListWidget {
    fn clone(&self) -> Self {
        // Create a new instance with the same properties
        let mut clone = Self {
            x: self.x,
            y: self.y,
            width: self.width,
            height: self.height,
            todo_list: self.todo_list.clone(),
            panel: self.panel.clone(),
            add_button: self.add_button.clone(),
            title_input: self.title_input.clone(),
            filter_buttons: self.filter_buttons.clone(),
            search_input: self.search_input.clone(),
            scroll_offset: self.scroll_offset,
            max_scroll: self.max_scroll,
            todo_item_widgets: Vec::new(), // Will be regenerated
            show_completed: self.show_completed,
            filter_priority: self.filter_priority,
            filter_status: self.filter_status,
            search_text: self.search_text.clone(),
            on_item_status_change: None, // Will be manually cloned
            on_item_edit: None, // Will be manually cloned
            on_item_delete: None, // Will be manually cloned
            theme: CyberpunkTheme::new(), // Theme is stateless, just create a new one
            modal_open_index: None, // Will be manually cloned
            expanded_items: self.expanded_items.clone(), // Will be manually cloned
            visible_items: self.visible_items.clone(),
            filter_value: self.filter_value.clone(),
            filter_type: self.filter_type,
            status_filter: self.status_filter,
            priority_filter: self.priority_filter,
        };
        
        // Manually clone callback Arc pointers
        if let Some(cb) = &self.on_item_status_change {
            clone.on_item_status_change = Some(cb.clone());
        }
        
        if let Some(cb) = &self.on_item_edit {
            clone.on_item_edit = Some(cb.clone());
        }
        
        if let Some(cb) = &self.on_item_delete {
            clone.on_item_delete = Some(cb.clone());
        }
        
        // Regenerate todo item widgets
        clone.update_todo_items();
        
        clone
    }
}