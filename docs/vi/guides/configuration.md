# Cấu hình application và build

> [English](../../eng/guides/configuration.md)

## 1. Cấu hình application

Setting nằm trong file `.axs` độc lập có root `axis_config`. Scene dùng
`axis_scene`; không dùng `Config` trong scene như cơ chế override mới.

```yaml
axis_config:
  WINDOW_WIDTH: 1920
  WINDOW_HEIGHT: 1080
  WINDOW_MODE: BORDERLESS_FULLSCREEN
  MONITOR: 0
  REFRESH_RATE: 60
  VSYNC: 1
  FPS: 120
  GRAPHICS_API: OPENGL
  PHYSICS_ENGINE: BULLET
  AUDIO_ENGINE: NULL
  SHADOWS: 1
  ANTIALIASING: TAA
  MSAA: 4
  FRUSTUM: 1
  SPATIAL_CULLING: AUTO
  PHYSICS_MODE: BALANCED
  STRICT_ASSET_LOADING: 0
  UI_REFERENCE_SIZE: 1920 1080
```

Config được parse, validate/sanitize rồi publish thành snapshot bất biến.

### Engine và system

- `LOG_LEVEL`: `NONE`, `MINIMAL`, `VERBOSE`, `DEBUG`.
- `JOB_THREADS`: `-1` tự nhận số core.
- `TIME_SCALE`: hệ số simulation.
- `ASYNC_RESOURCES`: bật load nền.
- `STRICT_ASSET_LOADING`: không dùng fallback shader/texture/model.

### Display, graphics và hậu kỳ

- `WINDOW_MODE`: `WINDOWED`, `FULLSCREEN`, `BORDERLESS`,
  `BORDERLESS_FULLSCREEN`.
- `RENDER_SCALE`, `VSYNC`, `FPS`.
- `ANTIALIASING`: `NONE`, `FXAA`, `TAA`; `MSAA`: `2`, `4`, `8`, `16`.
- `ANISOTROPY`, `HDR_ENABLED`, `TONEMAPPING`, `BLOOM_ENABLED`,
  `BLOOM_INTENSITY`, `BLOOM_THRESHOLD`, `BLOOM_RADIUS`.
- `GAMMA`, `EXPOSURE`, `SKYBOX_INTENSITY`, `AMBIENT_INTENSITY`.
- `UI_REFERENCE_WIDTH`, `UI_REFERENCE_HEIGHT`, `UI_REFERENCE_SIZE`.
- `FRUSTUM`; `SPATIAL_CULLING`: `AUTO`, `LINEAR`, `OCTREE`.

`AUTO` đo linear/octree định kỳ và dùng hysteresis cùng scene churn để tránh
đổi backend liên tục.

### Shadow

- `SHADOW_RESOLUTION`, `SHADOW_SOFTNESS`, `SHADOW_BIAS`.
- `SHADOW_SIZE`, `SHADOW_FRUSTUM`, `SHADOW_DISTANCE`.

### Physics

- `GRAVITY`.
- `PHYSICS_MODE`: `FAST`, `BALANCED`, `ACCURATE`.
- `CCD_ENABLED`, `SOLVER_ITERATIONS`, `MAX_SUBSTEPS`.

### Audio và input

- `VOLUME`.
- `AUDIO_CAPTURE_ENABLED`, `AUDIO_CAPTURE_DEVICE`.
- `AUDIO_CAPTURE_INPUT_VOLUME`, `AUDIO_CAPTURE_NOISE_GATE`,
  `AUDIO_CAPTURE_GAIN`.
- `AUDIO_CAPTURE_ATTACK_SECONDS`, `AUDIO_CAPTURE_RELEASE_SECONDS`,
  `AUDIO_CAPTURE_PEAK_DECAY_SECONDS`, `AUDIO_CAPTURE_CALIBRATION_SECONDS`.
- `AUDIO_CAPTURE_PULSE_THRESHOLD`, `AUDIO_CAPTURE_PULSE_COOLDOWN`,
  `AUDIO_CAPTURE_PULSE_DURATION`.
- `MOUSE_SENSITIVITY`, `MOUSE_INVERT_Y`, `GAMEPAD_DEAD_ZONE`.

### Backend

| Key | Provider tích hợp |
|---|---|
| `GRAPHICS_API` | `OPENGL` |
| `PHYSICS_ENGINE` | `BULLET` |
| `AUDIO_ENGINE` | `NULL`, `IRRKLANG`, `FMOD` |

Giá trị cũ `VULKAN`, `DIRECTX`, `PHYSX`, `OPENAL` vẫn parse để tương thích,
nhưng built-in validation sẽ thay provider không có. Custom factory trong
`AppBuilder` giữ nguyên enum được yêu cầu.

## 2. Runtime optimization

Editor áp các option sau live qua `ConfigChangedEvent::Optimization`:

- Resource: `OPT_RESOURCE_HOT_RELOAD`, upload budget, model/texture per frame,
  CPU mesh discard, compressed texture.
- Streaming: throttling và check interval.
- Reflection/shadow/animation: capture/build/evaluation budget và threshold.
- Navigation: spatial hash, async request, rebuild/tile budget.
- Network: batching, event/time/byte budget, replication rate, interest radius.
- Particle: spawn budget và batching.
- Render/physics/UI/video: state cache, persistent buffer, tiled light,
  entity-ID GBuffer, mesh-shape cache, layout cache, async decode/AV sync.

CPU mesh discard giữ position/index cho physics/navigation/editor nhưng bỏ
normal/UV; static batching phải chạy trước upload khi bật option này.
Correctness invariant như immutable snapshot và generation-safe entity không
thể tắt.

## 3. CMake

- Debug: symbol, không optimize, MSVC `MDd`.
- Release: optimize, MSVC `MD`.
- Dependency không vendored; dùng vcpkg/package manager/vendor SDK.

```powershell
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake
cmake --build build --config Release --parallel
```

FMOD/irrKlang cần SDK riêng và `FMOD_ROOT_DIR`/`IRRKLANG_ROOT_DIR`. Khi thêm
dependency, ưu tiên package manifest và imported CMake target; không commit
binary/header third-party vào source tree.
