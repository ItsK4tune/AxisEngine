# Game Engine Manual

Welcome to the documentation for the C++ Game Engine.

## Documentation Sections

### 1. [Project Structure & Config](docs/project_structure.md)
Overview of folders, `AppConfig`, CMake setup, and how to add new libraries/assets.

### 2. [Scene File Format](docs/scene_format.md)
Reference for the `.scene` file based entity definition system. Learn how to:
- Load Resources (Models, Shaders, Animations).
- Define Entities.
- Configure Components (Transform, Physics, Lighting, UI).
- Add Audio Sources.

### 3. [Scripting Guide](docs/scripting_guide.md)
Learn how to write C++ scripts (`Scriptable`) to add logic to your entities.
- Lifecycle (`OnCreate`, `OnUpdate`).
- Input Handling (Key/Mouse Actions).
- Physics Callbacks (`OnCollisionEnter`, `OnTriggerEnter`).
- Component Interaction.

### 4. [Post Processing](docs/post_processing.md)
How to apply screen-space effects like Inversion, Grayscale, or custom shaders.

---

## Quick Start

1.  **Build**: Open the project in Visual Studio or use Makefile/CMake.
2.  **Scene**: Create a `game.scene` in `scenes/` folder.
3.  **Run**: Launch the executable. It defaults to loading `scenes/game.scene`.

---

## System Overview

-   **ECS (Entity Component System)**: Built on `EnTT`.
-   **Rendering**: OpenGL 3.3+.
-   **Physics**: Bullet Physics.
-   **Scripting**: Native C++ Classes.
-   **Audio**: irrKlang.