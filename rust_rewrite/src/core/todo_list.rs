use std::collections::{HashMap, HashSet};
use uuid::Uuid;
use super::todo_item::{TodoItem, Status, Priority};

/// TodoList manages a collection of TodoItems with hierarchy support
#[derive(Debug, Clone, serde::Serialize, serde::Deserialize)]
pub struct TodoList {
    /// The name of this todo list
    name: String,
    
    /// Map of item IDs to TodoItems
    items: HashMap<Uuid, TodoItem>,
    
    /// Map of parent IDs to child item IDs for quick hierarchy lookups
    hierarchy: HashMap<Option<Uuid>, HashSet<Uuid>>,
}

impl TodoList {
    /// Create a new, empty TodoList with the given name
    pub fn new(name: &str) -> Self {
        TodoList {
            name: name.to_string(),
            items: HashMap::new(),
            hierarchy: HashMap::new(),
        }
    }
    
    /// Get the name of this TodoList
    pub fn name(&self) -> &str {
        &self.name
    }
    
    /// Set the name of this TodoList
    pub fn set_name(&mut self, name: &str) {
        self.name = name.to_string();
    }
    
    /// Get the number of items in this TodoList
    pub fn len(&self) -> usize {
        self.items.len()
    }
    
    /// Check if this TodoList is empty
    pub fn is_empty(&self) -> bool {
        self.items.is_empty()
    }
    
    /// Add a TodoItem to the list
    pub fn add_item(&mut self, item: TodoItem) -> Uuid {
        // Store the item's ID and parent ID for hierarchy maintenance
        let id = item.id();
        let parent_id = item.parent_id();
        
        // Add item to the items map
        self.items.insert(id, item);
        
        // Update the hierarchy map
        self.hierarchy
            .entry(parent_id)
            .or_insert_with(HashSet::new)
            .insert(id);
             
        id
    }
    
    /// Create and add a new TodoItem with the given title
    pub fn create_item(&mut self, title: &str) -> Uuid {
        let item = TodoItem::new(title);
        self.add_item(item)
    }
    
    /// Get a reference to a TodoItem by ID
    pub fn get_item(&self, id: Uuid) -> Option<&TodoItem> {
        self.items.get(&id)
    }
    
    /// Get a mutable reference to a TodoItem by ID
    pub fn get_item_mut(&mut self, id: Uuid) -> Option<&mut TodoItem> {
        self.items.get_mut(&id)
    }
    
    /// Remove a TodoItem from the list
    /// 
    /// Returns the removed item if it existed, or None if it didn't
    pub fn remove_item(&mut self, id: Uuid) -> Option<TodoItem> {
        // First, check if the item exists
        if !self.items.contains_key(&id) {
            return None;
        }
        
        // Find and remove any children of this item
        if let Some(children) = self.hierarchy.remove(&Some(id)) {
            // Recursively remove all children
            for child_id in children {
                self.remove_item(child_id);
            }
        }
        
        // Remove the item from its parent's children list
        if let Some(parent_id) = self.items.get(&id).and_then(|item| item.parent_id()) {
            if let Some(siblings) = self.hierarchy.get_mut(&Some(parent_id)) {
                siblings.remove(&id);
            }
        } else {
            // No parent, so remove from root items
            if let Some(root_items) = self.hierarchy.get_mut(&None) {
                root_items.remove(&id);
            }
        }
        
        // Finally, remove the item itself
        self.items.remove(&id)
    }
    
    /// Get all root items (items with no parent)
    pub fn root_items(&self) -> Vec<&TodoItem> {
        match self.hierarchy.get(&None) {
            Some(root_ids) => root_ids
                .iter()
                .filter_map(|id| self.items.get(id))
                .collect(),
            None => Vec::new(),
        }
    }
    
    /// Get IDs of all root items
    pub fn root_item_ids(&self) -> Vec<Uuid> {
        match self.hierarchy.get(&None) {
            Some(root_ids) => root_ids.iter().copied().collect(),
            None => Vec::new(),
        }
    }
    
