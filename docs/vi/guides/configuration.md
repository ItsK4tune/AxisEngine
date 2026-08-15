# Hướng dẫn Tra cứu Cấu hình Configuration

> [English](../../eng/guides/configuration.md) | [Hướng dẫn Định dạng Scene](scene_format.md) | [Mục lục Tài liệu](../INDEX.md)

---

## 1. Giới thiệu

AxisEngine sở hữu một hệ thống cấu hình tập trung định hướng dữ liệu. Giá trị mặc định của ứng dụng, các flag dựng hình đồ họa, độ chính xác vật lý, backend âm thanh và các thông số tối ưu hóa được định nghĩa trong `axis_config.axs` hoặc thay đổi động trong quá trình chạy.

---

## 2. Cách dùng

1. **Cấu hình Tĩnh (`.axs`)**: Định nghĩa khối `axis_config` trong file cấu hình dự án `.axs`.
2. **API Cấu hình Runtime**: Truy cập `Application::Get().GetConfig()` để thay đổi tham số bằng mã C++.
3. **Thông báo Hệ thống**: Phát sự kiện `ConfigChangedEvent` qua `EventManager` để áp dụng trực tiếp các thay đổi đồ họa, vật lý hoặc tối ưu hóa.

---

## 3. Ví dụ

### 1. Ví dụ File Cấu hình `.axs`
```yaml
axis_config:
    LOG_LEVEL: VERBOSE
    JOB_THREADS: -1
    TIME_SCALE: 1.0
    STRICT_ASSET_LOADING: 0

    WINDOW_WIDTH: 1920
    WINDOW_HEIGHT: 1080
    WINDOW_MODE: BORDERLESS_FULLSCREEN
    VSYNC: 1
    FPS: 120

    GRAPHICS_API: OPENGL
    ANTIALIASING: TAA
    HDR_ENABLED: 1
    TONEMAPPING: ACES
    BLOOM_ENABLED: 1

    SHADOWS: 1
    SHADOW_RESOLUTION: 2048
    SHADOW_SOFTNESS: 1

    PHYSICS_ENGINE: BULLET
    GRAVITY: 0.0 -9.81 0.0
    PHYSICS_MODE: BALANCED

    AUDIO_ENGINE: NULL
    VOLUME: 100
```

### 2. Ví dụ Điều chỉnh Cấu hình Động Runtime
```cpp
#include <axis_sdk.h>

void ApplyRuntimeGraphicsQuality(bool highQuality) {
    auto& config = Application::Get().GetConfig();
    config.graphics.shadowResolution = highQuality ? 4096 : 1024;
    config.graphics.antiAliasing = highQuality ? AntialiasingMode::TAA : AntialiasingMode::FXAA;

    EventManager::Get().Publish(ConfigChangedEvent::Graphics{});
}
```

---

## 4. Tra cứu Param, Setting & API Reference

### Bảng Tra cứu Cấu hình Engine & Toàn cục

| Khóa Cấu hình | Giá trị Hợp lệ / Khoảng | Mặc định | Mô tả |
| :--- | :--- | :--- | :--- |
| `LOG_LEVEL` | `NONE`, `MINIMAL`, `VERBOSE`, `DEBUG` | `VERBOSE` | Ngưỡng chi tiết ghi log ra console |
| `JOB_THREADS` | `-1` (Tự động), `1` đến `64` | `-1` | Số lượng luồng worker đa luồng |
| `TIME_SCALE` | `0.0` đến `10.0` | `1.0` | Hệ số nhân thời gian mô phỏng toàn cục |
| `STRICT_ASSET_LOADING` | `0` hoặc `1` | `0` | Tắt texture bàn cờ mặc định khi thiếu asset |

### Bảng Tra cứu Cấu hình Đồ họa & Hiển thị

| Khóa Cấu hình | Giá trị Hợp lệ / Khoảng | Mặc định | Mô tả |
| :--- | :--- | :--- | :--- |
| `WINDOW_MODE` | `WINDOWED`, `FULLSCREEN`, `BORDERLESS_FULLSCREEN` | `BORDERLESS_FULLSCREEN` | Chế độ hiển thị cửa sổ |
| `VSYNC` | `0` hoặc `1` | `1` | Bật/tắt đồng bộ hóa dọc VSync |
| `FPS` | `0` (Không giới hạn), `30`, `60`, `120`, `144` | `120` | Giới hạn khung hình mục tiêu |
| `ANTIALIASING` | `NONE`, `FXAA`, `TAA` | `TAA` | Thuật toán khử răng cưa |
| `TONEMAPPING` | `NONE`, `REINHARD`, `ACES` | `ACES` | Đường cong tonemapping HDR |
| `BLOOM_ENABLED` | `0` hoặc `1` | `1` | Bật/tắt hiệu ứng Bloom |
| `SPATIAL_CULLING` | `AUTO`, `LINEAR`, `OCTREE` | `AUTO` | Chiến lược culling không gian dựng hình |

### Bảng Tra cứu Đổ bóng & Vật lý

| Khóa Cấu hình | Giá trị Hợp lệ / Khoảng | Mặc định | Mô tả |
| :--- | :--- | :--- | :--- |
| `SHADOWS` | `0` hoặc `1` | `1` | Công tắc chính bật/tắt đổ bóng |
| `SHADOW_RESOLUTION` | `512`, `1024`, `2048`, `4096` | `2048` | Độ phân giải bản đồ độ sâu |
| `GRAVITY` | Vector 3D `X Y Z` | `0 -9.81 0` | Vector trọng lực thế giới |
| `PHYSICS_MODE` | `FAST` (30Hz), `BALANCED` (60Hz), `ACCURATE` (120Hz) | `BALANCED` | Cấu hình sẵn tần số mô phỏng |
