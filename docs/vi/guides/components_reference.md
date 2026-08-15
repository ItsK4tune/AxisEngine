# Hướng dẫn Tra cứu Component Có thể Nạp (Serializable) & EntityBuilder

> [English](../../eng/guides/components_reference.md) | [Định dạng Scene](scene_format.md) | [Lập trình Scriptable API](../scripting/scriptable_api.md) | [Mục lục Tài liệu](../INDEX.md)

---

## 1. Giới thiệu

AxisEngine sử dụng EnTT cho hệ thống Entity-Component-System (ECS). Trong các file scene (`.axs` / `.axsb`) cũng như khởi tạo bằng mã C++, entity có thể chứa các component có khả năng serialize đã đăng ký trong `ComponentLoader` và `SceneSerializer` của engine.

Để gắn và cấu hình các component có thể serialize này bằng mã C++, AxisEngine cung cấp lớp hỗ trợ fluent API `EntityBuilder`.

---

## 2. Cách dùng

1. **Thêm Component trong File Scene `.axs`**: Chỉ định khóa serializer của component bên dưới entity (ví dụ: `Transform:`, `Renderer:`, `RigidBody:`).
2. **Thêm Component qua `EntityBuilder`**: Khởi tạo `EntityBuilder(scene, resources, "Ten")`, gọi các phương thức cấu hình (`WithPosition()`, `WithPBRMesh()`, `WithRigidBody()`), và gọi `Build()`.
3. **Thêm Component qua API C++**: Sử dụng `entity.AddComponent<T>()` hoặc truy vấn component qua `entity.HasComponent<T>()`.

---

## 3. Ví dụ

### 1. Ví dụ Khai báo Component trong Scene `.axs`
```yaml
axis_scene:
    Version: 1.0

Entities:
    - Name: "Physics Box"
      Transform:
          Position: 0.0 2.0 0.0
          Rotation: 0.0 45.0 0.0
      Renderer:
          Model: "models/cube.obj"
          Shader: "shaders/pbr.glsl"
          CastShadow: 1
      RigidBody:
          Mass: 2.5
          Shape: BOX
```

### 2. Ví dụ Tạo bằng Mã C++ qua `EntityBuilder`
```cpp
#include <axis_sdk.h>

void SpawnBoxEntity(Scene& scene, ResourceManager& resources, const Vector3& pos) {
    EntityBuilder builder(scene, resources, "Physics Box");

    builder.WithPosition(pos)
           .WithRotationEuler(Vector3(0.0f, 45.0f, 0.0f))
           .WithPBRRenderable("models/cube.obj", "shaders/pbr.glsl", pos)
           .WithRigidBody(2.5f /* mass */, false /* isStatic */, false /* isTrigger */)
           .WithTag("Interactive");

    Entity box = builder.Build();
    AXIS_LOG_INFO("Entity da duoc tao qua EntityBuilder thanh cong");
}
```

---

## 4. Tra cứu Param, Setting & API Reference

### Bảng Tra cứu Chi tiết Các Component Có thể Thêm / Serialize

