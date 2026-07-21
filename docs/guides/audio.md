# Audio Guide

Axis exposes playback through `IAudioEngine`; the concrete backend can be irrKlang, FMOD, or the null backend selected by the build and configuration.

---

## 1. Audio Components

### AudioSourceComponent
Defines a sound emitter attached to an entity.
- **Settings**:
  - `Path`: Relative file path to the audio asset.
  - `Volume`: Playback volume (0.0 to 1.0).
  - `Loop`: Boolean toggle for repeating audio.
  - `Is3D`: Enables distance-based attenuation and panning.
  - `MinDistance`: The radius within which the sound remains at maximum volume.
  - `PlayOnAwake`: Automatically start playback when the scene loads.

---

## 2. Audio service API
`AudioService` owns the selected playback backend and provides direct playback.

### Accessing the Manager
```cpp
if (auto* audio = Resolve<AudioService>()) {
    audio->Play2D("audio/music.ogg", true);
}
```

### Common Operations
- **Play2D**: Use for background music or UI sound effects (`audio->Play2D("path.wav", true)`).
- **Play3D**: Trigger a one-off sound at a world position.
- **Global Control**: `StopAll()` or `SetGlobalVolume(float)` for master volume control.
- **Listener**: The system automatically updates the listener's "ears" to match the active camera position and orientation.

---

## 3. Audio System Lifecycle
The `AudioSystem` runs every frame to ensure spatial consistency:

1.  **3D Tracking**: Updates the position of all active `AudioSourceComponent` instances to match their entity's `TransformComponent`.
2.  **Listener Sync**: Synchronizes the 3D listener state with the current camera's view matrix.
3.  **Automatic Playback**: Starts sources that are flagged with `PlayOnAwake` or set to play via logic.
4.  **Resource Cleanup**: Safely releases backend sound instances when entities are destroyed.

---

## See Also
- [Physics Guide](physics.md)
- [Graphics Guide](graphics.md)
- [Scriptable API](../scripting/scriptable_api.md)
