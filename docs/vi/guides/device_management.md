# Hướng dẫn Quản lý Thiết bị & Màn hình Display

> [English](../../eng/guides/device_management.md) | [Tra cứu Cấu hình Configuration](configuration.md) | [Mục lục Tài liệu](../INDEX.md)

---

## 1. Giới thiệu

AxisEngine cung cấp khả năng quản lý màn hình đa hiển thị và thiết bị đầu vào qua GLFW. Subsystem điều khiển các chế độ hiển thị (Fullscreen, Windowed, Borderless), lựa chọn màn hình chỉ định, điều tiết VSync và cắm rút nóng gamepad.

---

## 2. Cách dùng

1. **Truy cập Đối tượng Cửa sổ**: Lấy đối tượng cửa sổ qua `Application::Get().GetWindow()`.
2. **Truy vấn Màn hình**: Gọi `window.GetMonitorCount()`.
3. **Chuyển Chế độ Hiển thị**: Gọi `window.SetDisplayMode(mode, width, height, monitorIndex)`.

---

## 3. Ví dụ

### Ví dụ Chuyển Độ phân giải Màn hình
```cpp
#include <axis_sdk.h>

void SetSecondaryDisplay() {
    auto& window = Application::Get().GetWindow();
    if (window.GetMonitorCount() > 1) {
        window.SetDisplayMode(WindowMode::FULLSCREEN, 1920, 1080, 1);
    }
}
```

---

## 4. Tra cứu Param, Setting & API Reference

### Bảng Tra cứu Thiết lập Màn hình Display

| Tham số / Thuộc tính | Kiểu dữ liệu | Mặc định | Mô tả |
| :--- | :--- | :--- | :--- |
| `MONITOR` | `int` | `0` | Chỉ số màn hình mục tiêu (`0` cho màn hình chính) |
| `WINDOW_MODE` | `Enum` | `BORDERLESS_FULLSCREEN` | Chế độ hiển thị (`WINDOWED`, `FULLSCREEN`, `BORDERLESS`) |
| `REFRESH_RATE` | `int` | `60` | Tần số quét màn hình tính bằng Hz |
| `RENDER_SCALE` | `float` | `1.0` | Hệ số độ phân giải dựng hình nội bộ (`0.5` đến `2.0`) |
