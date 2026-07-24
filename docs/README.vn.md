<p align="center">
  <img src="include/engine/asset/project/logo.png" alt="Logo AxisEngine" width="220">
</p>

# AxisEngine

AxisEngine là game engine và multimedia framework viết bằng C++20, sử dụng ECS.
Bản hiện tại cung cấp renderer OpenGL, physics Bullet, Null audio mặc định, và
có thể chọn FMOD hoặc irrKlang khi đã cài SDK tương ứng. Repository còn bao gồm
scene serialization, scripting, navigation, networking, video, editor ImGui,
scene compiler và ứng dụng mẫu gồm 33 scenario.

Đây là engine/framework, không phải game hoàn chỉnh. Khi bật sample, executable
được tạo ra là `axis_samples`. Ứng dụng sử dụng engine cần cung cấp lớp
`Application` và `State` đầu tiên.

[Trang song ngữ](README.md) | [English README](README.eng.md) |
[Tài liệu tiếng Việt](docs/vi/INDEX.md) | [English documentation](docs/eng/INDEX.md) |
[Báo cáo audit](docs/vi/audit/source_audit_2026-07-23.md)

## Khả năng hiện có

| Hạng mục | Implementation đi kèm | Ghi chú |
| --- | --- | --- |
| Ngôn ngữ | C++20 | CMake 3.20 trở lên |
| Graphics | OpenGL | Renderer yêu cầu OpenGL 4.6; không chạy được trên macOS |
| Physics | Bullet | Chọn tại thời điểm configure |
| Audio playback | Null, FMOD, irrKlang | Null là mặc định; FMOD/irrKlang cần SDK |
| Microphone | WASAPI trên Windows | Nền tảng khác trả về trạng thái unsupported |
| Nền tảng | Windows, Linux; macOS giới hạn | macOS không chạy được renderer OpenGL 4.6 hiện tại |
| Editor | ImGui, tùy chọn | Bật bằng `ENABLE_EDITOR=ON` |
| Scene | `.axs`, `.axsb` | `.axs` là YAML subset riêng; `.axsb` là binary đã compile |

Các enum Vulkan, DirectX, PhysX và OpenAL vẫn còn để tương thích source/scene,
nhưng bản phát hành này không có backend tương ứng.

## Build nhanh trên Windows

Chỉ build static engine library:

```powershell
cmake --preset windows-msvc
cmake --build --preset windows-release
```

Build editor và sample:

```powershell
cmake --preset windows-msvc-editor
cmake --build build --config Release --parallel
.\build\bin\Release\axis_samples.exe
```

Build và chạy test:

```powershell
cmake -S . -B build -DENABLE_TESTS=ON
cmake --build build --config Release --target axis_test --parallel
ctest --test-dir build -C Release --output-on-failure
```

`axis_tools.bat` và `axis_tools.sh` là menu tiện ích. CMake vẫn là workflow
chuẩn và dễ tái lập nhất.

## Public API

- Dùng `axis_sdk.h` cho game và tool.
- Dùng `axis_plugin.h` để thay provider/module.
- Chỉ dùng `axis_advanced.h` khi cần tích hợp system cấp thấp.
- Header trong thư mục `strategy/` là implementation detail và không được cài
  cùng SDK package.

Đọc [manual tiếng Việt](docs/vi/MANUAL.md) để xem workflow đầy đủ, cấu trúc
project, scene, script, editor, đóng gói và troubleshooting.

Các hướng dẫn chính:

- [Bắt đầu sử dụng](docs/vi/core/getting_started.md)
- [Hướng dẫn build](docs/vi/guides/build_guide.md)
- [Hướng dẫn editor](docs/vi/guides/editor.md)
- [Mở rộng editor](docs/vi/guides/editor_extensions.md)
- [Scriptable API](docs/vi/scripting/scriptable_api.md)
- [State API](docs/vi/state/state_api.md)

## Ranh giới an toàn

AxisEngine xem scene và asset là dữ liệu tin cậy của project. Network mặc định
`RequireSecure` và từ chối khởi động nếu chưa có `INetworkSecurityProvider`.
Chế độ `TrustedNetwork` dùng ENet không xác thực/mã hóa và không được public ra
Internet. AxisEngine chưa đi kèm cryptographic provider; ứng dụng production
phải cung cấp implementation đã được review.

Xem [báo cáo khắc phục](docs/vi/audit/remediation_2026-07-23.md).

## License

Repository hiện chưa có file license. Không nên mô tả hoặc phân phối dự án như
phần mềm MIT cho đến khi license chính thức được thêm vào. FMOD và irrKlang cũng
có điều khoản cấp phép riêng.
