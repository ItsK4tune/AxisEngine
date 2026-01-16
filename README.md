# Game Engine Manual

Welcome to the documentation for the C++ Game Engine.

## Documentation Sections

### 1. Project & Config
- [Directory Structure](docs/project_structure.md)
- [Configuration & Build](docs/configuration.md)

### 2. Scene System
- [Scene File Format](docs/scene_format.md)
- [Component Reference](docs/components_reference.md)

### 3. Scripting
- [Scripting Basics (Lifecycle)](docs/scripting_basics.md)
- [Scripting API (Input, Physics)](docs/scripting_api.md)

### 4. Advanced Systems
- [Asset Management](docs/asset_management.md)
- [Graphics & Rendering](docs/graphics_guide.md)
- [Post Processing](docs/post_processing.md)

---

## Quick Start

1.  **Build**: Run `build_engine.bat` to Configure, Build, Run, or Clean the project.
    - Select cleaning options (Strict/Soft) for easy rebuilding.
2.  **Scene**: Create a `game.scene` in `scenes/` folder.
3.  **Run**: Launch the executable. It defaults to loading `scenes/game.scene`.

---

## System Overview

-   **ECS (Entity Component System)**: Built on `EnTT`.
-   **Rendering**: OpenGL 3.3+.
-   **Physics**: Bullet Physics.
-   **Scripting**: Native C++ Classes.
-   **Audio**: irrKlang.