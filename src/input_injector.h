#pragma once

#include <string>

namespace InputInjector {

// Initialize the input subsystem (e.g., open /dev/uinput on Linux)
bool init();

// Cleanup resources
void shutdown();

// Move mouse by relative delta
void move_mouse_relative(int dx, int dy);

// Click a mouse button: 0=left, 1=right, 2=middle
void mouse_click(int button);

// Press and hold a mouse button
void mouse_down(int button);

// Release a mouse button  
void mouse_up(int button);

// Scroll the mouse wheel (positive = up, negative = down)
void mouse_scroll(int delta);

// Type a string of text (unicode-aware)
void type_text(const std::string& text);

// Press a special key by name: "enter", "tab", "backspace", "escape",
// "up", "down", "left", "right", "delete", "home", "end",
// "ctrl+c", "ctrl+v", "ctrl+a", "ctrl+z", "alt+tab", etc.
void press_key(const std::string& key);

} // namespace InputInjector
