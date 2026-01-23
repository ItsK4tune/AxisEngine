# Documentation Structure

## Overview
AXIS Engine documentation is organized by **package** for easy navigation and maintenance.

---

## 📂 Directory Structure

```
docs/
├── core/                    # Core engine concepts
│   ├── architecture.md      # High-level architecture & diagrams
│   ├── ecs_overview.md      # ECS system deep dive
│   └── getting_started.md   # First game tutorial
│
├── state/                   # State management
│   └── state_api.md         # State API reference with patterns
│
├── scripting/               # Scripting system
│   └── scriptable_api.md    # Scriptable API reference with patterns
│
├── components/              # Component API references
│   ├── core.md              # Transform, Info, Camera
│   ├── rendering.md         # MeshRenderer, Material, Lights
│   ├── physics.md           # RigidBody
│   ├── ui.md                # UI components
│   ├── audio.md             # AudioSource
│   └── scripting.md         # ScriptComponent
│
├── managers/                # Manager API references
│   ├── resource_manager.md  # Asset loading
│   ├── scene_manager.md     # Scene management
│   ├── sound_manager.md     # Audio playback
│   ├── input_manager.md     # Input handling
│   └── physics_world.md     # Physics config & simulation
│
├── systems/                 # System API references
│   ├── render_system.md     # Rendering
│   ├── physics_system.md    # Physics simulation
│   ├── audio_system.md      # Audio processing
│   ├── ui_system.md         # UI rendering & interaction
│   ├── video_system.md      # Video playback
│   └── event_system.md      # Event handling
│
└── guides/                  # General guides
    ├── build_guide.md       # Build instructions
    ├── project_structure.md # Codebase layout
    ├── configuration.md     # Configuration options
    ├── scene_format.md      # Scene file syntax
    ├── components_reference.md  # Component quick reference
    ├── asset_management.md  # Asset loading guide
    ├── graphics_guide.md    # Graphics & rendering
    ├── post_processing.md   # Post-processing effects
    ├── debug_system.md      # Debug tools
    └── device_management.md # Device configuration
```

---

## 📖 Learning Path

### For Beginners
1. **Start Here:** [README.md](../README.md)
2. **Tutorial:** [Getting Started](core/getting_started.md)
3. **Understand ECS:** [ECS Overview](core/ecs_overview.md)
4. **Scripting:** [Scriptable API](scripting/scriptable_api.md)

### For Intermediate Developers
1. **Architecture:** [Architecture Overview](core/architecture.md)
2. **State Management:** [State API](state/state_api.md)
3. **Scene Format:** [Scene Format](guides/scene_format.md)
4. **Component Reference:** [Components](guides/components_reference.md)

### For Advanced Contributors
1. **Architecture Deep Dive:** [Architecture](core/architecture.md)
2. **ECS Internals:** [ECS Overview](core/ecs_overview.md)
3. **Manager APIs:** [Managers](managers/)
4. **System APIs:** [Systems](systems/)

---

## 📑 Document Categories

### Core Documentation
High-level concepts and architecture:
- **Architecture** - System design, execution flow, memory model
- **ECS Overview** - Entities, Components, Systems explained
- **Getting Started** - Complete tutorial with working examples

### API References
Detailed API documentation with examples:
- **State API** - State lifecycle, methods, patterns
- **Scriptable API** - Script lifecycle, input, physics, patterns
- **Components** - All component data structures
- **Managers** - Resource, Scene, Sound, Input managers
- **Systems** - Render, Physics, Audio, UI, Video systems

### Guides
Practical how-to guides:
- **Build Guide** - Compilation and setup
- **Scene Format** - .scene file syntax
- **Asset Management** - Loading resources
- **Graphics Guide** - Rendering techniques
- **Shadow Guide** - Shadow system setup & limits
- **Debug System** - F-key shortcuts and tools

---

## 🔗 Cross-References

### Most Referenced Documents
- [Architecture Overview](core/architecture.md) - Referenced by all docs
- [ECS Overview](core/ecs_overview.md) - Core concept for components/systems
- [Scene Format](guides/scene_format.md) - Used in all scene creation
- [Scriptable API](scripting/scriptable_api.md) - For all gameplay programming

### Common Navigation Paths
```
README.md
    → Getting Started
        → ECS Overview
            → Scriptable API
                → Component Reference

README.md
    → Architecture
        → State API / Scriptable API
            → Manager/System APIs
```

---

## 📊 Documentation Statistics

| Category | Files | Content |
|----------|-------|---------|
| Core | 3 | ~2,000 lines, Architecture + Tutorial |
| State | 1 | ~700 lines, Complete API reference |
| Scripting | 1 | ~850 lines, Complete API reference |
| Components | 6 | Component API references |
| Managers | 4 | Manager API references |
| Systems | 6 | System API references |
| Guides | 10 | Practical how-to guides |
| **Total** | **31 files** | **~20,000 words** |

---

## 🎯 Quick Links

### Most Important Documents
1. [Getting Started](core/getting_started.md) - Your first game
2. [Scriptable API](scripting/scriptable_api.md) - Gameplay programming
3. [State API](state/state_api.md) - State management
4. [ECS Overview](core/ecs_overview.md) - Core architecture
5. [Scene Format](guides/scene_format.md) - Scene creation

### API Quick Reference
- **State:** [State API](state/state_api.md)
- **Scripts:** [Scriptable API](scripting/scriptable_api.md)
- **Components:** [Component Docs](components/)
- **Managers:** [Manager Docs](managers/)
- **Systems:** [System Docs](systems/)

---

## 📝 Contributing to Documentation

When adding new documentation:
1. Place in appropriate package directory
2. Follow existing formatting (TOC, examples, cross-links)
3. Update this index
4. Update README.md links
5. Add cross-references to related docs

---

**Last Updated**: 2026-01-23
