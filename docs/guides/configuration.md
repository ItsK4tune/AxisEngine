# Application & Build Configuration

## 1. Application Configuration

The AXIS Engine is configured via the `Config` block located at the top of your scene files (`.axs`). This allows per-scene configuration of graphics, physics, and window settings seamlessly applied via `ConfigLoader`.

### `.axs` Configuration Structure

```yaml
axis_scene:
  Config:
    title: AXIS Engine
    WINDOW_WIDTH: 1280
    WINDOW_HEIGHT: 720
    WINDOW_MODE: WINDOWED
    WINDOW_MONITOR: 0
    WINDOW_REFRESH_RATE: 60
    VSYNC: 1
    FPS: 120
    SHADOWS: 1
    CULL_FACE: 1 BACK
    DEPTH_TEST: 1 LESS
    ANTIALIASING: TAA
    FRUSTUM: 1
    DISTANCE: 0.0
    INSTANCING: 1
    OCCLUSION_CULLING: 0
    SHADOW_SIZE: 100.0
    SHADOW_FRUSTUM: 1
    SHADOW_DISTANCE: 100.0
    PHYSICS_MODE: 1
    PHYSICS_ASYNC: TRUE
```

### Parameters

- **Window Settings**
    - `WINDOW_WIDTH`, `WINDOW_HEIGHT`: Initial resolution of the window.
    - `WINDOW_MODE`: Window display mode (`WINDOWED`, `FULLSCREEN`, or `BORDERLESS`).
    - `WINDOW_MONITOR`: Index of the monitor to display on (0 = Primary, 1 = Secondary, etc.).
    - `WINDOW_REFRESH_RATE`: Target refresh rate (Hz) for Fullscreen mode (0 = Desktop rate).
    - `VSYNC`: Enable Vertical Sync (`1` or `0`).
    - `FPS`: Maximum frames per second (0 = Unlimited).
    - `iconPath`: Path to the application window icon.

- **Graphics Settings**
    - `SHADOWS`: Shadow rendering mode (default: `1`).
        - `0`: No shadows
        - `1`: Single directional shadow
        - `2`: Multiple directional shadows
    - `CULL_FACE`: Enable/Disable back-face culling (`1 BACK`, `1 FRONT`, or `0`).
    - `FRUSTUM`: Enable/Disable camera frustum culling (`1` or `0`).
    - `OCCLUSION_CULLING`: Enable/Disable hardware occlusion culling (`1` or `0`). Requires `Occlusion` component on entities to be tested.
    - `DISTANCE`: Maximum render distance from camera in world units (default: `0.0` = unlimited). Objects beyond this distance will not be rendered.
    - `DEPTH_TEST`: Enable/Disable depth testing (`1 LESS`, `1 LEQUAL`, etc.).
    - `INSTANCING`: Enable/Disable instance batching for static meshes (`1` or `0`). Batching reduces draw calls but disabling can help with debugging transform issues.
    - `SHADOW_SIZE`: Size of the orthogonal projection for directional light shadows (default: `100.0`).
    - `SHADOW_FRUSTUM`: Enable/Disable culling of objects outside the light's view frustum (default: `1`).
    - `SHADOW_DISTANCE`: Maximum distance (from main camera) at which objects cast shadows (default: `100.0`). Objects further than this will not cast shadows.
    - `ANTIALIASING`: Anti-Aliasing mode (default: `TAA`). Options: `NONE`, `FXAA`, `TAA`.

- **Physics Settings**
    - `PHYSICS_MODE`: Physics simulation mode (default: `1`).
        - `0`: FAST (30Hz, 2 iters)
        - `1`: BALANCED (60Hz, 10 iters)
        - `2`: ACCURATE (120Hz, 40 iters)
    - `physicsAsync`: Enable/Disable asynchronous physics simulation (default: `true`). When enabled, physics calculations run in parallel with rendering for better performance. Disable for deterministic single-threaded behavior.

- **Audio Settings**
    - `audioDevice`: ID or Name of the audio output device (use "default" for system default). Use F2 in-game to see available device IDs.

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
1.  **Headers**: Place `.h` files in `includes/`.
2.  **Static Libs**: Place `.lib` files in `lib/`.
3.  **Dynamic Libs**: Place `.dll` files in `dlls/`.
4.  **CMake**: Update `CMakeLists.txt` to find and link the library.
