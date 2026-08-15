# Hướng dẫn Hệ thống Giao diện Người dùng (UI System)

> [English](../../eng/guides/ui.md) | [Tra cứu Component Reference](components_reference.md) | [Tra cứu Cấu hình Configuration](configuration.md) | [Mục lục Tài liệu](../INDEX.md)

---

## 1. Giới thiệu

AxisEngine sở hữu một **Hệ thống UI dạng Canvas** tăng tốc phần cứng cho phép tạo giao diện 2D HUD, menu tương tác, thanh máu và văn bản lớp phủ. Bố cục dựa trên phép toán điểm neo `RectTransform` hỗ trợ co giãn độc lập với độ phân giải.

---

## 2. Cách dùng

1. **Tạo Entity UI**: Tạo entity Canvas gốc và các entity phần tử UI con.
2. **Cấu hình Bố cục**: Thêm `RectTransformComponent` để thiết lập điểm neo (`anchorMin`, `anchorMax`), `sizeDelta` và `pivot`.
3. **Thêm Component Trực quan**: Gắn `UIImageComponent` cho texture/panel và `UITextComponent` cho nhãn văn bản.
4. **Xử lý Sự kiện Click**: Gắn `UIButtonComponent` và thiết lập callback `onClick`.

---

## 3. Ví dụ

### Ví dụ Tạo Button UI Tương tác
```cpp
#include <axis_sdk.h>

void CreateUIButton(Scene& scene) {
    auto canvas = scene.CreateEntity("Canvas");
    auto button = scene.CreateEntity("Play Button");

    auto& rect = button.AddComponent<RectTransformComponent>();
    rect.anchorMin = Vector2(0.5f, 0.5f);
    rect.anchorMax = Vector2(0.5f, 0.5f);
    rect.sizeDelta = Vector2(200.0f, 50.0f);

    auto& image = button.AddComponent<UIImageComponent>();
    image.color = Vector4(0.1f, 0.5f, 0.9f, 1.0f);

    auto& text = button.AddComponent<UITextComponent>();
    text.text = "CHOI NGAY";

    auto& btn = button.AddComponent<UIButtonComponent>();
    btn.onClick = []() {
        AXIS_LOG_INFO("Button Choi Ngay da duoc click!");
    };
}
```

---

## 4. Tra cứu Param, Setting & API Reference

### Bảng Tra cứu UI Components

| Tên Component | Thuộc tính Chính | Mục đích |
| :--- | :--- | :--- |
| `RectTransformComponent` | `anchorMin`, `anchorMax`, `anchoredPosition`, `sizeDelta`, `pivot` | Phạm vi màn hình 2D và phép toán bố cục anchor |
| `UIImageComponent` | `texturePath`, `color` | Dựng hình sprite texture hoặc panel màu đơn sắc |
| `UITextComponent` | `text`, `fontSize`, `color` | Dựng hình chuỗi văn bản phông chữ TrueType |
| `UIButtonComponent` | `onClick` (`std::function<void()>`) | Xử lý callback tương tác rê chuột và click chuột |
