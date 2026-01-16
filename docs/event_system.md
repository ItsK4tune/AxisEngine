# Event System Guide

The Event System allows decoupled communication between different parts of the engine (e.g., Scripts, Systems) using a publish-subscribe pattern.

## 1. Core Concepts
-   **Channel = Event Type**: You don't create named channels (like "PlayerChannel"). Instead, the C++ struct *is* the channel.
-   **Lazy Registration**: You don't need to "register" an event type beforehand. The system creates the dispatcher the first time someone subscribes to or publishes that event.

## 2. Defining an Event
Events are simple C++ structs (PODs). They do not need to inherit from any base class.

```cpp
struct PlayerHitEvent {
    int damage;
    int playerId;
};
```

## 2. Publishing an Event
Anywhere in your code, you can publish an event using the `EventSystem` singleton.

```cpp
#include <engine/core/event_system.h>

void OnPlayerHit(int dmg) {
    EventSystem::Instance().Publish(PlayerHitEvent{ dmg, 1 });
}
```

## 3. Subscribing to an Event
There are two ways to subscribe: **Manual Management** and **Safe Destruction (RAII)**.

### Option A: Safe Destruction (Recommended)
Use `ScopedSubscriber<T>` to automatically unsubscribe when the object is destroyed. This prevents crashes from dangling pointers.

```cpp
class HealthSystem : public Scriptable {
    // 1. Declare the scoped subscriber as a member
    ScopedSubscriber<PlayerHitEvent> m_HitListener;

public:
    void OnStart() override {
        // 2. Subscribe and store the token
        int id = EventSystem::Instance().Subscribe<PlayerHitEvent>(
            [this](const PlayerHitEvent& e) { this->OnDamage(e); }
        );
        
        // 3. Hand over responsibility to the RAII wrapper
        m_HitListener.Reset(id);
    }

    // No need to manually Unsubscribe in OnDestroy!
    // m_HitListener destructor handles it automatically.

    void OnDamage(const PlayerHitEvent& e) {
        std::cout << "Took Damage: " << e.damage << std::endl;
    }
};
```

### Option B: Manual Management
If you need manual control, store the `int listenerId` and call `Unsubscribe` yourself.

```cpp
class ManualSystem {
    int m_Id = -1;

public:
    void Init() {
        m_Id = EventSystem::Instance().Subscribe<PlayerHitEvent>(...);
    }

    ~ManualSystem() {
        if (m_Id != -1) {
            EventSystem::Instance().Unsubscribe<PlayerHitEvent>(m_Id);
        }
    }
};
```

## Key Points
-   **Synchronous**: Events are dispatched immediately when `Publish` is called.
-   **Type-Safe**: The system uses templates to ensure you get the exact event type you subscribed to.
-   **ScopedSubscriber**: Always prefer using `ScopedSubscriber` for member callbacks to ensure safe cleanup.
