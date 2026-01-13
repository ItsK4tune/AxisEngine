# Application & Build Configuration

## 1. Application Configuration

The application is configured in `main.cpp` using the `AppConfig` struct.

```cpp
AppConfig config;
config.title = "GameEngine";
config.width = 1280;
config.height = 720;
config.vsync = false; // Enable Vertical Sync

Application app(config);
```

**Parameters:**
- **title**: Window title bar text.
- **width/height**: Initial window resolution.
- **vsync**: Limits frame rate to monitor refresh rate if true.

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
