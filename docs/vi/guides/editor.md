# Sổ tay Hướng dẫn Sử dụng Editor (`Axis::Editor`)

> [English](../../eng/guides/editor.md) | [Mở rộng Editor](editor_extensions.md) | [Hệ thống Debug](debug_system.md) | [Mục lục Tài liệu](../INDEX.md)

---

## 1. Giới thiệu

`Axis::Editor` là một **Visual Editor tích hợp dựa trên ImGui** được biên dịch khi `ENABLE_EDITOR=ON`. Editor cung cấp khả năng duyệt cây hierarchy, inspector component, gizmo viewport, duyệt asset, thông số profiler và nút điều khiển mô phỏng.

---

## 2. Cách dùng

1. **Bật Build Editor**: Cấu hình CMake với `ENABLE_EDITOR=ON`.
2. **Đăng ký Editor System**: Đăng ký `EditorSystem` vào `SystemRegistry` của engine.
3. **Kiểm tra & Chỉnh sửa**: Click vào entity trong Hierarchy để chỉnh sửa component trong Inspector; dùng gizmo 3D viewport để di chuyển/xoay/co giãn.
4. **Play / Pause / Stop**: Sử dụng thanh công cụ phía trên để chạy thử mô phỏng game.

---

## 3. Ví dụ

### Ví dụ Khởi chạy Editor Host
```cpp
#include <axis_sdk.h>

#if defined(AXIS_ENABLE_EDITOR)
#include <axis_editor.h>
#endif

int main() {
    auto app = std::make_shared<Application>();
    AppConfig config;
    config.title = "AxisEngine Editor Host";

    if (!app->Initialize(config)) return 1;

    #if defined(AXIS_ENABLE_EDITOR)
    app->GetSystemRegistry().RegisterSystem<EditorSystem>();
    #endif

    app->Run();
    return 0;
}
```

---

## 4. Tra cứu Param, Setting & API Reference

### Bảng Tra cứu Các Bảng điều khiển Editor

| Tên Bảng điều khiển | Mục đích & Khả năng |
| :--- | :--- |
| **Hierarchy Panel** | Duyệt cây entity scene, tạo entity, đổi cha, xóa entity |
| **Inspector Panel** | Chỉnh sửa thuộc tính component thời gian thực, gắn script |
| **Project Browser** | Cây hệ thống file workspace, xem trước asset, nạp scene |
| **Viewport Window** | Màn hình dựng hình 3D kèm gizmo Di chuyển, Xoay và Co giãn |
| **Tools Panel** | Bật tắt đồ họa trực tiếp (bóng đổ, post-processing, khung dây vật lý) |
| **Profiler Panel** | Phân rã thời gian khung hình CPU/GPU và số lượng draw call |
| **Console Panel** | Cửa sổ hiển thị log đầu ra hỗ trợ lọc |
| **Play Controls** | Bộ chuyển đổi trạng thái mô phỏng engine (**Play**, **Pause**, **Stop**) |
