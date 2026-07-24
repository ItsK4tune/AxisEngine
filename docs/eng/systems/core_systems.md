# Core Systems

> [Tiếng Việt](../../vi/systems/core_systems.md)

This document covers high-level engine systems that provide essential services across the engine.

---

## 1. Event System
The Event System enables decoupled communication using a **Publish-Subscribe** pattern.

### Core Concepts
- **Type-Based**: The C++ struct *is* the event channel. No registration is required.
- **Synchronous**: Events are dispatched immediately upon publication.
- **Scoped Subscribers**: RAII-based safety to prevent dangling pointers.

### Usage
```cpp
// 1. Define an event
struct PlayerHitEvent { int damage; };

// 2. Publish
EventManager::Instance().Publish(PlayerHitEvent{ 10 });

// 3. Subscribe (Recommended: ScopedSubscriber)
class MyScript : public Scriptable {
    ScopedSubscriber<PlayerHitEvent> m_Listener;
    void OnCreate() override {
        int token = EventManager::Instance().Subscribe<PlayerHitEvent>(
            [this](const auto& e) { OnHit(e); }
        );
        m_Listener.Reset(token);
    }
};
```

---

## 2. Video System
Manages asynchronous video playback using **FFmpeg**.

### Responsibilities
- **Decoding**: Frame processing happens in the background.
- **Auto-Injection**: Automatically updates the texture of a `UIRendererComponent` on the same entity.
- **Sync**: Playback speed, end-of-stream behavior, and audio/video clock synchronization.
- **Embedded Audio**: FFmpeg decodes embedded audio to a cached stereo PCM WAV before handing it to the selected audio backend. Playback follows play, pause, stop, seek, loop, speed, and volume changes consistently across providers.

### VideoPlayerComponent
- **Settings**: `Path`, `IsPlaying`, `IsLooping`, `Speed`, `Volume` (`0.0` to `1.0`).
- **Playback Controls**: `Play()`, `Pause()`, `Stop()`, `Seek(double)`.

---

## 3. Job System
The Job System provides a pool of worker threads for parallelizing heavy tasks.

- **Asynchronous Assets**: Used internally by the `ResourceManager` for non-blocking loads.
- **Parallel Iteration**: Optimization for large component views (where thread-safe).

---

## 4. Scene Management
The engine supports a stacked multi-scene architecture.

- **Scene Overlays**: Multiple `.axs` files can be loaded simultaneously (e.g., a game world scene with a UI overlay scene).
- **Active Camera**: The engine automatically picks the camera from the primary scene but allows overlay scenes to render their own 2D/3D elements on top.

---

## 5. Window & Video Modes
The engine supports various display modes configured via the `WINDOW_MODE` flag:

- **BORDERLESS_FULLSCREEN**: Renders the engine at full desktop resolution without borders, allowing for seamless Alt-Tab behavior.
- **FULLSCREEN**: Exclusive fullscreen for maximum performance.
- **WINDOWED**: Standard windowed mode with title bar.

---

## See Also
- [Graphics Guide](../guides/graphics.md)
- [UI Guide](../guides/ui.md)
- [Managers](../core/managers.md)