    /// Get all child items of a given parent
    pub fn children(&self, parent_id: Uuid) -> Vec<&TodoItem> {
        match self.hierarchy.get(&Some(parent_id)) {
            Some(child_ids) => child_ids
                .iter()
                .filter_map(|id| self.items.get(id))
                .collect(),
            None => Vec::new(),
        }
    }
    
    /// Get IDs of all child items of a given parent
    pub fn child_ids(&self, parent_id: Uuid) -> Vec<Uuid> {
        match self.hierarchy.get(&Some(parent_id)) {
            Some(child_ids) => child_ids.iter().copied().collect(),
            None => Vec::new(),
        }
    }
    
    /// Move an item to be a child of another item
    /// 
    /// Returns `Ok(())` if successful, or an error message if not.
    pub fn move_item(&mut self, item_id: Uuid, new_parent_id: Option<Uuid>) -> Result<(), String> {
        // Check if the item exists
        if !self.items.contains_key(&item_id) {
            return Err(format!("Item with ID {} not found", item_id));
        }
        
        // If there's a new parent, check if it exists
        if let Some(parent_id) = new_parent_id {
            if !self.items.contains_key(&parent_id) {
                return Err(format!("Parent item with ID {} not found", parent_id));
            }
            
            // Check for cycles: an item can't be its own ancestor
            if parent_id == item_id || self.is_ancestor(item_id, parent_id) {
                return Err("Moving this item would create a cycle".to_string());
            }
        }
        
        // Get the current parent ID
        let current_parent_id = self.items.get(&item_id).and_then(|item| item.parent_id());
        
        // Remove from current parent's children
        if let Some(current_parent) = self.hierarchy.get_mut(&current_parent_id) {
            current_parent.remove(&item_id);
        }
        
        // Add to new parent's children
        self.hierarchy
            .entry(new_parent_id)
            .or_insert_with(HashSet::new)
            .insert(item_id);
            
        // Update the item's parent_id
        if let Some(item) = self.items.get_mut(&item_id) {
            item.set_parent_id(new_parent_id);
        }
        
        Ok(())
    }
    
    /// Check if one item is an ancestor of another
    fn is_ancestor(&self, item_id: Uuid, potential_ancestor_id: Uuid) -> bool {
        // Get the item's parent
        let parent_id = match self.items.get(&item_id).and_then(|item| item.parent_id()) {
            Some(id) => id,
            None => return false, // No parent, so definitely not an ancestor
        };
        
        // Check if the parent is the potential ancestor
        if parent_id == potential_ancestor_id {
            return true;
        }
        
        // Recursively check the parent's ancestors
        self.is_ancestor(parent_id, potential_ancestor_id)
    }
    
    /// Get all items matching a filter function
    pub fn filter_items<F>(&self, filter_fn: F) -> Vec<&TodoItem>
    where
        F: Fn(&TodoItem) -> bool,
    {
        self.items
            .values()
            .filter(|item| filter_fn(item))
            .collect()
    }
    
    /// Get all completed items
    pub fn completed_items(&self) -> Vec<&TodoItem> {
        self.filter_items(|item| item.is_completed())
    }
    
    /// Get all incomplete items
    pub fn incomplete_items(&self) -> Vec<&TodoItem> {
        self.filter_items(|item| !item.is_completed())
    }
    
    /// Get items by priority
    pub fn items_by_priority(&self, priority: Priority) -> Vec<&TodoItem> {
        self.filter_items(|item| item.priority() == priority)
    }
    
    /// Get items by status
    pub fn items_by_status(&self, status: Status) -> Vec<&TodoItem> {
        self.filter_items(|item| item.status() == status)
    }
    
    /// Get all overdue items
    pub fn overdue_items(&self) -> Vec<&TodoItem> {
        self.filter_items(|item| item.is_overdue())
    }
    
    /// Get all items as a flat list
    pub fn all_items(&self) -> Vec<&TodoItem> {
        self.items.values().collect()
    }
    
    /// Get all items as a vector of references ordered by a specified criterion
    pub fn sorted_items<F, K>(&self, key_fn: F) -> Vec<&TodoItem>
    where
        F: Fn(&TodoItem) -> K,
        K: Ord,
    {
        let mut items: Vec<&TodoItem> = self.items.values().collect();
        items.sort_by_key(|item| key_fn(*item));
        items
    }
    
