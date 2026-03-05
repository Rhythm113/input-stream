# Input Stream

> Remote mouse & keyboard controller via a web portal served from your machine.

**Developer:** Rhythm113

## Overview

Input Stream is a cross-platform C++ application that opens a web server on **port 9999** and serves a mobile-friendly portal. Connect from any device on your local network to control your host machine's mouse and keyboard using kernel-level input APIs.

### Features

-  **Trackpad** - Drag to move the cursor, tap to click, two-finger tap for right-click
-  **Keyboard Input** - Type directly from your phone/tablet
-  **Special Keys** - Enter, Tab, Backspace, Arrows, Ctrl+C/V/Z, Alt+Tab, and more
-  **Scroll** - Two-finger swipe or dedicated scroll strip
-  **Dark UI** - Premium glassmorphism design, fully mobile responsive
-  **Cross-Platform** - Windows, Linux, and macOS

### Platform Input APIs

| Platform | API | Notes |
|----------|-----|-------|
| Windows  | `SendInput()` | Works from any interactive session |
| Linux    | `/dev/uinput` | Requires `root` or `input` group membership |
| macOS    | `CGEventPost()` | Requires Accessibility permissions |

## Build

### Prerequisites

- **CMake** ≥ 3.15
- **C++17** compiler (MSVC, GCC, or Clang)

### Windows

```bash
cmake -B build -S .
cmake --build build --config Release
```

Run: `build\Release\input_stream.exe`

### Linux

```bash
cmake -B build -S .
cmake --build build
```

Run: `sudo ./build/input_stream` (or add your user to the `input` group)

### macOS

```bash
cmake -B build -S .
cmake --build build
```

Run: `./build/input_stream` (grant Accessibility access when prompted)

## Usage

1. Run the executable on your computer
2. Note the IP address printed in the console
3. Open `http://<your-ip>:9999` on your phone or any browser on the same network
4. Use the trackpad area to move the mouse, the text field to type, and the special key buttons for shortcuts

## License

MIT
