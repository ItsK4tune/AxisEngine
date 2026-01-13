# Project Structure & Configuration

## 1. Directory Structure

| Directory | Description |
| :--- | :--- |
| `src/` | **Source Code**. Contains `.cpp` files for Engine core and Game logic. |
| `includes/` | **Headers**. Contains `.h` files. Includes third-party libs (Bullet, EnTT) and Engine headers. |
| `resources/` | **Assets**. Stores Models, Shaders, Textures, Audio, Fonts. |
| `scenes/` | **Scene Files**. `.scene` files defining the game world. |
| `dlls/` | **Runtime Binaries**. Dynamic Link Libraries (Assimp, Freetype, etc.) needed by the executable. |
| `lib/` | **Static Libraries**. `.lib` files for linking during build. |
| `cmake/` | **CMake Modules**. Scripts for finding packages. |
| `bin/` | **Output Details**. The final executable is built here. |

---

## 2. Application Configuration

The application is configured in `main.cpp` using the `AppConfig` struct.

```cpp
AppConfig config;
config.title = "Game Engine";
config.width = 1280;
config.height = 720;
config.vsync = false;

Application app(config); // Application instance uses this config
```

---

## 3. Build System (CMake)

The project uses **CMake** for cross-platform build configuration.

### Configuration
- **Generators**: Visual Studio (MSVC) or Makefile.
- **Runtime Library**: 
    - Debug: `MDd` (Multi-threaded Debug DLL)
    - Release: `MD`
- **Output**: The executable is built into `bin/`.

### DLL Management
`CMakeLists.txt` is configured to automatically copy `.dll` files from the `dlls/` folder to the output directory (`bin/` or `build/Debug/`) after a successful build.

> **Important**: If you add new runtime libraries, place their `.dll` in `dlls/` so they are copied correctly.

---

## 4. Asset Loading

The engine uses a `FileSystem` helper to locate assets relative to the project root.

- **Root Directory**: The engine attempts to find the project root (where `resources/` is located).
- **Relativity**: In `.scene` files or C++ code, always use **relative paths** from the project root.

**Example:**
*   Correct: `resources/models/player.fbx`
*   Incorrect: `C:/Project/resources/models/player.fbx`

---

## 5. Adding Third-Party Libraries

To integrate a new library:

1.  **Headers**: Place `.h` files in `includes/` (or a subdirectory).
2.  **Static Libs**: Place `.lib` files in `lib/`.
3.  **Dynamic Libs**: Place `.dll` files in `dlls/`.
4.  **CMake**:
    *   Add `find_package` or manually verify the library in `CMakeLists.txt`.
    *   Update `target_link_libraries` to include the new `.lib`.
