# Application & Build Configuration
![AXIS Engine Logo](../assets/logo.png)

**Engine**: AXIS Engine  
**Contributor**: Duong "Caftun" Nguyen

## 1. Application Configuration

The AXIS Engine is configured via a JSON file located at `configuration/settings.json`. This file is loaded at startup.

### `settings.json` Structure

```json
{
    "title": "AXIS Engine",
    "width": 1280,
    "height": 720,
    "windowMode": 0,
    "vsync": true,
    "monitorIndex": 0,
    "refreshRate": 60,
    "frameRateLimit": 120,
    "shadowsEnabled": true,
    "cullFaceEnabled": true,
    "depthTestEnabled": true,
    "audioDevice": "default",
    "iconPath": "assets/icon.png"
}
```

### Parameters

- **Window Settings**
    - `title`: Text displayed in the window title bar.
    - `width`, `height`: Initial resolution of the window.
    - `windowMode`: Window display mode.
        - `0`: Windowed
        - `1`: Fullscreen
        - `2`: Borderless Windowed
    - `monitorIndex`: Index of the monitor to display on (0 = Primary, 1 = Secondary, etc.).
    - `vsync`: Enable Vertical Sync (locks FPS to refresh rate).
    - `refreshRate`: Target refresh rate (Hz) for Fullscreen mode.
    - `frameRateLimit`: Maximum frames per second (0 = Unlimited).
    - `iconPath`: Path to the window icon image (PNG/JPG).

- **Graphics Settings**
    - `shadowsEnabled`: Enable/Disable shadow mapping.
    - `cullFaceEnabled`: Enable/Disable back-face culling.
    - `depthTestEnabled`: Enable/Disable depth testing.

- **Audio Settings**
    - `audioDevice`: ID or Name of the audio output device (use "default" for system default). Use F2 in-game to see available device IDs.

## 2. CMake Build System

The project uses CMake for cross-platform build generation.

### Build Types
- **Debug**: Includes symbols, no optimization. Uses `MDd` runtime library.
- **Release**: Optimized. Uses `MD` runtime library.

### DLL Management
The build system automatically copies required DLLs (from `dlls/`) to the output binary directory (`bin/` or `build/Debug/`).

> **Important**: When adding a new library, ensure its `.dll` is placed in the `dlls/` folder so it can be found at runtime.

### Adding Libraries
To integrate a new third-party library:
1.  **Headers**: Place `.h` files in `includes/`.
2.  **Static Libs**: Place `.lib` files in `lib/`.
3.  **Dynamic Libs**: Place `.dll` files in `dlls/`.
4.  **CMake**: Update `CMakeLists.txt` to find and link the library.
