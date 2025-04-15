mod todo_item;
mod todo_list;

pub use todo_item::{TodoItem, Status, Priority};
pub use todo_list::TodoList;

/// The core module contains the data structures for the todo list.
/// This includes the TodoItem and TodoList structures, as well as
/// supporting enums like Status and Priority.
pub mod prelude {
    pub use super::{TodoItem, TodoList, Status, Priority};
} 