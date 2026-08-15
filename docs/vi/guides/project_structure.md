# Hướng dẫn Cấu trúc Dự án (Project Structure Guide)

> [English](../../eng/guides/project_structure.md) | [Hướng dẫn Build](build_guide.md) | [Mục lục Tài liệu](../INDEX.md)

---

## 1. Giới thiệu

AxisEngine tổ chức mã nguồn thành các thư mục cấp cao riêng biệt (`include/`, `src/`, `sample/`, `compiler/`, `tests/`, `docs/`, `cmake/`) để tách biệt các header công khai khỏi mã nguồn triển khai nội bộ, bộ kiểm thử, asset và công cụ.

---

## 2. Cách dùng

1. **Phát triển Game**: Include header từ `include/`, viết logic ứng dụng trong thư mục dự án của bạn và nạp asset tương đối theo file thực thi hoặc sử dụng `asset://`.
2. **Truy cập Asset Tích hợp Engine**: Sử dụng giao thức URI `asset://` để giải quyết shader, texture và mesh mặc định của engine.
3. **Biên dịch Scene**: Sử dụng công cụ biên dịch nằm trong `compiler/` (`axis_compile`) để chuyển đổi file `.axs` sang `.axsb`.

---

## 3. Ví dụ

### Ví dụ Giải quyết Đường dẫn
```cpp
#include <axis_sdk.h>

void LoadAssetsDemo() {
    auto& resources = ResourceManager::Get();

    // 1. Giao thức URI asset tích hợp
    auto checkerTex = resources.LoadTexture("asset://textures/default_checker.png");

    // 2. Đường dẫn tương đối dự án
    auto modelMesh = resources.LoadModel("models/hero.obj");
}
```

---

## 4. Tra cứu Param, Setting & API Reference

### Bảng Tra cứu Cấu trúc Thư mục

| Đường dẫn Thư mục | Mục đích & Nội dung | Sản phẩm Đầu ra |
| :--- | :--- | :--- |
| `include/` | Header SDK công khai (`axis_sdk.h`, `axis_plugin.h`, `axis_advanced.h`) | Thư mục header SDK đã cài đặt |
| `src/` | Mã nguồn engine và thư viện ImGui `Axis::Editor` | `axis_engine.lib`, `axis_editor.lib` |
| `sample/` | 33 scenario demo, scene thử nghiệm, texture và shader | `axis_samples.exe` |
| `compiler/` | Công cụ biên dịch scene nhị phân `axis_compile` | `axis_compile.exe` |
| `tests/` | Các bộ kiểm thử unit test, integration test | `axis_test.exe` |
| `docs/` | Tài liệu hướng dẫn đa ngôn ngữ và sổ tay sử dụng | Tài liệu Markdown |
| `cmake/` | Cấu hình xuất package, find modules, triplets và vcpkg toolchain | `AxisEngineConfig.cmake` |

### Bảng Tra cứu Giao thức Đường dẫn

| Giao thức / Sơ đồ | Đường dẫn Giải quyết | Mục đích |
| :--- | :--- | :--- |
| `asset://` | `share/AxisEngine/assets/` | Truy cập texture, shader, mesh mặc định tích hợp sẵn |
| Đường dẫn Tương đối | Thư mục Chạy File Thực thi | Truy cập model, texture, file âm thanh của dự án |
| Đường dẫn Tuyệt đối | Đường dẫn Trực tiếp Hệ thống | Công cụ phát triển và thao tác import trong editor |
