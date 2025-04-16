use std::time::{SystemTime, UNIX_EPOCH};
use std::fmt;
use uuid::Uuid;

/// Priority levels for todo items
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, serde::Serialize, serde::Deserialize)]
pub enum Priority {
    Low,
    Medium,
    High,
}

impl Default for Priority {
    fn default() -> Self {
        Priority::Medium
    }
}

impl fmt::Display for Priority {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Priority::Low => write!(f, "Low"),
            Priority::Medium => write!(f, "Medium"),
            Priority::High => write!(f, "High"),
        }
    }
}

/// Status of a todo item
#[derive(Debug, Clone, Copy, PartialEq, Eq, serde::Serialize, serde::Deserialize)]
pub enum Status {
    NotStarted,
    InProgress,
    Completed,
}

impl Default for Status {
    fn default() -> Self {
        Status::NotStarted
    }
}

impl fmt::Display for Status {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Status::NotStarted => write!(f, "Not Started"),
            Status::InProgress => write!(f, "In Progress"),
            Status::Completed => write!(f, "Completed"),
        }
    }
}

/// A TodoItem represents a single task in the todo list
#[derive(Debug, Clone, PartialEq, serde::Serialize, serde::Deserialize)]
pub struct TodoItem {
    /// Unique identifier for the item
    id: Uuid,
    
    /// Title of the todo item
    title: String,
    
    /// Optional detailed description
    description: Option<String>,
    
    /// Current status of the item
    status: Status,
    
    /// Priority level of the item
    priority: Priority,
    
    /// Unix timestamp of when the item was created
    created_at: u64,
    
    /// Unix timestamp of when the item is due, if any
    due_date: Option<u64>,
    
    /// Parent item ID for hierarchical structure
    parent_id: Option<Uuid>,
    
    /// Additional metadata as key-value pairs
    #[serde(default)]
    metadata: std::collections::HashMap<String, String>,
}

impl TodoItem {
    /// Create a new TodoItem with the given title
    pub fn new(title: &str) -> Self {
        let now = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .expect("Time went backwards")
            .as_secs();
            
        TodoItem {
            id: Uuid::new_v4(),
            title: title.to_string(),
            description: None,
            status: Status::default(),
            priority: Priority::default(),
            created_at: now,
            due_date: None,
            parent_id: None,
            metadata: std::collections::HashMap::new(),
        }
    }
    
    // --- Getters ---
    
    /// Get the item's unique ID
    pub fn id(&self) -> Uuid {
        self.id
    }
    
    /// Get the item's title
    pub fn title(&self) -> &str {
        &self.title
    }
    
    /// Get the item's description, if any
    pub fn description(&self) -> Option<&str> {
        self.description.as_deref()
    }
    
    /// Get the item's status
    pub fn status(&self) -> Status {
        self.status
    }
    
    /// Get the item's priority
    pub fn priority(&self) -> Priority {
        self.priority
    }
    
    /// Get the item's creation timestamp
    pub fn created_at(&self) -> u64 {
        self.created_at
    }
    
    /// Get the item's due date, if any
    pub fn due_date(&self) -> Option<u64> {
        self.due_date
    }
    
    /// Get the item's parent ID, if any
    pub fn parent_id(&self) -> Option<Uuid> {
        self.parent_id
    }
    
    /// Get a reference to the item's metadata
    pub fn metadata(&self) -> &std::collections::HashMap<String, String> {
        &self.metadata
    }
    
    // --- Setters ---
    
    /// Set the item's title
    pub fn set_title(&mut self, title: &str) {
        self.title = title.to_string();
    }
    
    /// Set the item's description
    pub fn set_description(&mut self, description: Option<&str>) {
        self.description = description.map(|s| s.to_string());
    }
    
    /// Set the item's status
    pub fn set_status(&mut self, status: Status) {
        self.status = status;
    }
    
    /// Set the item's priority
    pub fn set_priority(&mut self, priority: Priority) {
        self.priority = priority;
    }
    
    /// Set the item's due date
    pub fn set_due_date(&mut self, due_date: Option<u64>) {
        self.due_date = due_date;
    }
    
    /// Set the item's parent ID
    pub fn set_parent_id(&mut self, parent_id: Option<Uuid>) {
        self.parent_id = parent_id;
    }
    
    /// Add or update a metadata value
    pub fn set_metadata(&mut self, key: &str, value: &str) {
        self.metadata.insert(key.to_string(), value.to_string());
    }
    
    /// Remove a metadata value
    pub fn remove_metadata(&mut self, key: &str) -> Option<String> {
        self.metadata.remove(key)
    }
    
    // --- Convenience methods ---
    
    /// Check if the item is completed
    pub fn is_completed(&self) -> bool {
        self.status == Status::Completed
    }
    
    /// Mark the item as completed
    pub fn mark_completed(&mut self) {
        self.status = Status::Completed;
    }
    
    /// Check if the item is overdue
    pub fn is_overdue(&self) -> bool {
        if let Some(due) = self.due_date {
            let now = SystemTime::now()
                .duration_since(UNIX_EPOCH)
                .expect("Time went backwards")
                .as_secs();
                
            return due < now && !self.is_completed();
        }
        false
    }
    
    // --- Builder methods ---
    
    /// Set the parent ID and return self (builder pattern)
    pub fn with_parent(mut self, parent_id: Uuid) -> Self {
        self.parent_id = Some(parent_id);
        self
    }
    
    /// Set the priority and return self (builder pattern)
    pub fn with_priority(mut self, priority: Priority) -> Self {
        self.priority = priority;
        self
    }
    
    /// Set the status and return self (builder pattern)
    pub fn with_status(mut self, status: Status) -> Self {
        self.status = status;
        self
    }
    
    /// Set the description and return self (builder pattern)
    pub fn with_description(mut self, description: &str) -> Self {
        self.description = Some(description.to_string());
        self
    }
    
    /// Set the due date and return self (builder pattern)
    pub fn with_due_date(mut self, due_date: u64) -> Self {
        self.due_date = Some(due_date);
        self
    }
}

impl fmt::Display for TodoItem {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let status_marker = match self.status {
            Status::Completed => "✓",
            Status::InProgress => "⊘",
            Status::NotStarted => "○",
        };
        
        let priority_marker = match self.priority {
            Priority::High => "!!!",
            Priority::Medium => "!!",
            Priority::Low => "!",
        };
        
        write!(f, "[{}] {} {}", status_marker, self.title, priority_marker)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_create_todo_item() {
        let item = TodoItem::new("Test Task");
        assert_eq!(item.title(), "Test Task");
        assert_eq!(item.status(), Status::NotStarted);
        assert_eq!(item.priority(), Priority::Medium);
    }
    
    #[test]
    fn test_status_changes() {
        let mut item = TodoItem::new("Task");
        assert!(!item.is_completed());
        
        item.set_status(Status::InProgress);
        assert_eq!(item.status(), Status::InProgress);
        assert!(!item.is_completed());
        
        item.mark_completed();
        assert!(item.is_completed());
    }
    
    #[test]
    fn test_metadata() {
        let mut item = TodoItem::new("Task with metadata");
        
        item.set_metadata("category", "work");
        item.set_metadata("context", "office");
        
        assert_eq!(item.metadata().get("category"), Some(&"work".to_string()));
        assert_eq!(item.metadata().get("context"), Some(&"office".to_string()));
        
        item.remove_metadata("context");
        assert!(item.metadata().get("context").is_none());
    }
} 