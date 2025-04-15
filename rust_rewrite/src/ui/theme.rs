/// CyberpunkTheme encapsulates the visual styling for the UI
#[derive(Debug, Clone)]
pub struct CyberpunkTheme {
    // Various color constants and styling options
}

impl CyberpunkTheme {
    /// Create a new theme with default values
    pub fn new() -> Self {
        Self {}
    }
    
    /// Get neon pink as [r, g, b, a]
    pub fn neon_pink(&self) -> [f32; 4] {
        [1.0, 0.255, 0.639, 1.0] // #FF41A3
    }
    
    /// Get cyan as [r, g, b, a]
    pub fn cyan(&self) -> [f32; 4] {
        [0.0, 1.0, 0.95, 1.0] // #00FFF3
    }
    
    /// Get purple as [r, g, b, a]
    pub fn purple(&self) -> [f32; 4] {
        [0.67, 0.36, 1.0, 1.0] // #AD5CFF
    }
    
    /// Get dimmed purple as [r, g, b, a]
    pub fn dimmed_purple(&self) -> [f32; 4] {
        [0.67, 0.36, 1.0, 0.7] // #AD5CFF with 70% opacity
    }
    
    /// Get bright text color as [r, g, b, a]
    pub fn bright_text(&self) -> [f32; 4] {
        [0.95, 0.95, 1.0, 1.0] // #F2F2FF
    }
    
    /// Get dark background as [r, g, b, a]
    pub fn background(&self) -> [f32; 4] {
        [0.039, 0.039, 0.078, 1.0] // #0A0A14
    }
    
    /// Get muted text color as [r, g, b, a]
    pub fn muted_text(&self) -> [f32; 4] {
        [0.65, 0.65, 0.75, 1.0] // #A6A6BF - slightly brighter gray with blue tint
    }
    
    /// Get panel background with translucency as [r, g, b, a]
    pub fn panel_background(&self) -> [f32; 4] {
        [0.12, 0.12, 0.22, 0.85] // Translucent dark blue with better opacity
    }
    
    /// Get border color as [r, g, b, a]
    pub fn border(&self) -> [f32; 4] {
        [0.0, 0.9, 0.9, 1.0] // Brighter cyan border
    }
    
    /// Get highlight color as [r, g, b, a]
    pub fn highlight(&self) -> [f32; 4] {
        [1.0, 0.8, 0.2, 1.0] // Gold-ish highlight
    }
    
    /// Get danger/error color as [r, g, b, a]
    pub fn danger(&self) -> [f32; 4] {
        [1.0, 0.3, 0.3, 1.0] // Red-ish danger
    }
    
    /// Get success color as [r, g, b, a]
    pub fn success(&self) -> [f32; 4] {
        [0.3, 1.0, 0.5, 1.0] // Green-ish success
    }
    
    /// Get default text size
    pub fn text_size(&self) -> f32 {
        18.0 // Increased from 16.0
    }
    
    /// Get header text size
    pub fn header_text_size(&self) -> f32 {
        28.0 // Increased from 24.0
    }
    
    /// Get small text size
    pub fn small_text_size(&self) -> f32 {
        14.0 // Added new size for smaller text
    }
    
    /// Get button padding [x, y]
    pub fn button_padding(&self) -> [f32; 2] {
        [12.0, 8.0] // Increased from [10.0, 5.0]
    }
    
    /// Get panel padding [x, y]
    pub fn panel_padding(&self) -> [f32; 2] {
        [18.0, 18.0] // Increased from [15.0, 15.0]
    }
    
    /// Get default border width
    pub fn border_width(&self) -> f32 {
        2.0
    }
    
    /// Get default corner radius
    pub fn corner_radius(&self) -> f32 {
        6.0 // Increased from 4.0
    }
    
    /// Get glow intensity
    pub fn glow_intensity(&self) -> f32 {
        0.8 // Increased from 0.7
    }
    
    /// Get filter button background
    pub fn filter_button_bg(&self) -> [f32; 4] {
        [0.15, 0.15, 0.25, 1.0] // Dark blue-purple
    }
    
    /// Get filter button selected background
    pub fn filter_button_selected_bg(&self) -> [f32; 4] {
        [0.2, 0.2, 0.35, 1.0] // Brighter blue-purple
    }
    
    /// Get todo item height
    pub fn todo_item_height(&self) -> f32 {
        48.0 // Increased from 40.0
    }
    
    // Modal colors
    
