# Các system lõi

> [English](../../eng/systems/core_systems.md)

## 1. Event System

Event dùng mô hình publish–subscribe đồng bộ và phân kênh bằng type C++.
Subscriber nên dùng RAII để tránh callback trỏ tới object đã hủy.

```cpp
struct PlayerHitEvent { int damage; };

EventManager::Instance().Publish(PlayerHitEvent{10});

ScopedSubscriber<PlayerHitEvent> listener;
int token = EventManager::Instance().Subscribe<PlayerHitEvent>(
    [this](const PlayerHitEvent& event) { OnHit(event); });
listener.Reset(token);
```

## 2. Video System

Video được decode nền bằng FFmpeg. `VideoSystem` cập nhật texture cho
`UIRendererComponent` cùng entity, đồng bộ timestamp, loop, seek, speed và
volume. Audio nhúng được decode sang stereo PCM rồi giao cho audio backend.

`VideoPlayerComponent` có `Path`, `IsPlaying`, `IsLooping`, `Speed`, `Volume`
và các thao tác `Play`, `Pause`, `Stop`, `Seek`.

## 3. Job System

Pool worker dùng cho asset async và công việc song song an toàn. Không capture
component reference hoặc service context có lifetime ngắn hơn job.

## 4. Scene Management

Engine hỗ trợ nhiều scene xếp chồng: world, UI overlay hoặc persistent scene.
Scene operation được queue FIFO; camera active và membership phải được quản lý
qua API scene thay vì sửa registry tùy ý.

## 5. Window và video mode

- `BORDERLESS_FULLSCREEN`: fullscreen không viền theo desktop.
- `FULLSCREEN`: fullscreen exclusive.
- `WINDOWED`: cửa sổ có title bar.

## Xem thêm

- [Graphics](../guides/graphics.md)
- [UI](../guides/ui.md)
- [Manager và service](../core/managers.md)
