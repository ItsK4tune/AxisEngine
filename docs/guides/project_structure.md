# Project Structure & Configuration

## Directory Layout

The project follows a modular structure to separate Engine core, Game logic, and Assets.

```text
GameEngine/
├── assets/          # Branding assets (Logo, Icon)
├── bin/             # Output binaries (Executables)
├── cmake/           # CMake modules and scripts
├── configuration/   # JSON Configuration files (settings.json)
├── dlls/            # Runtime DLLs (Assimp, Freetype, irrKlang, etc.)
├── docs/            # Project Documentation
├── game/            # User-Land Game Code
│   ├── includes/    # Game-specific headers
│   └── src/         # Game-specific logic (Scripts, States)
├── includes/        # Engine & Library Headers
│   ├── bullet/      # Physics Library
│   ├── engine/      # Core Engine Headers
│   └── entt/        # ECS Library
├── lib/             # Static Libraries (.lib)
├── resources/       # Game Assets
│   ├── audio/       # Sound files
│   ├── fonts/       # TTF Fonts
│   ├── models/      # 3D Models (FBX, OBJ)
│   ├── shaders/     # GLSL Shaders
│   └── textures/    # Image files
├── scenes/          # Scene Definitions (.scene)
└── src/             # Core Engine Source Code
    ├── core/        # Core systems
    ├── ecs/         # Component systems
    ├── graphic/     # Rendering modules
    └── utils/       # Helpers
```

## Key Directories

### `src/` (Engine Core)
Contains the foundation of the AXIS Engine. Modifications here affect the entire engine.

### `game/` (Game Logic)
Place your specific game logic here. This separates your game mechanics from the engine core, making upgrades easier.

### `configuration/`
Contains `settings.json` which controls the startup behavior (Window size, Title, VSync, etc.).

> See [Configuration Guide](configuration.md) for details on modifying settings.