    /// Get modal background
    pub fn modal_background(&self) -> [f32; 4] {
        [0.08, 0.08, 0.15, 0.95] // Dark translucent background
    }
    
    /// Get modal border glow
    pub fn modal_border_glow(&self) -> [f32; 4] {
        [0.0, 0.9, 0.9, 0.7] // Cyan glow
    }
    
    /// Get modal title color
    pub fn modal_title(&self) -> [f32; 4] {
        [0.0, 0.9, 0.9, 1.0] // Bright cyan for title
    }
    
    /// Get modal text color
    pub fn modal_text(&self) -> [f32; 4] {
        [0.85, 0.85, 0.95, 1.0] // Light blue-tinted text
    }
    
    /// Get modal label color (for field labels)
    pub fn modal_label(&self) -> [f32; 4] {
        [0.65, 0.65, 0.85, 1.0] // Bluer grey for labels
    }
    
    /// Get modal close button color
    pub fn modal_close_button(&self) -> [f32; 4] {
        [0.2, 0.2, 0.3, 0.8] // Dark button
    }
    
    /// Get modal close button hover color
    pub fn modal_close_button_hover(&self) -> [f32; 4] {
        [0.3, 0.3, 0.4, 0.9] // Lighter when hovered
    }
    
    /// Get modal close button icon color
    pub fn modal_close_icon(&self) -> [f32; 4] {
        [0.8, 0.8, 0.9, 0.9] // Light grey
    }
    
    /// Get modal close button icon hover color
    pub fn modal_close_icon_hover(&self) -> [f32; 4] {
        [1.0, 0.3, 0.3, 1.0] // Red when hovered
    }
    
    /// Get modal overlay color (for darkening the background)
    pub fn modal_overlay(&self) -> [f32; 4] {
        [0.0, 0.0, 0.0, 0.4] // Translucent black
    }
    
    /// Get modal shadow color
    pub fn modal_shadow(&self) -> [f32; 4] {
        [0.0, 0.0, 0.0, 0.5] // Semi-transparent shadow
    }
    
    /// Get modal warning color (for overdue tasks, etc.)
    pub fn modal_warning(&self) -> [f32; 4] {
        [1.0, 0.5, 0.2, 1.0] // Orange-ish warning
    }
    
    // Priority colors
    
    /// Priority colors for Critical priority
    pub fn priority_critical(&self) -> [f32; 4] {
        [1.0, 0.0, 0.0, 1.0] // Pure red
    }
    
    /// Priority colors for High priority
    pub fn priority_high(&self) -> [f32; 4] {
        [1.0, 0.3, 0.3, 1.0] // Red
    }
    
    /// Priority colors for Medium priority
    pub fn priority_medium(&self) -> [f32; 4] {
        [1.0, 0.8, 0.2, 1.0] // Yellow/gold
    }
    
    /// Priority colors for Low priority
    pub fn priority_low(&self) -> [f32; 4] {
        [0.3, 0.8, 0.3, 1.0] // Green
    }
    
    /// Priority colors for None priority
    pub fn priority_none(&self) -> [f32; 4] {
        [0.4, 0.4, 0.4, 0.7] // Grey
    }
    
    // Todo item specific colors
    
    /// Checkbox border color
    pub fn checkbox_border(&self) -> [f32; 4] {
        [0.5, 0.5, 0.7, 0.9] // Blueish grey
    }
    
    /// Checkbox empty background
    pub fn checkbox_empty(&self) -> [f32; 4] {
        [0.15, 0.15, 0.2, 0.5] // Dark translucent
    }
    
    /// Checkbox filled background
    pub fn checkbox_filled(&self) -> [f32; 4] {
        [0.1, 0.5, 0.1, 0.7] // Green translucent
    }
    
    /// Checkbox checkmark color
    pub fn checkbox_check(&self) -> [f32; 4] {
        [0.0, 1.0, 0.5, 1.0] // Bright green
    }
    
    /// Edit button color
    pub fn edit_button(&self) -> [f32; 4] {
        [0.2, 0.3, 0.4, 0.7] // Blue-ish
    }
    
    /// Edit button icon color
    pub fn edit_button_icon(&self) -> [f32; 4] {
        [0.5, 0.8, 1.0, 1.0] // Light blue
    }
    
    /// Delete button color
    pub fn delete_button(&self) -> [f32; 4] {
        [0.3, 0.1, 0.1, 0.7] // Dark red
    }
    
