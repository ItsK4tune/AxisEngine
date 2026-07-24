# AxisEngine editor guide

> [Tiếng Việt](../../vi/guides/editor.md)

Updated: 2026-07-23. Scope: builds with `ENABLE_EDITOR=ON`.

## Architecture

The editor is registered through the `Axis::Editor` bootstrap and organized
around shared selection, command history, viewport state, input routing,
modules, and panels. Modules own per-frame tool/overlay logic; panels own ImGui
windows. Extensions register factories through `IEditorExtensionRegistry` with
a stable owner ID.

The game renders directly to the platform backbuffer. The central dockspace is
transparent and editor UI overlays it; there is no second editor G-buffer or
mandatory viewport texture copy.

## Selection and transforms

- Click or box-drag in the viewport to select through the ID buffer.
- Hold `Ctrl` to add/toggle selection.
- Hierarchy `Shift+click` selects a range.
- The most recently selected entity is primary and drives the Inspector.
- Selected entities use the selection-outline SSBO.
- Transient/editor-only entities appear in the hierarchy but are excluded from
  normal scene serialization.

The viewport gizmo supports move, rotate, scale, world/local orientation,
pivot/center placement, handles, and snapping. One drag is one undo transaction.

Keyboard nudge:

- `Alt` + arrows/PageUp/PageDown: move.
- `Ctrl+Alt` + arrows/PageUp/PageDown: rotate.
- `Shift+Alt` + arrows/PageUp/PageDown: scale.

## Play/edit safety and history

- Play captures a complete edit-scene snapshot.
- Pause stops runtime progression.
- Stop restores the snapshot, discards runtime mutations, and resets history.
- `Ctrl+S` saves and updates the dirty baseline.
- `Ctrl+R` reloads immediately when clean and asks before discarding dirty work.
- Undo: `Ctrl+Z`; redo: `Ctrl+Shift+Z`.

Keep the repository under version control. File and prefab operations write real
project content. File Hierarchy canonicalizes paths beneath the project root,
creates files without replacement, generates unique duplicate names, and
refuses rename conflicts. Explicit save/apply operations still replace their
selected output.

## Panels

The built-in set includes Scene Hierarchy, Project/Assets, Inspector, Tools,
Settings, Profiler, Console, State, Network, Help, Animation Graph, VFX Graph,
Input Actions, Navigation, Frame Debugger, Lighting, Prefabs, File Hierarchy,
and Resource Browser.

`Project / Assets` handles project browsing, imports, dependencies, reimport,
and loaded-resource inspection. `Prefabs` uses fragment `.axs` assets for linked
instances and overrides. `Lighting` queues probe capture and can bake simple
UV-space PPM lightmaps for static meshes. `Navigation` edits navmesh generation
and cost rules.

## Shortcuts

Panel bank 1:

- `Ctrl+1` Hierarchy; `Ctrl+2` Project/Assets; `Ctrl+3` Inspector;
  `Ctrl+4` Tools; `Ctrl+5` Settings; `Ctrl+6` Profiler; `Ctrl+7` Console;
  `Ctrl+8` State; `Ctrl+9` Network; `Ctrl+0` Help.

Panel bank 2:

- `Ctrl+Shift+1` Animation Graph; `+2` VFX Graph; `+3` Input Actions;
  `+4` Navigation; `+5` Frame Debugger; `+6` Lighting; `+7` Prefabs.

Function keys:

- `F1` entity names, `F2` gizmos, `F3` light gizmos, `F4` skybox,
  `F5` shadows, `F6` post-process, `F7` physics debug, `F8` audio debug,
  `F9` particle debug, `F10` editor cursor, `Shift+F10` debug camera,
  `F11` pause/resume, `F12` cycle time scale.

Editor shortcuts require exact modifiers. When the editor/ImGui owns keyboard
input, gameplay does not receive those events.

## Debug camera

- `F10` toggles editor cursor ownership.
- `Shift+F10` switches between game and transient Debug Camera.
- Hold RMB + WASD; `Q/E` moves vertically.
- MMB pans; `Alt+LMB` orbits; scroll dollies.
- `F` focuses the primary selection.
- `Shift`/`Ctrl` increase/decrease speed.

The previous active game camera is restored when editor camera control ends.
The Debug Camera is transient and is not saved.

## Extension rules

- Put per-frame/world tool logic in a module and windows in a panel.
- Identify registrations by stable owner/name, not vector index.
- Begin an editor transaction before ECS mutation.
- Mark editor helpers `InfoComponent::isTransient`.
- Use the main viewport/G-buffer context for overlays and picking.
- Register asset loaders through the loader registry.

See [editor extensions](editor_extensions.md).
