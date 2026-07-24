# Tài liệu AxisEngine

> [English](../eng/INDEX.md)

Tài liệu này phản ánh repository tại commit `787f438` ngày 2026-07-23. Khi tài
liệu khác code, public header, CMake, sample và test là nguồn chuẩn.

## Bắt đầu

- [Manual tiếng Việt](MANUAL.md)
- [README tiếng Việt](../../README.vn.md)
- [Bắt đầu sử dụng](core/getting_started.md)
- [Hướng dẫn build](guides/build_guide.md)
- [Bề mặt API công khai](core/api_surface.md)
- [Audit source code](audit/source_audit_2026-07-23.md)
- [Báo cáo khắc phục](audit/remediation_2026-07-23.md)

## Tài liệu chi tiết

- [Kiến trúc](core/architecture.md)
- [Core system](systems/core_systems.md)
- [Manager và service](core/managers.md)
- [Cấu trúc project](guides/project_structure.md)
- [State API](state/state_api.md)
- [Scriptable API](scripting/scriptable_api.md)
- [Mở rộng engine](guides/extending_engine.md)
- [Quy định comment](guides/comment_policy.md)

## Nội dung và runtime

- [Định dạng scene](guides/scene_format.md)
- [Tham chiếu component](guides/components_reference.md)
- [Cấu hình](guides/configuration.md)
- [Asset](guides/assets.md)
- [Graphics và rendering](guides/graphics.md)
- [Physics](guides/physics.md)
- [Navigation](guides/navigation.md)
- [UI](guides/ui.md)
- [Audio](guides/audio.md)
- [Microphone](guides/audio_capture.md)
- [Thiết bị](guides/device_management.md)
- [Debug](guides/debug_system.md)

## Editor

- [Hướng dẫn editor](guides/editor.md)
- [Mở rộng editor](guides/editor_extensions.md)

Quy ước:

- `.axs` là YAML-like subset riêng của AxisEngine, không phải YAML đầy đủ.
- `asset://` trỏ tới asset nội bộ của engine.
- Enum tồn tại không đồng nghĩa backend tương ứng đã được implement.
- Nội dung trong `sample/` là ví dụ, không được install cùng SDK.