| Khóa Serializer | Component Struct C++ | Tham số chính trong `.axs` | Phân loại | Mục đích & Mô tả |
| :--- | :--- | :--- | :--- | :--- |
| `Transform` | `TransformComponent` | `Position`, `Rotation`, `Scale` | Không gian | Vị trí 3D, góc xoay và tỷ lệ local |
| `Renderer` | `MeshRendererComponent` | `Model`, `Shader`, `Color`, `CastShadow`, `ReceiveShadow`, `Order` | Đồ họa | Renderer model hình học mesh 3D |
| `Material` | `MaterialComponent` | `Metallic`, `Roughness`, `AO`, `Albedo`, `Normal`, `Emission` | Đồ họa | Texture vật liệu PBR và tham số bề mặt |
| `Camera` | `CameraComponent` | `Primary`, `FOV`, `Near`, `Far`, `AspectRatio`, `Orthographic` | Dựng hình | Camera chiếu góc nhìn perspective hoặc orthographic |
| `DirectionalLight` | `DirectionalLightComponent` | `Active`, `Direction`, `Color`, `Intensity`, `CastShadow` | Ánh sáng | Nguồn phát ánh sáng mặt trời hướng |
| `PointLight` | `PointLightComponent` | `Active`, `Color`, `Intensity`, `Radius`, `CastShadow` | Ánh sáng | Nguồn phát ánh sáng điểm mọi hướng |
| `SpotLight` | `SpotLightComponent` | `Active`, `Direction`, `Color`, `Intensity`, `CutOff`, `Radius` | Ánh sáng | Nguồn phát ánh sáng nón hội tụ |
| `LightProbe` | `LightProbeComponent` | `Radius`, `Intensity` | Ánh sáng | Light probe môi trường hài cầu (Spherical Harmonics) |
| `ReflectionProbe` | `ReflectionProbeComponent` | `Type`, `Resolution`, `BoxProjection` | Ánh sáng | Reflection probe cubemap môi trường |
| `Reflective` | `ReflectiveComponent` | `Active`, `Reflectivity`, `FresnelPower`, `FresnelBias` | Ánh sáng | Phản xạ đẳng hướng Fresnel specular |
| `PlanarReflection` | `PlanarReflectionComponent` | `Resolution`, `Normal` | Ánh sáng | Phase dựng hình phản xạ mặt phẳng gương |
| `SkyboxRenderer` | `SkyboxRenderComponent` | `Skybox`, `Shader`, `Primary` | Đồ họa | Renderer skybox cubemap môi trường |
| `LOD` | `LODComponent` | `Models`, `Distances` | Đồ họa | Bộ chuyển đổi mesh LOD theo khoảng cách |
| `Occlusion` | `OcclusionComponent` | `Active` | Đồ họa | Trạng thái hiển thị culling occlusion |
| `Streaming` | `StreamingComponent` | `ModelPath`, `LoadDistance`, `UnloadDistance` | Đồ họa | Nạp luồng asset theo khoảng cách |
| `Decal` | `DecalComponent` | `AlbedoMap`, `Opacity`, `Roughness`, `Metallic`, `TintColor` | Môi trường | Texture decal chiếu lên bề mặt |
| `Terrain` | `TerrainComponent` | `TerrainSize`, `MaxHeight`, `HeightMap`, `SplatMap` | Môi trường | Renderer mesh địa hình heightmap và va chạm vật lý |
| `PostProcess` | `PostProcessComponent` | `Active`, `Effects` | Môi trường | Danh sách pass pipeline post-processing toàn màn hình |
| `RigidBody` | `RigidBodyComponent` | `Mass`, `IsStatic`, `IsTrigger`, `Friction`, `Restitution` | Vật lý | Động lực học vật thể rắn Bullet 3D |
| `RigidShape` | `RigidShapeComponent` | `ShapeType`, `Size`, `Radius`, `Height` | Vật lý | Định nghĩa hình dạng va chạm |
| `CharacterController` | `CharacterControllerComponent` | `StepHeight`, `MaxSlope`, `Radius`, `Height` | Vật lý | Kinematic character controller cho nhân vật di chuyển |
| `AudioSource` | `AudioSourceComponent` | `File`, `PlayOnAwake`, `Loop`, `Is3D`, `Volume`, `Pitch` | Âm thanh | Nguồn phát clip âm thanh 2D/3D |
| `VideoPlayer` | `VideoPlayerComponent` | `VideoPath`, `Loop`, `Volume`, `Speed`, `PlayOnAwake` | Media | Giải mã video và phát texture |
| `ParticleEmitter` | `ParticleEmitterComponent` | `SpawnRate`, `LifeTime`, `StartSize`, `EndSize`, `MinVelocity` | Media & VFX | Thiết lập hệ thống phát hạt |
| `Animation` | `AnimationComponent` | `Animation`, `Speed`, `Rate`, `BlendFactor` | Media & VFX | Trạng thái phát animation skeletal |
| `UITransform` | `UITransformComponent` | `Position`, `Size`, `AnchorMin`, `AnchorMax`, `Pivot`, `ZOrder` | Bố cục UI | Phạm vi transform 2D UI trên màn hình |
| `UIRenderer` | `UIRendererComponent` | `Texture`, `Color` | Trực quan UI | Renderer texture hình ảnh/panel UI 2D |
| `UIText` | `UITextComponent` | `Text`, `Font`, `Scale`, `Color`, `Alignment` | Trực quan UI | Renderer nhãn văn bản phông chữ TrueType |
| `UIFlex` | `UIFlexLayoutComponent` | `Direction`, `Spacing` | Bố cục UI | Sắp xếp tự động kiểu Flexbox 2D |
| `UIInteractive` | `UIInteractiveComponent` | `Interactable` | Logic UI | Mục tiêu tương tác chuột và callback click |
| `UIAnimation` | `UIAnimationComponent` | `AnimateColor`, `AnimateScale` | Logic UI | Hiệu ứng chuyển động animation cho button UI |
| `PathFollower` | `PathFollowerComponent` | `MoveSpeed`, `RotationSpeed` | Điều hướng | Theo dõi di chuyển theo đường cong spline |
| `NavMesh` | `NavMeshComponent` | `Dynamic`, `WalkableNormalY` | Điều hướng | Thiết lập lưới điều hướng Recast NavMesh |
| `NavigationGrid` | `NavigationGridComponent` | `Width`, `Height`, `CellSize` | Điều hướng | Bản đồ lưới 2D tìm đường |
| `Fragment` | `FragmentComponent` | `Path`, `Overrides` | Prefab | Liên kết prefab đoạn scene tái sử dụng |
| `Network` | `NetworkComponent` | `NetworkID`, `OwnerID`, `IsLocal` | Mạng | Định danh đồng bộ mạng |
| `Script` | `ScriptComponent` | `Class`, `Properties` | Scripting | Gắn script C++ `Scriptable` |