    /// Delete button icon color
    pub fn delete_button_icon(&self) -> [f32; 4] {
        [1.0, 0.5, 0.5, 1.0] // Light red
    }
    
    /// Task item background
    pub fn item_bg(&self) -> [f32; 4] {
        [0.1, 0.1, 0.2, 0.3] // Very dark translucent
    }
    
    /// Task item hover background
    pub fn item_hover_bg(&self) -> [f32; 4] {
        [0.15, 0.15, 0.25, 0.5] // Slightly brighter when hovered
    }
    
    /// Task title text color when normal
    pub fn text_normal(&self) -> [f32; 4] {
        [0.9, 0.9, 1.0, 1.0] // Nearly white
    }
    
    /// Task title text color when completed
    pub fn text_completed(&self) -> [f32; 4] {
        [0.5, 0.5, 0.6, 0.8] // Dimmed grey
    }

    // Compatibility methods with 'get_' prefix

    /// Get background color
    pub fn get_background_color(&self) -> [f32; 4] {
        self.background()
    }

    /// Get card background color
    pub fn get_card_background_color(&self) -> [f32; 4] {
        self.item_bg()
    }

    /// Get high priority color
    pub fn get_high_priority_color(&self) -> [f32; 4] {
        self.priority_high()
    }

    /// Get medium priority color
    pub fn get_medium_priority_color(&self) -> [f32; 4] {
        self.priority_medium()
    }

    /// Get low priority color
    pub fn get_low_priority_color(&self) -> [f32; 4] {
        self.priority_low()
    }

    /// Get normal priority color
    pub fn get_normal_priority_color(&self) -> [f32; 4] {
        self.priority_none()
    }

    /// Get hierarchy indent color
    pub fn get_hierarchy_indent_color(&self) -> [f32; 4] {
        [0.15, 0.15, 0.3, 0.5] // Subtle color for indentation
    }

    /// Get checkbox checked color
    pub fn get_checkbox_checked_color(&self) -> [f32; 4] {
        self.checkbox_filled()
    }

    /// Get checkbox unchecked color
    pub fn get_checkbox_unchecked_color(&self) -> [f32; 4] {
        self.checkbox_empty()
    }

    /// Get text color
    pub fn get_text_color(&self) -> [f32; 4] {
        self.text_normal()
    }

    /// Get completed text color
    pub fn get_completed_text_color(&self) -> [f32; 4] {
        self.text_completed()
    }

    /// Get delete button color
    pub fn get_delete_button_color(&self) -> [f32; 4] {
        self.delete_button_icon()
    }

    /// Get edit button color
    pub fn get_edit_button_color(&self) -> [f32; 4] {
        self.edit_button_icon()
    }

    /// Get expand button color
    pub fn get_expand_button_color(&self) -> [f32; 4] {
        [0.6, 0.6, 0.9, 1.0] // Light bluish color
    }

    /// Get overdue color
    pub fn get_overdue_color(&self) -> [f32; 4] {
        self.danger()
    }

    /// Get due date color
    pub fn get_due_date_color(&self) -> [f32; 4] {
        self.muted_text()
    }

    /// Get modal overlay color
    pub fn get_modal_overlay_color(&self) -> [f32; 4] {
        self.modal_overlay()
    }

    /// Get modal background color
    pub fn get_modal_bg_color(&self) -> [f32; 4] {
        self.modal_background()
    }

    /// Get modal header color
    pub fn get_modal_header_color(&self) -> [f32; 4] {
        [0.12, 0.12, 0.25, 1.0] // Slightly darker than the modal background
    }

    /// Get modal text color
    pub fn get_modal_text_color(&self) -> [f32; 4] {
        self.modal_text()
    }

    /// Get modal close button color
    pub fn get_modal_close_button_color(&self) -> [f32; 4] {
        self.modal_close_icon()
    }

    /// Get placeholder color
    pub fn get_placeholder_color(&self) -> [f32; 4] {
        [0.4, 0.4, 0.5, 0.6] // Dimmed text for placeholders
    }

    /// Get scrollbar background color
    pub fn get_scrollbar_bg_color(&self) -> [f32; 4] {
        [0.15, 0.15, 0.25, 0.3] // Semi-transparent dark color
    }

    /// Get scrollbar handle color
    pub fn get_scrollbar_handle_color(&self) -> [f32; 4] {
        [0.3, 0.3, 0.5, 0.7] // Semi-transparent lighter color
    }
}

impl Default for CyberpunkTheme {
    fn default() -> Self {
        Self::new()
    }
} 