    /// Get a hierarchical representation of the todo list
    ///
    /// Returns a vector of (item, depth) pairs in a pre-order traversal,
    /// where depth is the nesting level (0 for root items).
    pub fn hierarchical_view(&self) -> Vec<(&TodoItem, usize)> {
        let mut result = Vec::with_capacity(self.items.len());
        
        // Helper function for recursive traversal
        fn traverse<'a>(
            list: &'a TodoList,
            parent_id: Option<Uuid>,
            depth: usize,
            result: &mut Vec<(&'a TodoItem, usize)>,
        ) {
            // Get children of this parent
            let child_ids = match parent_id {
                Some(id) => list.child_ids(id),
                None => list.root_item_ids(),
            };
            
            // Add each child to the result, then traverse its children
            for id in child_ids {
                if let Some(item) = list.get_item(id) {
                    result.push((item, depth));
                    traverse(list, Some(id), depth + 1, result);
                }
            }
        }
        
        // Start traversal from root items
        traverse(self, None, 0, &mut result);
        
        result
    }
    
    /// Move an item to be positioned before another item
    /// 
    /// Both items should have the same parent for this to work properly.
    /// If target_id is not found, the item will be moved to the end of its parent's children.
    /// 
    /// Returns `Ok(())` if successful, or an error message if not.
    pub fn move_item_before(&mut self, item_id: Uuid, target_id: Uuid) -> Result<(), String> {
        // Check if both items exist
        if !self.items.contains_key(&item_id) {
            return Err(format!("Item with ID {} not found", item_id));
        }
        if !self.items.contains_key(&target_id) {
            return Err(format!("Target item with ID {} not found", target_id));
        }
        
        // Get the parent IDs for both items
        let item_parent_id = match self.items.get(&item_id) {
            Some(item) => item.parent_id(),
            None => return Err("Item not found".to_string()),
        };
        
        let target_parent_id = match self.items.get(&target_id) {
            Some(item) => item.parent_id(),
            None => return Err("Target item not found".to_string()),
        };
        
        // If the parents are different, we need to move the item to the target's parent first
        if item_parent_id != target_parent_id {
            self.move_item(item_id, target_parent_id)?;
        }
        
        // Now both items have the same parent, so we can reorder
        let parent_id = target_parent_id;
        
        // Get all children of the parent
        let children = match parent_id {
            Some(pid) => self.child_ids(pid),
            None => self.root_item_ids(),
        };
        
        // Create a new ordered list of child IDs
        let mut new_order = Vec::with_capacity(children.len());
        
        // If the item is already in the list, we'll need to remove it first
        // to avoid duplicates when we insert it at the new position
        let mut item_included = false;
        
        // Build the new order of children
        for child_id in children {
            if child_id == item_id {
                // Skip this for now, we'll insert it at the right position
                item_included = true;
                continue;
            }
            
            if child_id == target_id {
                // Insert our item before the target
                new_order.push(item_id);
            }
            
            new_order.push(child_id);
        }
        
        // If we haven't added the item yet (target not found or item at the end),
        // add it to the end of the list
        if !item_included && !new_order.contains(&item_id) {
            new_order.push(item_id);
        }
        
        // Update the hierarchy map with the new order
        let entry = self.hierarchy.entry(parent_id).or_insert_with(HashSet::new);
        entry.clear();
        for id in new_order {
            entry.insert(id);
        }
        
        Ok(())
    }
    
    /// Find the index of an item by its ID
    pub fn find_item_index(&self, id: &Uuid) -> Option<Uuid> {
        if self.items.contains_key(id) {
            Some(*id)
        } else {
            None
        }
    }
    
