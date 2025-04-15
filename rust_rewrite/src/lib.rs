// This is the library entry point for the tewduwu application
// It exposes our core and UI modules for use in examples and binaries

pub mod core;
pub mod ui;

// Re-export commonly used types in the root module
pub use core::prelude;
pub use ui::prelude as ui_prelude; 