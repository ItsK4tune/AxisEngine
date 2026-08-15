# Hướng dẫn Quy chuẩn Comment & Ghi chú Mã nguồn

> [English](../../eng/guides/comment_policy.md) | [Bề mặt API Công khai](../core/api_surface.md) | [Mục lục Tài liệu](../INDEX.md)

---

## 1. Giới thiệu

AxisEngine duy trì các ghi chú comment mã nguồn C++ sạch sẻ, nhất quán trên toàn bộ các header công khai và mã nguồn triển khai nội bộ bằng cách sử dụng các tag Doxygen.

---

## 2. Cách dùng

1. **Ghi chú Tóm tắt Header**: Sử dụng `/// @brief` phía trên định nghĩa class, struct hoặc hàm.
2. **Ghi chú Tham số**: Ghi nhận tham số với `/// @param tên Mô tả`.
3. **Ghi chú Giá trị Trả về**: Ghi nhận giá trị trả về bằng `/// @return Mô tả`.

---

## 3. Ví dụ

### Ví dụ Comment Doxygen
```cpp
#include <axis_sdk.h>

/// @brief Đại diện cho script hành vi người chơi trong game.
class PlayerScript final : public Scriptable {
public:
    /// @brief Tick cập nhật logic script mỗi khung hình.
    /// @param dt Thời gian delta khung hình tính bằng giây.
    void OnUpdate(float dt) override {
        // Thực thi
    }
};
```

---

## 4. Tra cứu Param, Setting & API Reference

### Bảng Tra cứu Các Tag Doxygen

| Tên Tag | Mục đích | Ví dụ |
| :--- | :--- | :--- |
| `/// @brief` | Tóm tắt ngắn gọn về biểu tượng | `/// @brief Computes spatial culling.` |
| `/// @param` | Tài liệu hóa tham số truyền vào | `/// @param dt Frame delta time in seconds.` |
| `/// @return` | Mô tả giá trị trả về | `/// @return True if initialized successfully.` |
