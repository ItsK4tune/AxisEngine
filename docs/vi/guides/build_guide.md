# Hướng dẫn Build & Cấu hình CMake

> [English](../../eng/guides/build_guide.md) | [Tra cứu Cấu hình Configuration](configuration.md) | [Mục lục Tài liệu](../INDEX.md)

---

## 1. Giới thiệu

AxisEngine sử dụng **CMake 3.20+** làm hệ thống sinh dự án đa nền tảng. Hệ thống build biên dịch thư viện tĩnh `Axis::Engine`, thư viện editor trực quan tùy chọn `Axis::Editor`, các bộ kiểm thử tự động và công cụ biên dịch scene.

---

## 2. Cách dùng

1. **Chọn CMake Preset**: Lựa chọn preset phù hợp trong `CMakePresets.json` (`windows-msvc`, `windows-msvc-editor`, `linux-ninja`, `linux-ninja-editor`).
2. **Cấu hình Engine**: Chạy `cmake --preset <tên_preset>` từ gốc repository.
3. **Biên dịch Target**: Chạy `cmake --build build --config Release --parallel`.
4. **Chạy Mẫu / Test**: Chạy `axis_samples.exe` hoặc thực thi `ctest --test-dir build`.

---

## 3. Ví dụ

### 1. Ví dụ Build Editor & Samples qua CLI
```powershell
# Cấu hình với preset MSVC Editor
cmake --preset windows-msvc-editor

# Build các file binary Release song song
cmake --build build --config Release --parallel

# Chạy ứng dụng mẫu
.\build\bin\Release\axis_samples.exe
```

### 2. Ví dụ Sử dụng Package AxisEngine (`CMakeLists.txt`)
```cmake
cmake_minimum_required(VERSION 3.20)
project(MyGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
find_package(AxisEngine REQUIRED)

add_executable(MyGame src/main.cpp)
target_link_libraries(MyGame PRIVATE Axis::Engine)
```

---

## 4. Tra cứu Param, Setting & API Reference

### Bảng Tra cứu Cấu hình CMake Presets

| Tên Preset | Nền tảng | Trạng thái Editor | Trình biên dịch / Generator |
| :--- | :--- | :--- | :--- |
| `windows-msvc` | Windows | `OFF` | MSVC 2022 Thư viện Engine Tĩnh |
| `windows-msvc-editor` | Windows | `ON` | MSVC 2022 + Editor ImGui + `axis_samples` |
| `linux-ninja` | Linux | `OFF` | GCC / Clang + Ninja Generator |
| `linux-ninja-editor` | Linux | `ON` | GCC / Clang + Ninja + Editor + Samples |

### Bảng Tra cứu Tùy chọn Cấu hình CMake Reference

| Khóa Tùy chọn | Mặc định | Giá trị | Mô tả |
| :--- | :--- | :--- | :--- |
| `ENABLE_EDITOR` | `OFF` | `ON` / `OFF` | Biên dịch thư viện editor ImGui (`Axis::Editor`) |
| `BUILD_SAMPLES` | `ON` | `ON` / `OFF` | Biên dịch file thực thi `axis_samples` |
| `ENABLE_TESTS` | `OFF` | `ON` / `OFF` | Biên dịch bộ unit/integration test (`axis_test`) |
| `ENABLE_LTO` | `ON` | `ON` / `OFF` | Bật Tối ưu hóa lúc liên kết (IPO) |
| `ENABLE_PCH` | `ON` | `ON` / `OFF` | Bật Precompiled Header |
| `AXIS_GRAPHICS_BACKEND` | `OpenGL`| `OpenGL` | Provider dựng hình đồ họa |
| `AXIS_PHYSICS_BACKEND` | `Bullet` | `Bullet` | Provider mô phỏng vật lý |
| `AXIS_AUDIO_BACKEND` | `Null` | `Null`, `FMOD`, `IrrKlang` | Provider phát âm thanh |