    /// Replace an item at a specific index with a new item
    pub fn replace_item_at_index(&mut self, id: Uuid, new_item: TodoItem) -> Option<TodoItem> {
        if !self.items.contains_key(&id) {
            return None;
        }
        
        // Get the original item to preserve hierarchy relationships
        let original_item = self.items.get(&id).cloned();
        
        if let Some(original) = original_item {
            // Insert the new item, preserving parent-child relationships
            let mut item_to_insert = new_item;
            item_to_insert.set_parent_id(original.parent_id());
            
            // Replace the item in the map
            self.items.insert(id, item_to_insert.clone());
            
            Some(item_to_insert)
        } else {
            None
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_create_and_add_items() {
        let mut list = TodoList::new("Test List");
        
        // Create an item via TodoList
        let id1 = list.create_item("Task 1");
        
        // Create and add an item via TodoItem
        let item2 = TodoItem::new("Task 2");
        let id2 = list.add_item(item2);
        
        assert_eq!(list.len(), 2);
        assert_eq!(list.get_item(id1).unwrap().title(), "Task 1");
        assert_eq!(list.get_item(id2).unwrap().title(), "Task 2");
    }
    
    #[test]
    fn test_hierarchical_structure() {
        let mut list = TodoList::new("Hierarchy Test");
        
        // Create parent task
        let parent_id = list.create_item("Parent Task");
        
        // Create child tasks
        let child1_id = list.create_item("Child 1");
        let child2_id = list.create_item("Child 2");
        
        // Move children under the parent
        list.move_item(child1_id, Some(parent_id)).unwrap();
        list.move_item(child2_id, Some(parent_id)).unwrap();
        
        // Check parent-child relationships
        assert_eq!(list.children(parent_id).len(), 2);
        assert!(list.child_ids(parent_id).contains(&child1_id));
        assert!(list.child_ids(parent_id).contains(&child2_id));
        
        // Verify hierarchy view
        let hierarchy = list.hierarchical_view();
        assert_eq!(hierarchy.len(), 3);
        
        // Parent should be first, at depth 0
        assert_eq!(hierarchy[0].0.id(), parent_id);
        assert_eq!(hierarchy[0].1, 0);
        
        // Children should follow, at depth 1
        assert_eq!(hierarchy[1].1, 1);
        assert_eq!(hierarchy[2].1, 1);
    }
    
    #[test]
    fn test_remove_item_with_children() {
        let mut list = TodoList::new("Removal Test");
        
        // Create parent task
        let parent_id = list.create_item("Parent Task");
        
        // Create child tasks
        let child1_id = list.create_item("Child 1");
        let child2_id = list.create_item("Child 2");
        
        // Move children under the parent
        list.move_item(child1_id, Some(parent_id)).unwrap();
        list.move_item(child2_id, Some(parent_id)).unwrap();
        
        // Remove the parent (should also remove children)
        list.remove_item(parent_id);
        
        // List should be empty
        assert_eq!(list.len(), 0);
        assert!(list.get_item(parent_id).is_none());
        assert!(list.get_item(child1_id).is_none());
        assert!(list.get_item(child2_id).is_none());
    }
    
    #[test]
    fn test_filtering() {
        let mut list = TodoList::new("Filter Test");
        
        // Create items with different statuses and priorities
        let id1 = list.create_item("High Priority Task");
        let id2 = list.create_item("Medium Priority Task");
        let id3 = list.create_item("Completed Task");
        
        // Set properties
        list.get_item_mut(id1).unwrap().set_priority(Priority::High);
        list.get_item_mut(id2).unwrap().set_priority(Priority::Medium);
        list.get_item_mut(id3).unwrap().set_status(Status::Completed);
        
        // Test filters
        assert_eq!(list.items_by_priority(Priority::High).len(), 1);
        assert_eq!(list.items_by_priority(Priority::Medium).len(), 1);
        assert_eq!(list.completed_items().len(), 1);
        assert_eq!(list.incomplete_items().len(), 2);
    }
    
    #[test]
    fn test_cycle_prevention() {
        let mut list = TodoList::new("Cycle Test");
        
        // Create a chain of tasks: A -> B -> C
        let id_a = list.create_item("Task A");
        let id_b = list.create_item("Task B");
        let id_c = list.create_item("Task C");
        
        list.move_item(id_b, Some(id_a)).unwrap();
        list.move_item(id_c, Some(id_b)).unwrap();
        
        // Trying to make A a child of C would create a cycle
        assert!(list.move_item(id_a, Some(id_c)).is_err());
    }
} 