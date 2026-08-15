# Managers & Services Guide

> [Tiếng Việt](../../vi/core/managers.md) | [Architecture Overview](architecture.md) | [Documentation Index](../INDEX.md)

---

## 1. Introduction

AxisEngine manages core runtime subsystems using global singleton managers and decoupled services accessible through `ServiceLocator`. These managers control asset caching, event pub-sub dispatching, hardware input polling, scene loading, and multithreaded job execution.

---

## 2. How to Use

1. **Accessing Singletons**: Use static `Get()` accessors (e.g., `ResourceManager::Get()`, `InputManager::Get()`).
2. **Querying Services**: Use `ServiceLocator::Get<IServiceType>()` for decoupled interfaces (`IAudioService`, `IPhysicsWorld`).
3. **Event Dispatching**: Subscribe handlers via `EventManager::Get().Subscribe<T>()` and publish via `Publish()`.
4. **Asynchronous Jobs**: Dispatch multithreaded tasks using `JobSystem::Get().DispatchParallel()`.

---

## 3. Examples

### 1. `ResourceManager` & `InputManager` Example
```cpp
#include <axis_sdk.h>

void UpdateGameLogic(float dt) {
    auto& input = InputManager::Get();
    auto& resources = ResourceManager::Get();

    if (input.IsKeyPressed(KeyCode::L)) {
        auto tex = resources.LoadTexture("textures/hero.png");
        AXIS_LOG_INFO("Texture loaded!");
    }
}
```

### 2. `EventManager` Pub-Sub Example
```cpp
#include <axis_sdk.h>

struct GameOverEvent { int score; };

void SetupEvents() {
    EventManager::Get().Subscribe<GameOverEvent>([](const GameOverEvent& e) {
        AXIS_LOG_INFO("Game Over! Score: " + std::to_string(e.score));
    });

    EventManager::Get().Publish(GameOverEvent{ 1250 });
}
```

---

## 4. API & Configuration Reference

### Core Managers & Services API Reference

| Manager Name | Access Pattern | Key Methods / APIs | Primary Responsibilities |
| :--- | :--- | :--- | :--- |
| `ResourceManager` | `ResourceManager::Get()` | `LoadTexture()`, `LoadModel()`, `UnloadUnusedResources()` | Asset caching & deduplication |
| `EventManager` | `EventManager::Get()` | `Subscribe<T>()`, `Publish<T>()` | Decoupled pub-sub event dispatching |
| `InputManager` | `InputManager::Get()` | `IsKeyDown()`, `IsKeyPressed()`, `GetMouseDelta()` | Keyboard, mouse & gamepad polling |
| `SceneManager` | `SceneManager::Get()` | `LoadScene()`, `LoadBinaryScene()`, `GetActiveScene()` | `.axs` & `.axsb` scene loading |
| `JobSystem` | `JobSystem::Get()` | `DispatchParallel()`, `WaitAll()` | Multithreaded worker thread pool |
