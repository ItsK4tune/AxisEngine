# Hướng dẫn Quản lý Managers & Services

> [English](../../eng/core/managers.md) | [Tổng quan Kiến trúc](architecture.md) | [Mục lục Tài liệu](../INDEX.md)

---

## 1. Giới thiệu

AxisEngine quản lý các hệ thống con bằng cách kết hợp giữa các Singleton Manager toàn cục và các Service được phân tách truy cập qua `ServiceLocator`. Các manager này điều khiển cache tài nguyên, phát sự kiện pub-sub, nhận đầu vào phần cứng, nạp scene và thực thi tác vụ đa luồng.

---

## 2. Cách dùng

1. **Truy cập Singleton**: Sử dụng hàm truy cập tĩnh `Get()` (ví dụ: `ResourceManager::Get()`, `InputManager::Get()`).
2. **Truy vấn Service**: Sử dụng `ServiceLocator::Get<IServiceType>()` cho các giao diện (`IAudioService`, `IPhysicsWorld`).
3. **Phát Sự kiện**: Đăng ký hàm xử lý qua `EventManager::Get().Subscribe<T>()` và phát qua `Publish()`.
4. **Tác vụ Đa luồng**: Phân phối công việc đa luồng qua `JobSystem::Get().DispatchParallel()`.

---

## 3. Ví dụ

### 1. Ví dụ `ResourceManager` & `InputManager`
```cpp
#include <axis_sdk.h>

void UpdateGameLogic(float dt) {
    auto& input = InputManager::Get();
    auto& resources = ResourceManager::Get();

    if (input.IsKeyPressed(KeyCode::L)) {
        auto tex = resources.LoadTexture("textures/hero.png");
        AXIS_LOG_INFO("Texture da duoc nap!");
    }
}
```

### 2. Ví dụ `EventManager` Pub-Sub
```cpp
#include <axis_sdk.h>

struct GameOverEvent { int score; };

void SetupEvents() {
    EventManager::Get().Subscribe<GameOverEvent>([](const GameOverEvent& e) {
        AXIS_LOG_INFO("Game Over! Diem: " + std::to_string(e.score));
    });

    EventManager::Get().Publish(GameOverEvent{ 1250 });
}
```

---

## 4. Tra cứu Param, Setting & API Reference

### Bảng Tra cứu API Managers & Services Cốt lõi

| Tên Manager | Mẫu Truy cập | Các Phương thức / API Chính | Trách nhiệm & Vai trò Chính |
| :--- | :--- | :--- | :--- |
| `ResourceManager` | `ResourceManager::Get()` | `LoadTexture()`, `LoadModel()`, `UnloadUnusedResources()` | Cache & tự động khử trùng lặp tài nguyên |
| `EventManager` | `EventManager::Get()` | `Subscribe<T>()`, `Publish<T>()` | Phát sự kiện pub-sub phân tách |
| `InputManager` | `InputManager::Get()` | `IsKeyDown()`, `IsKeyPressed()`, `GetMouseDelta()` | Nhận trạng thái phím, chuột & gamepad |
| `SceneManager` | `SceneManager::Get()` | `LoadScene()`, `LoadBinaryScene()`, `GetActiveScene()` | Nạp scene `.axs` & `.axsb` |
| `JobSystem` | `JobSystem::Get()` | `DispatchParallel()`, `WaitAll()` | Phân phối công việc luồng worker đa luồng |
