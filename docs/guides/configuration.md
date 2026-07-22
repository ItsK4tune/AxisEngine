# Application & Build Configuration

## 1. Application Configuration

AXIS Engine application settings live in a standalone `axis_config` `.axs` file. Scene files use the separate
`axis_scene` schema; putting a `Config` block inside a scene is not a supported override mechanism.

### `.axs` Configuration Structure

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
    CULL_FACE: 1
    DEPTH_TEST: 1
    STENCIL_TEST: 1
    RENDER_ORDER: 0
    ANTIALIASING: TAA
    FRUSTUM: 1
    SPATIAL_CULLING: AUTO
    DISTANCE: 0.0
    INSTANCING: 1
    OCCLUSION_CULLING: 0
    SHADOW_SIZE: 100.0
    SHADOW_FRUSTUM: 1
    SHADOW_DISTANCE: 100.0
    FILTER_LAYER: 4294967295
    PHYSICS_MODE: BALANCED
    STRICT_ASSET_LOADING: 0
    AMBIENT_INTENSITY: 1.0
    UI_REFERENCE_SIZE: 1920 1080
    OPT_RESOURCE_UPLOAD_BUDGET: 1
    OPT_MAX_MODEL_UPLOADS_PER_FRAME: 2
    OPT_MAX_TEXTURE_UPLOADS_PER_FRAME: 4
    OPT_DISCARD_CPU_MESH_DATA_AFTER_UPLOAD: 0
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
- **`STRICT_ASSET_LOADING`**: Disables debug fallback assets for shaders, textures, and models when set to `1`; failed loads return failure/null instead of checkerboard/capsule fallbacks.

---

## 🖼️ 2. Graphics & Post-Processing (Modular)

These settings apply to the `IGraphicsContext` and rendering provider compiled into the build.

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
- **`AMBIENT_INTENSITY`**: Multiplier applied to light ambient terms.
- **`UI_REFERENCE_WIDTH` / `UI_REFERENCE_HEIGHT`**: Reference canvas size used by UI rendering and input scaling.
- **`UI_REFERENCE_SIZE`**: Shorthand form that accepts width and height in one line.

### Spatial culling
- **`FRUSTUM`**: Enables view-frustum culling.
- **`SPATIAL_CULLING`**: Selects the render-candidate backend: `AUTO`, `LINEAR`, or `OCTREE`.
  `AUTO` measures both paths periodically and uses hysteresis plus scene churn/candidate ratios to keep the faster path.

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

Configuration for the `IPhysicsWorld` abstraction. The current build provides the Bullet backend.

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

- **`VOLUME`**: Global master volume in percent (0 to 100).
- **`AUDIO_CAPTURE_ENABLED`**: Starts the configured microphone endpoint during application initialization.
- **`AUDIO_CAPTURE_DEVICE`**: Stable endpoint id; an empty value selects the system default.
- **`AUDIO_CAPTURE_INPUT_VOLUME` / `AUDIO_CAPTURE_NOISE_GATE` / `AUDIO_CAPTURE_GAIN`**: Input pre-amplification, gate threshold, and post-gate voice gain.
- **`AUDIO_CAPTURE_ATTACK_SECONDS` / `AUDIO_CAPTURE_RELEASE_SECONDS` / `AUDIO_CAPTURE_PEAK_DECAY_SECONDS`**: Level-envelope response.
- **`AUDIO_CAPTURE_CALIBRATION_SECONDS`**: Ambient-noise calibration window.
- **`AUDIO_CAPTURE_PULSE_THRESHOLD` / `AUDIO_CAPTURE_PULSE_COOLDOWN` / `AUDIO_CAPTURE_PULSE_DURATION`**: Pulse detection and lifetime controls.
- **`MOUSE_SENSITIVITY`**: Global X/Y look speed.
- **`MOUSE_INVERT_Y`**: Toggle vertical mouse axis inversion.
- **`GAMEPAD_DEAD_ZONE`**: Per-axis dead zone in the range `0.0-0.99` (default `0.15`).

---

## 🔌 6. Backend Selection

Directly selects the module implementation for each interface.

| Key | Description | Supported Modules |
|:---|:---|:---|
| `GRAPHICS_API` | Core rendering implementation | `OPENGL` |
| `PHYSICS_ENGINE` | Simulation implementation | `BULLET` |
| `AUDIO_ENGINE` | Sound processing implementation | `NULL`, `IRRKLANG`, `FMOD` |

Known serialized values such as `VULKAN`, `DIRECTX`, `PHYSX`, and `OPENAL` remain parseable for source compatibility. Runtime validation reports and replaces an unavailable backend when the application uses built-in providers; a matching custom `AppBuilder` factory preserves the requested enum and receives it unchanged. This release only offers OpenGL and Bullet through the CMake selectors. Unknown backend text is rejected while the current value is retained.

## 7. Runtime optimization policy

