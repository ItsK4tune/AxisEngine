# Bề mặt API công khai

> [English](../../eng/core/api_surface.md)

AxisEngine chia header đã cài đặt theo mức ổn định và đối tượng sử dụng.

| Header | Đối tượng | Cam kết |
|---|---|---|
| `axis_sdk.h` | Game và tool | API ổn định cho application, state, component và kiểu công khai |
| `axis_plugin.h` | Module thay thế được | Interface backend, lifecycle, resource, navigation, network, editor, serializer và post-process |
| `axis_advanced.h` | Tích hợp sâu với engine | System/manager cụ thể; được hỗ trợ nhưng dễ thay đổi hơn |
| `axis_all.h` | Tương thích source | Hợp của ba bề mặt trên; không nên dùng cho plugin tái sử dụng |

Header dưới `strategy/`, editor `modules/` và `panels/` là chi tiết triển khai,
không thuộc SDK cài đặt. `ServiceLocator` là cơ chế runtime; gameplay nên dùng
`EngineAccessor` hoặc interface trong `axis_plugin.h`.

`StaticBatchManager` và `TextureAtlas` vẫn là implementation nội bộ vì chưa có
renderer contract giữ nguyên material identity và entity picking.

## Quy tắc ownership và thay thế

- `Application` sở hữu backend do factory của `AppBuilder` trả về.
- Override resource library là `shared_ptr` được builder profile giữ lại.
- Platform provider trả `shared_ptr` để lời gọi đang chạy vẫn hợp lệ khi đổi provider.
- System, script, component codec, post-process và editor extension đăng ký qua
  registry tương ứng; phải unregister owner trước khi unload module.
- Gameplay dùng localization qua `ILocalizationService`.
- Mỗi thời điểm chỉ một `Application` được initialize; worker của Job System
  nhìn thấy đúng service context của application đó.

## Provider hiện có

- Graphics: OpenGL 4.6.
- Physics: Bullet.
- Playback: Null, tùy chọn FMOD hoặc irrKlang.
- Microphone trên Windows: WASAPI; nền tảng khác cần provider riêng.
- Vulkan, DirectX, PhysX và OpenAL chỉ còn để tương thích serialization/source,
  chưa phải backend có thể chọn.
- Renderer OpenGL 4.6 không chạy trên macOS OpenGL 4.1 và sẽ fail rõ khi startup.
