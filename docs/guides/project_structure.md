# Project Structure & Configuration

## Directory Layout

The project follows a modular structure to separate Engine core, Game logic, and Assets.

```text
AxisEngine/
├── src/               # Engine implementation (.cpp)
│   ├── audio/         # Audio engine & sound emitters
│   ├── core/          # Engine loop, application, logger, job system
│   ├── ecs/           # Entity-Component-System framework & builders
│   ├── navigation/    # NavMesh generation & Pathfinding
│   ├── physics/       # Bullet physics integration & queries
│   ├── platform/      # Windowing (GLFW), input, OpenGL context
│   ├── render/        # Rendering pipeline, shaders, textures, lights
│   ├── resource/      # Resource management & caching
│   ├── scene/         # Scene loading & management
│   ├── script/        # Scripting system base
│   └── third_party/   # Embedded libraries (stb, etc.)
├── include/          # Headers (.h)
│   ├── engine/        # Core Engine Headers
│   │   ├── audio/     # Audio interfaces & units
│   │   ├── core/      # Core logic & utilities
│   │   ├── ecs/       # ECS management & units
│   │   ├── navigation/# Navigation units & logic
│   │   ├── physics/   # Physics interfaces & units
│   │   ├── platform/  # Platform & input headers
│   │   ├── render/    # Rendering units & renderer logic
│   │   ├── resource/  # Resource management headers
│   │   ├── scene/     # Scene graph & serialization
│   │   └── script/    # Scriptable interfaces
│   └── entt/          # ECS Library (External)
├── game/              # User-Land Game Code
│   ├── include/      # Game-specific headers
│   └── src/           # Game-specific states & scripts
├── bin/               # Output binaries (Executables)
├── cmake/             # CMake build scripts
├── resources/         # Game Assets (Models, Textures, etc.)
└── scenes/            # Scene Definitions (.axs)
```

## Key Directories

### `src/` (Engine Core)
Contains the foundation of the AXIS Engine. Modifications here affect the entire engine.

### `game/` (Game Logic)
Place your specific game logic here. This separates your game mechanics from the engine core, making upgrades easier.

### `scenes/`
Contains the `.axs` scene definitions.

> See [Configuration Guide](configuration.md) for details on modifying settings.
