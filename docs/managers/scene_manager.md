# SceneManager API

**Include:** `<engine/core/scene_manager.h>`

The `SceneManager` handles loading scenes from files and managing entity creation.

## Access
Usually used internally by `Application`, but accessible if needed.

## Methods

*   `void LoadScene(const std::string& filePath)`
    *   Parses a text-based `.scene` file and populates the registry (Additive).
*   `void UnloadScene(const std::string& filePath)`
    *   Unloads entities belonging to the specified scene file.
*   `void ChangeScene(const std::string& filePath)`
    *   **Clears ALL** existing scenes (entities & physics) and loads the new scene.
*   `void ClearAllScenes()`
    *   Removes all entities and clears physics world.
*   `void SaveScene(const std::string& filePath)`
    *   (Not completely implemented) Serializes the current registry to a file.

## Scene Format
See [Scene Format Guide](../../scene_format.md) for details on the `.scene` file syntax.