All of these values are available in the editor's **Runtime Optimizations** settings section and apply live through
`ConfigChangedEvent::Optimization`:

- `OPT_RESOURCE_HOT_RELOAD`, `OPT_RESOURCE_UPLOAD_BUDGET`, `OPT_MAX_MODEL_UPLOADS_PER_FRAME`,
  `OPT_MAX_TEXTURE_UPLOADS_PER_FRAME`, `OPT_DISCARD_CPU_MESH_DATA_AFTER_UPLOAD`,
  `OPT_COMPRESSED_TEXTURE_LOADING`
- `OPT_STREAMING_UPDATE_THROTTLING`, `OPT_STREAMING_CHECK_INTERVAL`
- `OPT_REFLECTION_CAPTURE_BUDGET`, `OPT_MAX_REFLECTION_PROBE_FACES_PER_FRAME`,
  `OPT_MAX_PLANAR_REFLECTION_CAPTURES_PER_FRAME`
- `OPT_SHADOW_PARALLEL_BUILD`, `OPT_SHADOW_PARALLEL_THRESHOLD`
- `OPT_ANIMATION_PARALLEL_EVALUATION`, `OPT_ANIMATION_PARALLEL_THRESHOLD`
- `OPT_NAVIGATION_SPATIAL_HASH`, `OPT_NAVIGATION_AGENT_CELL_SIZE`, `OPT_NAVIGATION_ASYNC_PATHFINDING`,
  `OPT_NAVIGATION_MAX_PATH_REQUESTS_PER_FRAME`, `OPT_NAVMESH_REBUILD_BUDGET`,
  `OPT_MAX_NAVMESH_REBUILDS_PER_FRAME`, `OPT_NAVIGATION_DIRTY_TILES`,
  `OPT_NAVIGATION_NAVMESH_TILE_SIZE`, `OPT_NAVIGATION_MAX_DIRTY_TILES_PER_FRAME`
- `OPT_NETWORK_BATCHING`, `OPT_NETWORK_MAX_EVENTS_PER_UPDATE`, `OPT_NETWORK_MAX_EVENT_PROCESSING_MS`,
  `OPT_NETWORK_MAX_BYTES_PER_UPDATE`, `OPT_NETWORK_REPLICATION`, `OPT_NETWORK_REPLICATION_RATE_HZ`,
  `OPT_NETWORK_INTEREST_RADIUS`
- `OPT_PARTICLE_SPAWN_BUDGET`, `OPT_PARTICLE_MAX_SPAWN_PER_FRAME`, `OPT_PARTICLE_BATCHING`
- `OPT_RENDER_STATE_CACHE`, `OPT_PERSISTENT_MAPPED_BUFFERS`, `OPT_TILED_LIGHT_CULLING`,
  `OPT_TILED_LIGHT_TILE_SIZE`, `OPT_GBUFFER_ENTITY_ID`,
  `OPT_PHYSICS_MESH_SHAPE_CACHE`, `OPT_UI_LAYOUT_CACHE`,
  `OPT_VIDEO_ASYNC_DECODE`, `OPT_VIDEO_DECODE_QUEUE_SIZE`, `OPT_VIDEO_AV_SYNC_THRESHOLD`,
  `OPT_VIDEO_LOAD_RETRY_SECONDS`

CPU mesh discard keeps positions and indices for physics, navigation, and editor previews, but removes normals/UVs and
other interleaved render attributes. Explicit static batching must therefore run before the model upload when this
option is enabled. It defaults off for compatibility.

Correctness mechanisms such as immutable service/config snapshots, generation-safe entity indices, and nested job waits
are intentionally not toggleable.

## 2. CMake Build System

The project uses CMake for cross-platform build generation.

### Build Types
- **Debug**: Includes symbols, no optimization. Uses `MDd` runtime library.
- **Release**: Optimized. Uses `MD` runtime library.

### Dependency Management
Third-party libraries are intentionally not vendored in this repository. Install them with the OS package manager, vcpkg, Homebrew, or a vendor SDK, then expose them through CMake prefixes/toolchains.

On Windows, the recommended path is vcpkg manifest mode:

```powershell
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake
cmake --build build --config Debug
```

Commercial SDKs are not part of the manifest. Use `-DAXIS_AUDIO_BACKEND=FMOD -DFMOD_ROOT_DIR=<sdk>` or `-DAXIS_AUDIO_BACKEND=IrrKlang -DIRRKLANG_ROOT_DIR=<sdk>` when those SDKs are installed.

### Adding Libraries
To integrate a new third-party library:
1. Add it to the package manifest or document the system package/SDK requirement.
2. Add a focused `Find<Package>.cmake` only if the package does not provide a reliable CMake config.
3. Link an imported target from CMake; do not commit third-party `.lib`, `.dll`, `.h`, `.c`, or `.cpp` files into the source tree.
