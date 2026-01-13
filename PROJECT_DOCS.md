# Game Engine Documentation

## Core Systems

### 1. RenderSystem
The `RenderSystem` handles modern OpenGL rendering techniques, including:
- **Shadow Mapping**: Supports both Directional and Point Lights (Omnidirectional Shadow Maps).
- **Shadow Class**: Encapsulates shadow resources (FBOs, Textures, Shaders).
- **Batching/Sorting**: Sorts entities by shader to minimize state changes.
- **Frustum Culling**: Optimized culling (basic AABB support implemented).

### 2. ResourceManager
Manages lifecycle of assets:
- **Shaders**: Unified `LoadShader` for Vertex, Fragment, and optional Geometry shaders.
- **Textures, Models, Fonts**: Handles diverse asset types with `stb_image`, `Assimp`, and `FreeType`.

## Architecture
- **ECS (Entity Component System)**: Uses `EnTT` for high-performance entity management.
- **Component-Based**: Logic is split into `Systems` (Render, Physics, Script) operating on data `Components`.

## Build Instructions
1.  Run `clean_build.bat` in the root directory to clean `build/` and `bin/` folders and rebuild from scratch.
2.  Or use CMake: `cmake -B build` then `cmake --build build`.
