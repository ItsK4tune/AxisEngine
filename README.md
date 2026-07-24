<p align="center">
  <img src="include/engine/asset/project/logo.png" alt="AxisEngine logo" width="220">
</p>

# AxisEngine

## English

AxisEngine is a C++20, ECS-based game and multimedia engine. The current
release ships OpenGL rendering, Bullet physics, Null audio by default, optional
FMOD/irrKlang playback, scene serialization, scripting, navigation,
networking, video, an optional ImGui editor, a scene compiler, and 33 sample
scenarios.

Start here:

- [Full English README](docs/README.eng.md)
- [English documentation index](docs/eng/INDEX.md)
- [English manual](docs/eng/MANUAL.md)
- [English source audit](docs/eng/audit/source_audit_2026-07-23.md)
- [English remediation report](docs/eng/audit/remediation_2026-07-23.md)

Quick editor/sample build on Windows:

```powershell
cmake --preset windows-msvc-editor
cmake --build build --config Release --parallel
.\build\bin\Release\axis_samples.exe
```

## Tiếng Việt

AxisEngine là game engine và multimedia framework C++20 sử dụng ECS. Bản hiện
tại cung cấp OpenGL, Bullet, Null audio mặc định, FMOD/irrKlang tùy chọn, scene
serialization, scripting, navigation, networking, video, editor ImGui, scene
compiler và 33 scenario mẫu.

Bắt đầu tại đây:

- [README tiếng Việt đầy đủ](docs/README.vn.md)
- [Mục lục tài liệu tiếng Việt](docs/vi/INDEX.md)
- [Manual tiếng Việt](docs/vi/MANUAL.md)
- [Báo cáo audit tiếng Việt](docs/vi/audit/source_audit_2026-07-23.md)
- [Báo cáo khắc phục tiếng Việt](docs/vi/audit/remediation_2026-07-23.md)

Build nhanh editor/sample trên Windows:

```powershell
cmake --preset windows-msvc-editor
cmake --build build --config Release --parallel
.\build\bin\Release\axis_samples.exe
```

> AxisEngine treats scenes and assets as trusted project input. Secure network
> mode requires an application-supplied security provider.
> AxisEngine xem scene và asset là dữ liệu tin cậy; secure network mode cần
> security provider do ứng dụng cung cấp.