### Bảng Tra cứu API `EntityBuilder`

| Phân loại | Phương thức | Mô tả |
| :--- | :--- | :--- |
| **Tài nguyên** | `WithTextureResource(name, path, async)` | Đăng ký asset texture với ResourceManager |
| **Tài nguyên** | `WithModelResource(name, path, isStatic)` | Đăng ký asset mesh model 3D |
| **Tài nguyên** | `WithShaderResource(name, vert, frag, geom)` | Đăng ký chương trình GLSL shader pipeline |
| **Tài nguyên** | `WithFontResource(name, path, fontSize)` | Đăng ký asset phông chữ TrueType |
| **Tài nguyên** | `WithSkyboxResource(name, faces)` | Đăng ký bộ 6 mặt texture cubemap |
| **Meta Cốt lõi** | `WithName(name)` | Đặt chuỗi tên debug cho entity |
| **Meta Cốt lõi** | `WithTag(tag)` | Đặt chuỗi tag phân loại cho entity |
| **Meta Cốt lõi** | `WithLayer(layer)` | Đặt mặt nạ layer va chạm/dựng hình |
| **Meta Cốt lõi** | `WithActive(active)` | Đặt flag trạng thái active cho entity |
| **Meta Cốt lõi** | `WithParent(parentEntity)` | Thiết lập liên kết phân cấp cha-con |
| **Transform** | `WithPosition(pos)` | Đặt vector vị trí 3D |
| **Transform** | `WithRotationEuler(rotDegrees)` | Đặt góc xoay Euler 3D tính bằng độ |
| **Transform** | `WithScale(scale)` | Đặt vector tỷ lệ 3D hoặc co giãn đồng đều |
| **Dựng hình** | `WithMesh(modelName, shaderName)` | Gắn component mesh renderer |
| **Dựng hình** | `WithPBRMaterial(metallic, roughness, ao)` | Cấu hình thuộc tính bề mặt vật liệu PBR |
| **Dựng hình** | `WithPBRRenderable(model, shader, pos, rot, scale, metal, rough)` | Thiết lập nhanh transform, mesh và vật liệu PBR |
| **Dựng hình** | `WithLOD(models, distances)` | Cấu hình ngưỡng khoảng cách mesh LOD |
| **Vật lý** | `WithRigidBody(mass, isStatic, isTrigger)` | Gắn component vật lý body rắn Bullet 3D |
| **Vật lý** | `WithCharacterController(controller, stepHeight, maxSlope, radius, height)` | Gắn kinematic character controller |
| **Ánh sáng** | `WithDirectionalLight(dir, color, intensity)` | Gắn nguồn phát ánh sáng mặt trời hướng |
| **Ánh sáng** | `WithPointLight(color, intensity, radius)` | Gắn nguồn phát ánh sáng điểm mọi hướng |
| **Ánh sáng** | `WithSpotLight(dir, color, intensity, radius)` | Gắn nguồn phát ánh sáng nón hội tụ |
| **Bố cục UI** | `WithUITransform(pos, size, zIndex)` | Đặt vị trí 2D, kích thước và thứ tự dựng hình UI |
| **Bố cục UI** | `WithUIAnchored(anchor, pos, size)` | Cấu hình vị trí neo anchor UI |
| **Trực quan UI** | `WithUIText(text, fontName, scale, color)` | Gắn nhãn dựng hình văn bản TrueType |
| **Trực quan UI** | `WithUITexture(textureName, color)` | Gắn renderer texture sprite UI |
| **Scripting** | `WithScript(scriptName)` | Gắn script đã đăng ký theo tên factory |
| **Scripting** | `WithScriptable(className, instantiateFunc)` | Gắn script qua lambda factory |
| **Âm thanh** | `WithAudioSource(filePath, playOnAwake, loop, is3D, volume)` | Gắn component nguồn phát clip âm thanh 2D/3D |
| **Build** | `Build()` | Hoàn tất cấu hình và trả về handle EnTT `Entity` |
