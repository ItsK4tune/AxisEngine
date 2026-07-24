# Hướng dẫn graphics và rendering

> [English](../../eng/guides/graphics.md)

AxisEngine hỗ trợ forward/deferred render path với shader ABI dùng chung.

## 1. Component và lighting

- `MeshRenderer`: model, shader, render bucket.
- `Material`: PHONG hoặc PBR.
- `LOD`: đổi model theo khoảng cách.
- `SkyboxRenderer`: environment cubemap.
- `ParticleEmitter`: simulate CPU, render GPU instancing.

Default shader hỗ trợ tối đa bốn directional, point và spot light mỗi loại.
Direction của directional/spot theo transform; point light dùng radius và
attenuation.

## 2. Shadow

Tối đa hai shadow caster mỗi loại để giữ texture-unit budget.

- `SHADOWS: 0`: tắt.
- `SHADOWS: 1`: directional đầu tiên.
- `SHADOWS: 2`: mọi caster được hỗ trợ.

```yaml
axis_config:
  SHADOWS: 2
  SHADOW_SIZE: 100.0
  SHADOW_DISTANCE: 150.0
```

## 3. Culling và tối ưu

- Frustum culling.
- Hardware occlusion query khi entity có `Occlusion`.
- Instance batching cho static mesh cùng model/shader.
- Render order theo bucket.
- Layer visibility theo `FILTER_LAYER`.
- `SPATIAL_CULLING=AUTO` chọn linear hoặc octree theo đo đạc runtime.

## 4. Post-process và anti-aliasing

`PostProcessPipeline` chạy full-screen pass. AA: `NONE`, `FXAA`, `TAA`; filter
có grayscale, invert, sharpen, blur, edge. TAA dùng history target luân phiên và
reset khi resize hoặc camera matrix không hợp lệ.

## 5. Shader ABI

### Vertex layout

- location 0: position `vec3`.
- location 1: normal `vec3`.
- location 2: texcoord `vec2`.
- location 5/6: bone ID/weight.

### Binding chuẩn

- Model: `uniform mat4 u_Model`.
- Camera UBO: binding `20`.
- Light metadata UBO: `21`.
- Global frame UBO: `22`.
- Directional/point/spot SSBO: `23`/`24`/`25`.
- Audio pulse SSBO: `26`.
- Tối đa 128 bone, bốn influence/vertex.

```glsl
layout (std140, binding = 22) uniform GlobalData {
    float u_Time;
    float u_DeltaTime;
    vec2  u_Resolution;
    vec4  _padding[3];
};
```

Material có tám custom port: C++ ghi
`material.desc.ports.data[index]`; shader đọc `u_CustomPorts[8]`.

```yaml
Shader:
  Name: myCustomShader
  vertex: shaders/custom.vs
  geometry: shaders/custom.gs
  fragment: shaders/custom.fs
```

Runtime dùng `IRenderService`/`IRenderRuntimeControl`; ownership renderer và
frame lifecycle vẫn là nội bộ.

### ABI custom post-process

| Input | Unit | Uniform |
|---|---:|---|
| Current color | 0 | `u_ScreenTexture` |
| Capture depth | 1 | `u_DepthTexture` |
| G-buffer normal | 2 | `u_NormalTexture` |
| World position texture tùy chọn | 3 | `u_WorldPositionTexture` |

Pipeline còn set `u_InverseViewProjection`, `u_AudioLevel`, `u_PulseCount` và
flag `u_Has*`. OpenGL thường dựng world position từ depth:

```glsl
vec4 world = u_InverseViewProjection
           * vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
world /= world.w;
```

```glsl
struct AudioPulse {
    vec3 origin;
    float intensity;
    float peak;
    float age;
    float duration;
    float padding;
};

layout(std430, binding = 26) readonly buffer AudioPulseBuffer {
    AudioPulse u_AudioPulses[];
};
```

Tối đa 64 pulse active. Effect mask: color `1`, depth `2`, normal `4`, world
position `8`, camera matrix `16`, audio pulse `32`. Mặc định chỉ color; input
khác phải opt-in.

## 6. Skeletal animation

Hỗ trợ clip theo tên, blend hai clip, cross-fade và parameter-driven graph.
Transition có AND/OR/XOR/NAND/NOR/XNOR, NOT từng condition, trigger consumption
và normalized exit time. Chưa có layer, mask, retargeting, root motion, IK hoặc
2D blend tree.

```glsl
layout (location = 5) in ivec4 aBoneIds;
layout (location = 6) in vec4 aWeights;
uniform mat4 u_FinalBonesMatrices[128];
```

Vertex shader phải bỏ ID `-1`, chặn ID `>=128`, cộng bốn bone transform theo
weight rồi áp instance/model matrix.

## 7. Decal

Deferred/forward decal được sort theo `renderOrder`, có lifetime và tag filter
trong deferred path.

## 8. Texture nén

Loader nhận DDS 2D BC1–BC5 với mip chain, legacy FourCC hoặc DX10 header.
Block được upload trực tiếp; format hỏng/không hỗ trợ đi theo strict asset
policy. KTX2/Basis là trách nhiệm của provider.

## Xem thêm

- [Scene format](scene_format.md)
- [Kiến trúc](../core/architecture.md)
