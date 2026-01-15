# Project Structure & Configuration

## 1. Directory Structure

| Directory | Description |
| :--- | :--- |
| `src/` | **Engine Source**. Contains `.cpp` files for the Engine core. |
| `includes/` | **Engine Headers**. Contains `.h` files for Engine and third-party libs (Bullet, EnTT). |
| `game/src/` | **Game Source**. Contains `.cpp` files for Game logic (Scripts, States). |
| `game/includes/` | **Game Headers**. Contains `.h` files for Game specific code. |
| `resources/` | **Assets**. Stores Models, Shaders, Textures, Audio, Fonts. |
| `scenes/` | **Scene Files**. `.scene` files defining the game world. |
| `dlls/` | **Runtime Binaries**. Dynamic Link Libraries (Assimp, Freetype, etc.) needed by the executable. |
| `lib/` | **Static Libraries**. `.lib` files for linking during build. |
| `cmake/` | **CMake Modules**. Scripts for finding packages. |
| `bin/` | **Output Details**. The final executable is built here. |

| `bin/` | **Output Details**. The final executable is built here. |

> See [Configuration Guide](configuration.md) for build details.
