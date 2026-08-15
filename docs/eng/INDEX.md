# AxisEngine Documentation Index

> [Tiếng Việt](../vi/INDEX.md) | [User Manual](MANUAL.md) | [Root README](../README.eng.md)

Technical reference manuals, API guides, and architectural documentation for developers building C++20 applications with AxisEngine.

---

## 1. Core Architecture & Foundation
- [Getting Started](core/getting_started.md): Installation, project setup, and minimal application creation.
- [Architecture Overview](core/architecture.md): EnTT ECS registry, thread execution loop, state machine, Entity, Scene, and EntityBuilder API.
- [Public API Surface](core/api_surface.md): SDK header tiers (`axis_sdk.h`, `axis_plugin.h`, `axis_advanced.h`).
- [Managers & Services](core/managers.md): `ResourceManager`, `EventManager`, `InputManager`, `SceneManager`, and `JobSystem`.

---

## 2. Build & Configuration
- [Build Guide](guides/build_guide.md): CMake presets, compiler flags, MSVC/GCC configuration, and package exporting.
- [Configuration Reference](guides/configuration.md): Global engine configuration properties, graphics/physics settings, and runtime optimizations.
- [Project Structure](guides/project_structure.md): Organization of engine directories, CMake targets, and user assets.
- [Comment Policy](guides/comment_policy.md): Coding style conventions and internal code comment standards.

---

## 3. Scene & Components
- [Scene Format (`.axs` / `.axsb`)](guides/scene_format.md): Reference for the five `.axs` schemas (`axis_scene`, `axis_input`, `axis_data`, `axis_localization`, `axis_config`) and binary compiler usage.
- [Components Reference](guides/components_reference.md): Reference for all built-in ECS components and `EntityBuilder` C++ creation API.
- [Assets & Resource Management](guides/assets.md): Texture/model loading, hot-reloading, and resource streaming.

---

## 4. Subsystems
- [Graphics & Renderer](guides/graphics.md): OpenGL 4.6 PBR renderer, lighting, shadows, post-processing, particles, and terrain.
- [Physics Simulation](guides/physics.md): Bullet 3D integration, rigidbodies, colliders, character controllers, and raycasting.
- [Audio Playback](guides/audio.md): 2D/3D sound playback with Null, FMOD, and irrKlang backends.
- [Audio Capture](guides/audio_capture.md): WASAPI microphone streaming, noise gates, and voice reactivity.
- [User Interface (UI)](guides/ui.md): Canvas-based UI components, buttons, text, anchoring, and layout rules.
- [Navigation System](guides/navigation.md): Recast/Detour NavMesh generation, pathfinding, and spatial hashing.
- [Device Management](guides/device_management.md): Display modes, resolution scaling, and input device handling.

---

## 5. Scripting, State & Systems
- [Scriptable API](scripting/scriptable_api.md): C++ `Scriptable` lifecycle hooks, entity queries, and variable exposure.
- [State API](state/state_api.md): Stack-based `StateMachine`, state transitions, and modal overlays.
- [Core Systems](systems/core_systems.md): Registering custom `ISystem` instances and frame phase execution order.
- [Extending the Engine](guides/extending_engine.md): Building custom plugins, providers, and renderer strategies.

---

## 6. Tools & Debugging
- [Debug & Editor Controls](guides/debug_system.md): Keyboard shortcuts (F1-F12), debug GUI panels, `AXIS_LOG_*` logging, `AXIS_ASSERT` macros, `DebugConfig`, physics/audio drawers, and profiling.
- [Editor Manual](guides/editor.md): Visual scene editing, hierarchy, inspector, prefabs, and project management with ImGui.
- [Editor Extensions](guides/editor_extensions.md): Writing custom editor windows, inspector drawers, and wizard panels.
