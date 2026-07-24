# Asset và quản lý resource

> [English](../../eng/guides/assets.md)

`ResourceManager` quản lý lifecycle, cache và deduplicate asset.

## 1. Định dạng hỗ trợ

| Nhóm | Định dạng | Ghi chú |
|---|---|---|
| Model 3D | `.fbx`, `.obj`, `.gltf` | FBX phù hợp character có animation |
| Animation | `.fbx`, `.dae` | Cần model có rig tương thích |
| Texture | `.png`, `.jpg`, `.tga` | PNG cho alpha, JPG cho albedo lớn |
| Audio | `.wav`, `.mp3`, `.ogg` | WAV cho SFX, MP3/OGG cho nhạc |
| Video | `.mp4` | Decode async bằng FFmpeg |
| Shader | `.vs`, `.fs`, `.cs` | GLSL graphics/compute |

## 2. Khai báo trong `.axs`

```yaml
axis_scene:
  Resources:
    Model:
      Name: playerModel
      Path: models/hero.fbx
      Static: 0
    Texture:
      Name: hero_albedo
      Path: textures/hero_d.png
    Sound:
      Name: footsteps
      Path: audio/steps.wav
    ComputeShader:
      Name: particle_update
      Path: shaders/particle_update.cs
    Video:
      Name: intro
      Path: videos/intro.mp4
```

Resource trong block `Resources` preload khi scene bắt đầu. Asset chỉ xuất hiện
trong component được lazy-load khi entity được tạo.

## 3. Tổ chức và quy ước

```text
assets/
  models/
  textures/
  shaders/
  audio/
  video/
  fonts/
  scenes/
```

Tên nên ổn định và duy nhất: `carModel`, `outlineShader`, `stone_normal`.

## 4. Quản lý nâng cao

- Async: `res.LoadModelAsync("City", "models/city.fbx")`.
- Production: bật `STRICT_ASSET_LOADING: 1` để lỗi thay vì dùng fallback debug.
- Hot reload Debug: graphics/compute shader và texture file-backed.
- Script/metadata không hot reload vì script registry được compile.
- Cache deduplicate theo name/path và trả shared identity.
- Compute shader: lấy bằng `GetComputeShader`, set uniform rồi
  `Dispatch(x, y, z, barrier)`; group bằng 0 bị từ chối.
- Video tool cache dùng `LoadVideo`/`GetVideo`/`UnloadVideo`; mỗi
  `VideoPlayerComponent` có decoder riêng vì timeline và seek là state entity.

## Xem thêm

- [Scene format](scene_format.md)
- [Kiến trúc](../core/architecture.md)
- [Manager](../core/managers.md)
