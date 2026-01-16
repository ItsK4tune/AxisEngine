# Application & Build Configuration

## 1. Application Configuration

The application is configured in `main.cpp` using the `AppConfig` struct.

```cpp
AppConfig config;
config.title = "GameEngine";
config.width = 1280;
config.height = 720;
    config.mode = WindowMode::WINDOWED;
    config.monitorIndex = 0; // Select Monitor
    config.refreshRate = 0; // 0 = Unlimited/Default
    config.frameRateLimit = 0; // 0 = Unlimited
    
    Application app(config);
    ```
    
    **Parameters:**
    - **title**: Window title bar text.
    - **width/height**: Initial window resolution.
    - **vsync**: Limits frame rate to monitor refresh rate if true (via `glfwSwapInterval`).
    - **mode**: `WINDOWED`, `FULLSCREEN`, or `BORDERLESS`.
    - **monitorIndex**: Index of the monitor to display on (0 = Primary).
    - **refreshRate**: Target refresh rate (Hz) for Fullscreen mode.
    - **frameRateLimit**: Cap FPS to save CPU/GPU (0 = Unlimited).

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

## 3. Scene-Based Configuration
You can configure engine settings directly in your `.scene` file using the `CONFIG` command.

### Window Settings
```text
CONFIG WINDOW <width> <height> [mode] [monitor_idx] [refresh_rate]
```
- **width/height**: Resolution.
- **mode**: `WINDOWED` (default), `FULLSCREEN`, `BORDERLESS`.
- **monitor_idx**: 0, 1, etc.
- **refresh_rate**: Target Hz (useful for Fullscreen).

### Graphics Settings
```text
CONFIG SHADOWS <0/1>           # Enable/Disable Shadows
CONFIG VSYNC <0/1>             # Enable/Disable VSync
CONFIG FPS <limit>             # Frame Rate Cap (0 = Unlimited)
CONFIG CULL_FACE <0/1> [mode]  # Face Culling (mode: FRONT, BACK, FRONT_AND_BACK)
CONFIG DEPTH_TEST <0/1> [func] # Depth Test (func: LESS, ALWAYS, etc.)
```
