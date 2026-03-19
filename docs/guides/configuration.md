# Application & Build Configuration

## 1. Application Configuration

The AXIS Engine is configured via the `Config` block located at the top of your scene files (`.axs`). This allows per-scene configuration of graphics, physics, and window settings seamlessly applied via `ConfigLoader`.

### `.axs` Configuration Structure

```yaml
axis_scene:
  Config:
    WINDOW_WIDTH: 1920
    WINDOW_HEIGHT: 1080
    WINDOW_MODE: BORDERLESS_FULLSCREEN
    WINDOW_MONITOR: 0
    WINDOW_REFRESH_RATE: 60
    VSYNC: 1
    FPS: 120
    GRAPHICS_API: OPENGL
    PHYSICS_ENGINE: BULLET
    AUDIO_ENGINE: IRRKLANG
    SHADOWS: 1
    CULL_FACE: 1 BACK
    DEPTH_TEST: 1 LESS
    RENDER_ORDER: 0
    ANTIALIASING: TAA
    FRUSTUM: 1
    DISTANCE: 0.0
    INSTANCING: 1
    OCCLUSION_CULLING: 0
    SHADOW_SIZE: 100.0
    SHADOW_FRUSTUM: 1
    SHADOW_DISTANCE: 100.0
    FILTER_LAYER: -1
    PHYSICS_MODE: 1
    PHYSICS_MODE: 1
```

### Comprehensive Configuration Guide

AXIS Engine uses a highly flexible configuration system to tune the **Hybrid Modular** architecture. Settings can be defined in global config files or overridden per-scene in the `.axs` format.

---

## ⚙️ 1. Engine & System Global

Settings that control the fundamental behavior of the engine and hardware resource allocation.

- **`LOG_LEVEL`**: Console verbosity control.
    - Values: `NONE`, `MINIMAL`, `VERBOSE`, `DEBUG`.
- **`JOB_THREADS`**: Worker pool size for multithreaded systems.
    - `-1`: Auto-detect hardware concurrency.
- **`TIME_SCALE`**: Global simulation multiplier (0.5 = Slow motion, 2.0 = Fast forward).
- **`ASYNC_RESOURCES`**: Enables background asset loading (`1` or `0`).

---

## 🖼️ 2. Graphics & Post-Processing (Modular)

These settings apply to the `IGraphicsContext` and subsequent rendering providers (OpenGL/Vulkan).

### Display & Resolution
- **`WINDOW_MODE`**: Display strategy.
    - `WINDOWED`, `FULLSCREEN`, `BORDERLESS`, `BORDERLESS_FULLSCREEN`.
- **`RENDER_SCALE`**: Internal resolution multiplier (e.g., `0.5` for performance, `2.0` for supersampling).
- **`VSYNC`**: Frame synchronization (`1` or `0`).
- **`FPS`**: Frame rate limiter (`0` for unlimited).

### Quality & AA
- **`ANTIALIASING`**: Algorithm selection (`NONE`, `FXAA`, `TAA`).
- **`MSAA`**: Multisample samples (`2`, `4`, `8`, `16`).
- **`ANISOTROPY`**: Texture filtering quality (up to `16.0`).

### PBR & Screen Space Effects
- **`HDR_ENABLED`**: Enables high dynamic range rendering buffers.
- **`TONEMAPPING`**: Algorithm selection (`ACES`, `REINHARD`, `NONE`).
- **`BLOOM_ENABLED`**: Toggle glow effects.
    - Sub-params: `BLOOM_INTENSITY`, `BLOOM_THRESHOLD`, `BLOOM_RADIUS`.
- **`GAMMA`**: Color correction (Default: `2.2`).
- **`EXPOSURE`**: Virtual camera exposure level.
- **`SKYBOX_INTENSITY`**: Ambient light contribution from environment maps.

---

## 🌑 3. Advanced Shadow Mapping

Configures the `IShadowManager` which handles depth projection and filtering.

- **`SHADOW_RESOLUTION`**: Accuracy of shadow maps (e.g., `1024`, `2048`, `4096`).
- **`SHADOW_SOFTNESS`**: PCF filter radius.
    - `0` (Hard), `1` (3x3), `2` (5x5).
- **`SHADOW_BIAS`**: Depth offset to prevent "Shadow Acne" artifacts.
- **`SHADOW_SIZE`**: Orthographic projection width for directional lights.
- **`SHADOW_FRUSTUM`**: Enables light-space view culling.
- **`SHADOW_DISTANCE`**: Max radius for shadow casting.

---

## 🧊 4. Physics Simulation (Modular)

Configuration for the `IPhysicsWorld` abstraction, applicable to Bullet or PhysX backends.

- **`GRAVITY`**: Vector force `X Y Z` (e.g., `0 -9.81 0`).
- **`PHYSICS_MODE`**: Simulation precision presets.
    - `FAST`: 30Hz simulation, minimal iterations.
    - `BALANCED`: 60Hz simulation (Default).
    - `ACCURATE`: 120Hz simulation, high collision iterations.
- **`CCD_ENABLED`**: Continuous Collision Detection for high-speed entities.
- **`SOLVER_ITERATIONS`**: Internal precision of the constraint solver (e.g., `10`).
- **`MAX_SUBSTEPS`**: Max delta-time catch-up steps.

---

## 🔊 5. Audio & Input

- **`VOLUME`**: Global master volume (0.0 to 1.0).
- **`MOUSE_SENSITIVITY`**: Global X/Y look speed.
- **`MOUSE_INVERT_Y`**: Toggle vertical mouse axis inversion.

---

## 🔌 6. Backend Selection

Directly selects the module implementation for each interface.

| Key | Description | Supported Modules |
|:---|:---|:---|
| `GRAPHICS_API` | Core rendering implementation | `OPENGL`, `VULKAN`, `DIRECTX` |
| `PHYSICS_ENGINE` | Simulation implementation | `BULLET`, `PHYSX` |
| `AUDIO_ENGINE` | Sound processing implementation | `IRRKLANG`, `FMOD`, `OPENAL` |
| `RENDER_PATH` | Geometry processing strategy | `FORWARD`, `DEFERRED` |

## 2. CMake Build System

The project uses CMake for cross-platform build generation.

### Build Types
- **Debug**: Includes symbols, no optimization. Uses `MDd` runtime library.
- **Release**: Optimized. Uses `MD` runtime library.

### DLL Management
The build system automatically copies required DLLs (from `dlls/`) to the output binary directory (`bin/` or `build/Debug/`).

> **Important**: When adding a new library, ensure its `.dll` is placed in the `dlls/` folder so it can be found at runtime.

### Adding Libraries
To integrate a new third-party library:
1.  **Headers**: Place `.h` files in `include/`.
2.  **Static Libs**: Place `.lib` files in `lib/`.
3.  **Dynamic Libs**: Place `.dll` files in `dlls/`.
4.  **CMake**: Update `CMakeLists.txt` to find and link the library.
