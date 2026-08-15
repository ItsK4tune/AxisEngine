# Hướng dẫn Bề mặt API Công khai (Public API Surface)

> [English](../../eng/core/api_surface.md) | [Tổng quan Kiến trúc](architecture.md) | [Mục lục Tài liệu](../INDEX.md)

---

## 1. Giới thiệu

AxisEngine tổ chức các API C++ công khai thành bốn tầng header (`axis_sdk.h`, `axis_plugin.h`, `axis_advanced.h`, `axis_all.h`). Việc phân tầng này tách biệt ứng dụng bên ngoài khỏi chi tiết triển khai nội bộ đằng sau mà vẫn đảm bảo tốc độ biên dịch và sự ổn định ABI.

---

## 2. Cách dùng

Lựa chọn header phù hợp tùy thuộc vào vai trò ứng dụng của bạn:

1. **Lập trình Game & Scripting**: `#include <axis_sdk.h>` (API cấp cao ổn định).
2. **Provider Tùy chỉnh & Mở rộng**: `#include <axis_plugin.h>` (Hợp đồng plugin cho audio/physics/renderer).
3. **Tích hợp Cấp thấp Engine**: `#include <axis_advanced.h>` (Truy cập trực tiếp con trỏ Bullet/OpenGL).
4. **Biên dịch Nội bộ Engine**: `#include <axis_all.h>` (Chỉ dành cho precompiled header nội bộ).

---

## 3. Ví dụ

### Ví dụ Tầng 1 (`<axis_sdk.h>`)
```cpp
#include <axis_sdk.h>

void CreatePlayer(Scene& scene) {
    auto player = scene.CreateEntity("Player");
    player.AddComponent<TransformComponent>(Vector3(0.0f, 1.0f, 0.0f));
    player.AddComponent<MeshRendererComponent>("models/character.obj");
}
```

### Ví dụ Tầng 2 (`<axis_plugin.h>`)
```cpp
#include <axis_plugin.h>

class CustomAudioProvider final : public IAudioService {
public:
    bool Initialize() override { return true; }
    void Shutdown() override {}
    void PlaySound(const std::string& path, float volume) override {}
    void SetMasterVolume(float volume) override {}
};
AXIS_EXPORT_PLUGIN_PROVIDER(IAudioService, CustomAudioProvider)
```

---

## 4. Tra cứu Param, Setting & API Reference

### Bảng Tra cứu Lựa chọn Header & Độ Ổn định

| File Header | Đối tượng Phù hợp | Mức độ Ổn định | Hướng dẫn Include |
| :--- | :--- | :--- | :--- |
| `<axis_sdk.h>` | Lập trình viên Game, Scripter | **Cao (Ổn định)** | Header chính cho game & script người dùng |
| `<axis_plugin.h>` | Tác giả Plugin, Provider | **Trung bình** | Dùng cho backend âm thanh/vật lý/mạng tùy chỉnh |
| `<axis_advanced.h>` | Người Tích hợp Cấp thấp | **Thấp (Nội bộ)** | Dùng khi cần lấy con trỏ OpenGL/Bullet gốc |
| `<axis_all.h>` | Chỉ Dùng khi Build Nội bộ | **Nội bộ** | Không include trong các ứng dụng bên ngoài |
