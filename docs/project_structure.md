# Project Structure & Configuration

## 1. Directory Structure

| Directory | Description |
| :--- | :--- |
| `src/` | Source code (`.cpp`) for Engine and Game logic. |
| `includes/` | Header files (`.h`). Includes third-party libs (Bullet, EnTT) and Engine headers. |
| `resources/` | Game assets (Models, Shaders, Textures, Audio). |
| `scenes/` | Scene definition files (`.scene`). |
| `dlls/` | Dynamic Link Libraries (`.dll`) required at runtime (Assimp, Freetype, etc.). |
| `lib/` | Static libraries (`.lib`) for linking. |
| `cmake/` | CMake modules for finding packages. |
| `bin/` | Output directory for the executable (created after build). |

## 2. Application Configuration

The application is configured in `main.cpp` using `AppConfig`.

```cpp
AppConfig config;
config.title = "Game Engine";
config.width = 1280;
config.height = 720;
config.vsync = false;

Application app(config); // Application instance uses this config
```

## 3. Build System (CMake)

The project uses **CMake**.
- **Generators**: Visual Studio (MSVC) or Makefile.
- **Runtime Library**: Debug builds often use `MDd` (Multi-threaded Debug DLL), Release uses `MD`.
- **Output**: The executable is built into `bin/`.

### DLL Management
`CMakeLists.txt` is configured to automatically copy `.dll` files from `dlls/` to the output directory (`bin/` or `build/Debug/`) after a successful build.
- **Important**: If you add new libraries, place their `.dll` in `dlls/` so they are copied correctly.

## 4. Asset Loading

The engine uses a `FileSystem` helper to locate assets.
- **Root Directory**: The engine attempts to find the project root (where `resources/` is located).
- **Paths**: In `.scene` files or C++ code, always use **relative paths** from the project root.

**Example:**
- Correct: `resources/models/player.fbx`
- Incorrect: `C:/Project/resources/models/player.fbx`

## 5. Adding Third-Party Libraries

1.  **Headers**: Place `.h` files in `includes/` (or a subdirectory).
2.  **Static Libs**: Place `.lib` files in `lib/`.
3.  **Dynamic Libs**: Place `.dll` files in `dlls/`.
4.  **CMake**:
    *   Add `find_package` or manually verify the library in `CMakeLists.txt`.
    *   Update `target_link_libraries` to include the new `.lib`.
