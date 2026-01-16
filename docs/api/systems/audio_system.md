# AudioSystem

**Include:** `<engine/ecs/system.h>`

Updates `AudioSourceComponent` states based on logic (e.g., 3D positioning).

## Responsibilities
*   **3D Positioning**: Updates position of 3D sounds to match Entity Transform.
*   **State Management**: Starts playing sounds if `playOnAwake` or `shouldPlay` flag is set.
*   **Cleanup**: Stops sounds if the entity is destroyed (handled via manual StopAll or component destructor logic).

## Public API
*   `void Update(Scene &scene, SoundManager& soundManager)`
*   `void StopAll(Scene &scene)`
