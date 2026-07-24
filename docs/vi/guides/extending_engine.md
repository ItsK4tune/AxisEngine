# Mở rộng AxisEngine

> [English](../../eng/guides/extending_engine.md)

Game bắt đầu với `axis_sdk.h`; plugin contract nằm trong `axis_plugin.h`;
`axis_advanced.h` dành cho tích hợp sâu có chi phí nâng cấp cao hơn. Header
`engine/*/strategy` là nội bộ và không được install.

## Provider của application

Tạo `AppBuilder`, cấu hình factory rồi truyền vào constructor `Application`.
Provider thuộc riêng application profile, không rò sang test/application khác.

```cpp
AppBuilder providers;
providers
    .WithGraphicsContextFactory([](const AppConfig&) {
        return std::make_unique<MyGraphicsContext>();
    })
    .WithPhysicsWorldFactory([](const AppConfig&) {
        return std::make_unique<MyPhysicsWorld>();
    })
    .WithAudioEngineFactory([](const AppConfig&) {
        return std::make_unique<MyAudioEngine>();
    })
    .WithAudioCaptureFactory([] {
        return std::make_unique<MyAudioCaptureService>();
    });
```

Factory phải trả object hợp lệ. Null/exception làm `Initialize` fail và rollback
subsystem đã initialize. Có thể thay riêng shader, texture, model, sound, font,
skybox library bằng `With*Library(shared_ptr<...>)`.

Platform provider dùng `SetPlatformFileSystemProvider` và
`SetPlatformRuntimeProvider`; lấy handle an toàn bằng `AcquirePlatform*`.

## System và script

Đăng ký system theo application trong `RegisterUserSystems`; system trùng tên
built-in sẽ thay built-in đó. Script đăng ký trong `RegisterUserScripts`.

```cpp
void GameApplication::RegisterUserSystems(ISystemRegistry& systems) {
    systems.RegisterSystem(std::make_unique<MyNavigationSystem>());
}

void GameApplication::RegisterUserScripts() {
    RegisterScript<PlayerController>("PlayerController");
}
```

Runtime chỉ link `Axis::Engine`. Editor host link `Axis::Editor`, target này tự
truyền `ENABLE_EDITOR` và giữ bootstrap anchor.

## Registry có owner

- `IPostProcessRegistry`: effect, priority, rect, input mask.
- `IEditorExtensionRegistry`: module/panel factory.
- `IComponentCodecRegistry`: loader/serializer component.
- Gọi `UnregisterOwner` trước khi unload module.
- `INetworkService` và `INavigationService` che implementation ENet/pathfinding.

## File loader thống nhất

Implement `ILoaderStrategy`, trả type name ổn định từ `GetName`, rồi gọi
`ResourceManager::RegisterLoader`. Loader module đăng ký trước initialize không
bị default `CONFIG`/`INPUT` ghi đè.

## Biên truy cập runtime

`State` và `Scriptable` kế thừa `EngineAccessor`. Ưu tiên API scene, input,
config, data, time, render và physics cấp cao; chỉ dùng `Get<T>()`,
`Resolve<T>()`, `GetSystem<T>()` khi cần tích hợp nâng cao.

Xem [bề mặt API](../core/api_surface.md), [microphone](audio_capture.md),
[graphics](graphics.md) và [mở rộng editor](editor_extensions.md).
