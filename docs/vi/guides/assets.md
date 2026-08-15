# Hướng dẫn Quản lý Asset & Tài nguyên

> [English](../../eng/guides/assets.md) | [Định dạng Scene](scene_format.md) | [Quản lý Managers & Services](../core/managers.md) | [Mục lục Tài liệu](../INDEX.md)

---

## 1. Giới thiệu

AxisEngine quản lý các model 3D, texture, clip âm thanh và shader GLSL bằng cách sử dụng `ResourceManager` tập trung. Subsystem tài nguyên xử lý việc đếm tham chiếu, khử trùng lặp đường dẫn, nạp bất đồng bộ và dọn dẹp bộ nhớ.

---

## 2. Cách dùng

1. **Nạp Asset**: Gọi `ResourceManager::Get().LoadTexture("path.png")` hoặc `LoadModel("model.obj")`.
2. **Asset Tích hợp**: Truy cập asset mặc định của engine bằng sơ đồ URI `asset://` (`asset://textures/white.png`).
3. **Dọn dẹp Bộ nhớ**: Gọi `resources.UnloadUnusedResources()` để giải phóng các asset không còn tham chiếu khỏi RAM/GPU.

---

## 3. Ví dụ

### 1. Ví dụ Nạp Asset
```cpp
#include <axis_sdk.h>

void LoadAssets() {
    auto& resources = ResourceManager::Get();

    auto albedo = resources.LoadTexture("textures/brick_d.png");
    auto model = resources.LoadModel("models/house.obj");

    if (albedo && model) {
        AXIS_LOG_INFO("Tai nguyen da duoc cache thanh cong!");
    }
}
```

### 2. Ví dụ Dọn dẹp Bộ nhớ
```cpp
#include <axis_sdk.h>

void CleanupCache() {
    ResourceManager::Get().UnloadUnusedResources();
}
```

---

## 4. Tra cứu Param, Setting & API Reference

### Bảng Tra cứu Định dạng Asset & Trình Nạp

| Loại Asset | Định dạng Hỗ trợ | Lớp Nạp | Mẫu Khóa Cache |
| :--- | :--- | :--- | :--- |
| **Model 3D** | `.obj`, `.gltf`, `.glb` | `ModelLoader` | Đường dẫn tương đối file model |
| **Textures** | `.png`, `.tga`, `.jpg`, `.dds` | `TextureLoader` | Đường dẫn tương đối file texture |
| **Clip Âm thanh** | `.wav`, `.ogg`, `.mp3` | `AudioLoader` | Đường dẫn tương đối file âm thanh |
| **Shaders** | `.glsl`, `.vert`, `.frag` | `ShaderCompiler` | Đường dẫn file mã nguồn shader |
| **Scenes** | `.axs` (Văn bản), `.axsb` (Nhị phân) | `SceneManager` | Đường dẫn file scene |
