# AxisEngine documentation

> [Tiếng Việt](../vi/INDEX.md)

This documentation describes the repository at commit `787f438` (2026-07-23).
When a guide conflicts with code, the public headers, CMake options, sample
application, and tests are authoritative.

Vietnamese readers can start with the [Vietnamese documentation](../vi/INDEX.md).

## Start here

- [Manual](MANUAL.md): supported platforms, build, architecture, development
  workflow, deployment, and operational limits.
- [Getting started](core/getting_started.md): build and create a minimal
  application.
- [Build guide](guides/build_guide.md): presets, options, tests, installation,
  package consumption, and scene compilation.
- [Public API surface](core/api_surface.md): stable, plugin, advanced, and
  implementation-only boundaries.
- [Source audit, 2026-07-23](audit/source_audit_2026-07-23.md): correctness,
  security, performance, completeness, UX, and test findings.
- [Audit remediation, 2026-07-23](audit/remediation_2026-07-23.md):
  implemented fixes, verification, and remaining security boundary.

## Engine model

- [Architecture](core/architecture.md)
- [Core systems](systems/core_systems.md)
- [Managers and services](core/managers.md)
- [Project structure](guides/project_structure.md)
- [State API](state/state_api.md)
- [Scriptable API](scripting/scriptable_api.md)
- [Extending the engine](guides/extending_engine.md)
- [Comment policy](guides/comment_policy.md)

## Content and runtime guides

- [Scene format](guides/scene_format.md)
- [Component reference](guides/components_reference.md)
- [Assets and resources](guides/assets.md)
- [Configuration](guides/configuration.md)
- [Graphics](guides/graphics.md)
- [Physics](guides/physics.md)
- [Navigation](guides/navigation.md)
- [UI](guides/ui.md)
- [Audio](guides/audio.md)
- [Microphone capture](guides/audio_capture.md)
- [Device management](guides/device_management.md)
- [Debug controls](guides/debug_system.md)

## Editor

- [Editor manual](guides/editor.md)
- [Editor extensions](guides/editor_extensions.md)

## Documentation conventions

- `.axs` means AxisEngine's YAML-like subset, not general YAML.
- Paths are normally resolved relative to the detected application/project
  root. `asset://` addresses built-in engine assets.
- A feature listed in an enum is not necessarily a shipped backend. Consult
  the provider matrix in the manual.
- Code under `sample/` demonstrates behavior; it is not installed as part of
  the engine SDK.
