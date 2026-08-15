# Hướng dẫn Mở rộng Editor (Editor Extensions)

> [English](../../eng/guides/editor_extensions.md) | [Sổ tay Editor](editor.md) | [Mục lục Tài liệu](../INDEX.md)

---

## 1. Giới thiệu

AxisEngine cho phép các nhà phát triển viết các cửa sổ editor ImGui tùy chỉnh, wizard panels và inspector extensions. Các mở rộng tùy chỉnh kế thừa từ `IEditorPanel` và đăng ký với `EditorSystem` để vẽ trong phase render của editor.

---

## 2. Cách dùng

1. **Kế thừa `IEditorPanel`**: Kế thừa từ `IEditorPanel` và thực thi `void OnImGuiRender() override`.
2. **Thêm Widget ImGui**: Viết mã ImGui bên trong `OnImGuiRender()` (`ImGui::Begin`, `ImGui::Button`, v.v.).
3. **Đăng ký Panel**: Gọi `editorSystem.RegisterPanel<MyPanel>("Tiieu De Cua So")`.

---

## 3. Ví dụ

### Ví dụ Bảng điều khiển Editor ImGui Tùy chỉnh
```cpp
#include <axis_sdk.h>
#include <axis_editor.h>
#include <imgui.h>

class LevelGeneratorPanel final : public IEditorPanel {
private:
    int m_gridSize = 10;

public:
    void OnImGuiRender() override {
        ImGui::Begin("Trinh Tao Level");
        ImGui::SliderInt("Kich Thuoc Luoi", &m_gridSize, 5, 50);

        if (ImGui::Button("Tao Cac O Tile")) {
            AXIS_LOG_INFO("Dang tao cac o tile cho luoi level...");
        }
        ImGui::End();
    }
};

void RegisterExtension(EditorSystem& editor) {
    editor.RegisterPanel<LevelGeneratorPanel>("Trinh Tao Level");
}
```

---

## 4. Tra cứu Param, Setting & API Reference

### Bảng Tra cứu API Mở rộng Editor

| Giao diện / Lớp | Phương thức Chính | Mục đích |
| :--- | :--- | :--- |
| `IEditorPanel` | `virtual void OnImGuiRender() = 0` | Giao diện cơ sở cho cửa sổ editor tùy chỉnh |
| `EditorSystem` | `RegisterPanel<T>(title)` | Đăng ký instance panel vào main editor dockspace |
