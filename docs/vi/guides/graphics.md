# Hướng dẫn Hệ thống Đồ họa & Renderer

> [English](../../eng/guides/graphics.md) | [Tra cứu Component Reference](components_reference.md) | [Tra cứu Cấu hình Configuration](configuration.md) | [Mục lục Tài liệu](../INDEX.md)

---

## 1. Giới thiệu

AxisEngine triển khai một engine đồ họa **OpenGL 4.6 Dựng hình Dựa trên Vật lý (PBR)**. Hỗ trợ cả hai pipeline Forward và Deferred, renderer xử lý vật liệu PBR metallic-roughness, bản đồ bóng đổ động, post-processing (HDR, Bloom, TAA), environment reflection probes, hệ thống hạt và texture video.

---

## 2. Cách dùng

1. **Thiết lập Mesh & Material PBR**: Thêm `MeshRendererComponent` vào entity và chỉ định các thuộc tính texture albedo/normal, roughness và metallic.
2. **Thêm Ánh sáng Scene**: Thêm entity chứa `DirectionalLightComponent`, `PointLightComponent`, hoặc `SpotLightComponent`.
3. **Cấu hình Post-Processing**: Bật `HDR_ENABLED`, `BLOOM_ENABLED` và `ANTIALIASING` trong `AppConfig` hoặc file `axis_config.axs`.

---

## 3. Ví dụ

### 1. Ví dụ Cấu hình Material PBR
```cpp
#include <axis_sdk.h>

void CreatePBRSphere(Scene& scene) {
    auto entity = scene.CreateEntity("PBR Sphere");

    auto& transform = entity.AddComponent<TransformComponent>();
    transform.SetPosition(Vector3(0.0f, 1.0f, 0.0f));

    auto& renderer = entity.AddComponent<MeshRendererComponent>();
    renderer.modelPath = "models/sphere.obj";
    renderer.albedoTexture = "textures/metal_d.png";
    renderer.normalTexture = "textures/metal_n.png";
    renderer.roughness = 0.2f;
    renderer.metallic = 0.9f;
}
```

### 2. Ví dụ Tạo Ánh sáng Mặt trời Directional
```cpp
#include <axis_sdk.h>

void SetupSunlight(Scene& scene) {
    auto sun = scene.CreateEntity("Sun Light");

    auto& transform = sun.AddComponent<TransformComponent>();
    transform.SetRotation(Vector3(45.0f, -30.0f, 0.0f));

    auto& light = sun.AddComponent<DirectionalLightComponent>();
    light.color = Vector3(1.0f, 0.95f, 0.85f);
    light.intensity = 3.0f;
    light.castShadows = true;
}
```

---

## 4. Tra cứu Param, Setting & API Reference

### Bảng Tra cứu Tính năng & Cấu hình Đồ họa

| Tính năng / Khóa Cấu hình | Kiểu dữ liệu | Mặc định | Mô tả |
| :--- | :--- | :--- | :--- |
| `GRAPHICS_API` | `Enum` | `OPENGL` | Provider API dựng hình cốt lõi |
| `ANTIALIASING` | `Enum` | `TAA` | Thuật toán khử răng cưa (`NONE`, `FXAA`, `TAA`) |
| `HDR_ENABLED` | `bool` | `true` | Bật buffer dựng hình dải động cao HDR |
| `TONEMAPPING` | `Enum` | `ACES` | Lựa chọn đường cong tonemapping (`NONE`, `REINHARD`, `ACES`) |
| `BLOOM_ENABLED` | `bool` | `true` | Bật hiệu ứng phát sáng Bloom post-processing |
| `SHADOWS` | `bool` | `true` | Công tắc chính bật tắt đổ bóng |
| `SHADOW_RESOLUTION` | `int` | `2048` | Độ phân giải bản đồ độ sâu (`512` đến `4096`) |
| `SPATIAL_CULLING` | `Enum` | `AUTO` | Spatial culling backend (`AUTO`, `LINEAR`, `OCTREE`) |
