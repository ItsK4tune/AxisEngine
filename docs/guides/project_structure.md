# Project Structure & Configuration

## Directory Layout

The project follows a modular structure to separate Engine core, Game logic, and Assets.

```text
GameEngine/
├── src/
│   ├── asset/
│   │   ├── project/       # Project specific assets (icon.png, logo.png)
│   │   ├── shaders/       # Shader files
│   │   └── ...            # Other asset types (textures, models, fonts)
│   ├── app/         # Application, Window, Config
│   ├── audio/       # Sound System
│   ├── debug/       # Debug System
│   ├── ecs/         # ECS Systems & Components
│   ├── event/       # Event System
│   ├── graphic/     # Rendering modules
│   ├── input/       # Input Managers (Mouse, Keyboard)
│   ├── physic/      # Physics Wrapper
│   ├── resource/    # Resource Manager
│   ├── scene/       # Scene Management
│   ├── script/      # Scripting Implementation
│   ├── state/       # State Machine
│   ├── third_party/ # Vendor Code (glad, stb_image)
│   └── utils/       # Helpers
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
│   │   ├── app/     # App Lifecycle & Windowing
│   │   ├── interface/# Interfaces (IDeviceManager)
│   │   ├── input/   # Input Management
│   │   ├── scene/   # Scene Graph & Loading
│   │   ├── script/  # Scripting Base
│   │   └── ...      # Other modules (audio, resource, etc.)
│   └── entt/        # ECS Library
├── lib/             # Static Libraries (.lib)
├── resources/       # Game Assets
├── scenes/          # Scene Definitions (.scene)
└── src/             # Core Engine Source Code
    ├── app/         # Application, Window, Config
    ├── audio/       # Sound System
    ├── debug/       # Debug System
    ├── ecs/         # ECS Systems & Components
    ├── event/       # Event System
    ├── graphic/     # Rendering modules
    ├── input/       # Input Managers (Mouse, Keyboard)
    ├── physic/      # Physics Wrapper
    ├── resource/    # Resource Manager
    ├── scene/       # Scene Management
    ├── script/      # Scripting Implementation
    ├── state/       # State Machine
    ├── third_party/ # Vendor Code (glad, stb_image)
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
