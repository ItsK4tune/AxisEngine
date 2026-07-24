# Manager và service của engine

> [English](../../eng/core/managers.md)

Manager là service tập trung do `Application` tạo và quản lý lifecycle.

## 1. ResourceManager

**Include:** `<resource/logic/resource_manager.h>`  
**Từ `State`/`Scriptable`:** `Get<ResourceManager>()`

Quản lý load, cache và truy xuất model, texture, shader, font, sound, video,
animation và fragment.

- Deduplicate theo name/path.
- Hỗ trợ async load qua Job System.
- Hot reload shader/texture khi source thay đổi.
- `STRICT_ASSET_LOADING` tắt debug fallback để lỗi asset được trả về rõ ràng.
- API thường dùng: `GetModel`, `GetTexture`, `GetShader`, `LoadModel`,
  `LoadTexture`.

## 2. SceneManager

Gameplay nên dùng các hàm scene của `EngineAccessor`: `LoadScene`,
`QueueLoadScene`, `ChangeScene`, `UnloadScene`, `PopScene`.

- `LoadScene`: thêm scene `.axs` vào world.
- `ChangeScene`: dọn scene hiện tại và nạp scene mới.
- `ClearAllScenes`: reset world, trừ scene persistent theo policy.
- Các thao tác queue chạy FIFO.

## 3. InputManager và handler

Gameplay dùng `GetAction`, `GetActionDown`, `GetActionUp`. Code thiết bị cấp thấp
có thể resolve `IOHandler`.

- Keyboard/mouse raw state qua `IOHandler`.
- Cursor: `Normal`, `Hidden`, `Locked`, `LockedHidden`.
- Action binding được load/save từ cấu hình và tránh phụ thuộc key code trực tiếp.

## 4. AudioService

**Include:** `<audio/logic/audio_service.h>`  
**Truy cập:** `Resolve<AudioService>()`

Service bọc `IAudioEngine` đã chọn; microphone là
`IAudioCaptureService` độc lập.

- `Play2D(source, loop)`: nhạc nền/UI.
- `Play3D(source, position, loop)`: âm thanh không gian.
- `UpdateListener(position, direction, up)`: đồng bộ listener với camera.

## Xem thêm

- [Kiến trúc](architecture.md)
- [Scene format](../guides/scene_format.md)
- [Cấu trúc project](../guides/project_structure.md)
