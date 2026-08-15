# Mục lục Tài liệu AxisEngine

> [English](../eng/INDEX.md) | [Sổ tay Sử dụng](MANUAL.md) | [Trang chủ README](../README.vn.md)

Tài liệu hướng dẫn kỹ thuật, tra cứu API, hướng dẫn cấu hình và kiến trúc dành cho các nhà phát triển ứng dụng C++20 với AxisEngine.

---

## 1. Kiến trúc Cốt lõi & Nền tảng
- [Bắt đầu Nhanh](core/getting_started.md): Cài đặt, thiết lập dự án và tạo ứng dụng C++ tối giản.
- [Tổng quan Kiến trúc](core/architecture.md): EnTT ECS registry, luồng thực thi đa luồng, thiết kế state machine, Entity, Scene và API EntityBuilder.
- [Bề mặt API Công khai](core/api_surface.md): Phân tầng header SDK (`axis_sdk.h`, `axis_plugin.h`, `axis_advanced.h`).
- [Quản lý Managers & Services](core/managers.md): `ResourceManager`, `EventManager`, `InputManager`, `SceneManager` và `JobSystem`.

---

## 2. Build & Cấu hình Engine
- [Hướng dẫn Build](guides/build_guide.md): CMake presets, compiler flags, cấu hình MSVC/GCC và xuất đóng gói.
- [Tra cứu Cấu hình Configuration](guides/configuration.md): Thuộc tính cấu hình `.axs`, thiết lập đồ họa/vật lý và tối ưu runtime.
- [Cấu trúc Dự án](guides/project_structure.md): Tổ chức thư mục engine, mục tiêu CMake và asset người dùng.
- [Quy chuẩn Comment](guides/comment_policy.md): Quy tắc phong cách mã nguồn và chuẩn ghi chú comment nội bộ.

---

## 3. Hệ thống Scene & Component
- [Định dạng Scene (`.axs` / `.axsb`)](guides/scene_format.md): Tài liệu 5 định dạng schema `.axs` (`axis_scene`, `axis_input`, `axis_data`, `axis_localization`, `axis_config`) và bộ biên dịch nhị phân.
- [Tra cứu Component Reference](guides/components_reference.md): Tài liệu toàn bộ các component ECS có sẵn và API tạo bằng code C++ `EntityBuilder`.
- [Quản lý Asset & Tài nguyên](guides/assets.md): Tải texture/model, hot-reloading và nạp luồng tài nguyên.

---

## 4. Các Subsystem Runtime
- [Hệ thống Đồ họa & Renderer](guides/graphics.md): OpenGL 4.6 PBR renderer, chiếu sáng, shadow, post-processing, hạt và địa hình.
- [Mô phỏng Vật lý](guides/physics.md): Tích hợp Bullet 3D, rigidbodies, colliders, character controllers và raycasting.
- [Phát Âm thanh](guides/audio.md): Phát âm thanh 2D/3D với các backend Null, FMOD và irrKlang.
- [Thu âm Microphone](guides/audio_capture.md): Nạp luồng micro WASAPI, lọc tiếng ồn noise gate và phản hồi giọng nói.
- [Giao diện Người dùng (UI)](guides/ui.md): Component UI dạng Canvas, button, text, căn chỉnh và quy tắc bố cục.
- [Hệ thống Điều hướng NavMesh](guides/navigation.md): Tạo NavMesh Recast/Detour, tìm đường pathfinding và spatial hashing.
- [Quản lý Thiết bị](guides/device_management.md): Chế độ hiển thị, tỷ lệ độ phân giải và xử lý thiết bị đầu vào.

---

## 5. Scripting, State & Systems
- [Lập trình Scriptable API](scripting/scriptable_api.md): Vòng đời C++ `Scriptable`, truy vấn entity và hiển thị biến ra editor.
- [Quản lý State API](state/state_api.md): Quản lý `StateMachine` theo dạng Stack, chuyển đổi state và lớp phủ modal.
- [Hệ thống System Cốt lõi](systems/core_systems.md): Đăng ký `ISystem` tùy chỉnh và thứ tự các phase cập nhật khung hình.
- [Mở rộng Engine](guides/extending_engine.md): Xây dựng plugin tùy chỉnh, provider và chiến lược renderer.

---

## 6. Công cụ Editor & Debugging
- [Công cụ Debug & Editor Controls](guides/debug_system.md): Phím tắt (F1-F12), giao diện Debug GUI, hệ thống log `AXIS_LOG_*`, chẩn đoán `AXIS_ASSERT`, `DebugConfig`, vẽ khung vật lý/âm thanh và profiling.
- [Sổ tay Editor](guides/editor.md): Chỉnh sửa scene trực quan, hierarchy, inspector, prefabs và quản lý dự án với ImGui.
- [Mở rộng Editor](guides/editor_extensions.md): Viết cửa sổ editor tùy chỉnh, inspector drawers và wizard panels.
