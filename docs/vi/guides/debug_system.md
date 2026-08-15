# Hướng dẫn Công cụ Debug, Logging & Chẩn đoán Assertions

> [English](../../eng/guides/debug_system.md) | [Sổ tay Editor](editor.md) | [Tra cứu Cấu hình Configuration](configuration.md) | [Mục lục Tài liệu](../INDEX.md)

---

## 1. Giới thiệu

AxisEngine cung cấp một bộ công cụ debug toàn diện bao gồm hệ thống ghi log, chẩn đoán assertion thời gian chạy, và các bảng giao diện Debug GUI ImGui. Subsystem debug cho phép nhà phát triển phát hiện vi phạm hợp đồng qua assertions (`AXIS_ASSERT`), ghi vết log theo mức độ nghiêm trọng (`AXIS_LOG_*`), kiểm tra trạng thái dựng hình trực tiếp (`ToolsPanel`), theo dõi thông số hiệu năng (`ProfilerPanel`) và trực quan hóa khung va chạm vật lý.

---

## 2. Cách dùng

1. **Ghi Log**: Sử dụng `AXIS_LOG_INFO("thông điệp")`, `AXIS_LOG_WARN("thông điệp")`, `AXIS_LOG_ERROR("thông điệp")`, `AXIS_LOG_DEBUG("thông điệp")`, hoặc `AXIS_LOG_VERBOSE("thông điệp")`.
2. **Assertions Runtime**: Chèn `AXIS_ASSERT(ptr != nullptr, "Con trỏ không được phép null!")` để bắt buộc kiểm tra điều kiện hợp đồng.
3. **Phím tắt Điều khiển**: Sử dụng F1-F12 để bật tắt tên entity, gizmo, nguồn phát ánh sáng, bóng đổ, post-processing, khung dây vật lý, camera debug, tạm dừng và tốc độ thời gian.
4. **Cửa sổ Debug GUI**: Mở `ToolsPanel` để bật tắt công tắc dựng hình, `ProfilerPanel` để xem FPS/draw calls và `ConsolePanel` để xem log đầu ra.

---

## 3. Ví dụ

### 1. Ví dụ Ghi Log `AXIS_LOG_*` & Chẩn đoán `AXIS_ASSERT`
```cpp
#include <axis_sdk.h>

void ProcessEntityDamage(Entity player, int damageAmount) {
    // 1. Kiểm tra điều kiện hợp đồng bắt buộc với AXIS_ASSERT
    AXIS_ASSERT(damageAmount >= 0, "Luong sat thuong khong duoc phep am!");

    // 2. Ghi log có cấu trúc theo mức độ nghiêm trọng
    AXIS_LOG_DEBUG("ProcessEntityDamage duoc goi cho entity ID: " + std::to_string(static_cast<uint32_t>(player)));

    if (damageAmount > 50) {
        AXIS_LOG_WARN("Nhan sat thuong nang: " + std::to_string(damageAmount) + " HP!");
    } else {
        AXIS_LOG_INFO("Nguoi choi nhan " + std::to_string(damageAmount) + " sat thuong.");
    }
}
```

### 2. Ví dụ Đánh dấu Profiling Scope
```cpp
#include <axis_sdk.h>

void ProfileHeavyTask() {
    AXIS_PROFILE_SCOPE("ProfileHeavyTask");

    for (int i = 0; i < 1000; ++i) {
        // Tác vụ tính toán
    }
}
```

---

## 4. Tra cứu Param, Setting & API Reference

### Bảng Tra cứu Các Macro Ghi Log (`AXIS_LOG_*`)

| Tên Macro | Mức độ Nghiêm trọng | Mục đích Xuất Đầu ra |
| :--- | :--- | :--- |
| `AXIS_LOG_VERBOSE(msg)` | `VERBOSE` | Các thông điệp ghi vết tần suất cao và chẩn đoán chi tiết |
| `AXIS_LOG_DEBUG(msg)` | `DEBUG` | Thông tin debug dành cho lập trình viên trong quá trình phát triển |
| `AXIS_LOG_INFO(msg)` | `INFO` | Các sự kiện mốc hoạt động chung và trạng thái thông tin |
| `AXIS_LOG_WARN(msg)` | `WARNING` | Cảnh báo không gây chết ứng dụng, thiếu asset dự phòng |
| `AXIS_LOG_ERROR(msg)` | `ERROR` | Lỗi nghiêm trọng, thiếu tài nguyên bắt buộc, lỗi phần cứng |

### Bảng Tra cứu Assertions & Macro Chẩn đoán

| Macro Chẩn đoán | Hành vi Hợp đồng |
| :--- | :--- |
| `AXIS_ASSERT(condition, message)` | Kiểm tra biểu thức điều kiện; nếu sai sẽ ghi vết log kèm file/dòng và kích hoạt breakpoint trong bản build debug |

### Bảng Tra cứu Phím tắt Debug

| Phím tắt | Tên Chức năng | Mô tả |
| :--- | :--- | :--- |
| **F1** | Tên Entity | Bật tắt hiển thị nhãn tên entity 3D trên viewport |
| **F2** | Gizmos | Bật tắt gizmo transform di chuyển/xoay/tỷ lệ |
| **F3** | Gizmo Ánh sáng | Bật tắt biểu tượng nguồn phát ánh sáng |
| **F4** | Skybox | Bật tắt dựng hình skybox |
| **F5** | Đổ bóng (Shadow) | Bật tắt tính toán bản đồ bóng đổ động |
| **F6** | Post Process | Bật tắt HDR, Bloom, FXAA/TAA |
| **F7** | Physics Debug | Bật tắt khung dây va chạm Bullet 3D |
| **F8** | Audio Debug | Bật tắt cầu bán kính phát âm thanh 3D |
| **F9** | Particle Debug | Bật tắt hộp bao quanh nguồn phát hạt |
| **F10** | Con trỏ Editor | Bật tắt con trỏ giữa game viewport và GUI |
| **Shift+F10** | Debug Camera | Bật tắt camera debug bay tự do |
| **F11** | Tạm dừng Game | Tạm dừng hoặc tiếp tục vòng lặp engine tick |
| **F12** | Tỷ lệ Thời gian | Xoay vòng tốc độ mô phỏng (0.1x, 1.0x, 2.0x) |
