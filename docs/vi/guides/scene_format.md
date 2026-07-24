# Tham chiếu định dạng scene `.axs`

> [English](../../eng/guides/scene_format.md)

`.axs` là định dạng phân cấp dựa trên indentation, dùng để mô tả resource,
entity và component. `SceneSerializer` parse trực tiếp; tab indentation bị từ
chối, hãy dùng space.

## 1. Resources

```yaml
axis_scene:
  Resources:
    Shader:
      Name: modelShader
      Vertex: assets/shaders/model.vs
      Fragment: assets/shaders/model.fs
    Model:
      Name: unityChan
      Path: assets/models/unitychan.fbx
      Static: true
```

## 2. Configuration

Config runtime nên đặt dưới `axis_config`; legacy scene vẫn có thể chứa config
và `ConfigLoader` sẽ sanitize trước khi publish.

```yaml
axis_config:
  WINDOW_WIDTH: 1920
  WINDOW_HEIGHT: 1080
  WINDOW_MODE: BORDERLESS_FULLSCREEN
  VSYNC: 1
  ANTIALIASING: TAA
  MSAA: 4
  HDR_ENABLED: 1
  BLOOM_ENABLED: 1
  TONEMAPPING: ACES
  SHADOW_RESOLUTION: 2048
  JOB_THREADS: -1
  LOG_LEVEL: VERBOSE
  PHYSICS_MODE: BALANCED
```

## 3. Entities

Entity bắt đầu bằng name dưới `Entities`. `Tag`, `Layer`, `Parent`, `Active`,
`Transient`, `RenderOrder` và scene membership là metadata.

```yaml
axis_scene:
  Entities:
    Player:
      Tag: Player
      Layer: 1
```

## 4. Components

```yaml
axis_scene:
  Entities:
    MyCube:
      Tag: default
      Component: Transform
        Position: 0.0 0.0 0.0
        Rotation: 0.0 0.0 0.0
        Scale: 1.0 1.0 1.0
      Component: Renderer
        Model: cubeModel
        Shader: defaultShader
      Component: Script
        Class: PlayerController
      Component: RigidBody
        Type: CAPSULE
        Radius: 1.0
        Height: 1.8
        Mass: 70.0
        BodyType: DYNAMIC
        Offset: 0.0 0.9 0.0
        AngularFactor: 0 1 0
        Restitution: 0.2
```

Xem [tham chiếu component](components_reference.md) để biết field chính xác.

## 5. Quy tắc authoring

1. Entity name nên duy nhất trong scene context.
2. Dùng space, không dùng tab.
3. Key phân biệt hoa/thường; key không biết sẽ phát warning.
4. Giữ runtime config tách khỏi scene khi có thể.
5. Dùng `Layer` bitmask cho camera/physics filtering.
6. Dùng path tương đối project; chỉ dùng `asset://` cho asset tích hợp.
7. Scene/asset hiện được coi là trusted project input.

## 6. Nhóm component

- Transform/cơ bản: xem [kiến trúc](../core/architecture.md).
- Renderer, material, light, shadow: [graphics](graphics.md).
- Rigid body/controller: [physics](physics.md).
- NavMesh/path follower: [navigation](navigation.md).
- UI transform/renderer/text: [UI](ui.md).
- Script: [Scriptable API](../scripting/scriptable_api.md).
- Preload resource và tag: [asset](assets.md).

## Binary `.axsb`

`axis_compile` tạo `.axsb`. Loader kiểm tra magic/version và giới hạn file,
payload, string, entity trước cấp phát; legacy load rollback entity nếu file
hỏng. Không coi `.axsb` từ nguồn ngoài là dữ liệu an toàn.
