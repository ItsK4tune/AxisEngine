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

| `bin/` | **Output Details**. The final executable is built here. |

> See [Configuration Guide](configuration.md) for build details.
