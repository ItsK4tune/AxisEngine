# Anti-Aliasing Guide

The Axis Engine supports Anti-Aliasing to smooth jagged edges and improve image stability.

## Supported Modes

| Mode | Description | Pros | Cons |
| :--- | :--- | :--- | :--- |
| **NONE** | No Anti-Aliasing | Fastest performance | Jagged edges, shimmering |
| **FXAA** | Fast Approximate AA | Very low cost, good edge smoothing | Slightly blurs textures |
| **TAA** | Temporal AA | High quality, resolves sub-pixel detail, stable image | Can cause slight ghosting on fast moving objects |

## Configuration

You can enable Anti-Aliasing in two ways:

### 1. Scene File (`.scene`)
Add the `CONFIG ANTIALIASING` command to your scene file:

```
CONFIG ANTIALIASING TAA
// Or
CONFIG ANTIALIASING FXAA
// Or
CONFIG ANTIALIASING NONE
```

### 2. Settings File (`settings.json`)
Add the `antialiasing` key to your configuration file:

```json
{
    "antialiasing": "TAA", // "FXAA", "NONE"
    ...
}
```

## Technical Details

### FXAA (Fast Approximate Anti-Aliasing)
A post-processing shader that analyzes contrast to detect and smooth edges. It does not require additional memory or history.

### TAA (Temporal Anti-Aliasing)
Accumulates samples over multiple frames to resolve detail that a single frame cannot capture.
- Uses **Halton Sequence** jittering for sub-pixel sampling.
- Uses **Velocity Clamping** (via History Neighborhood) to reduce ghosting (re-projection is currently camera-based only).
- Requires a Depth Texture and History Buffer.

## Debugging
If you experience issues:
- Check the console for `[WARNING]` logs regarding invalid configuration.
- Ensure your GPU supports standard OpenGL 3.3+.
- If TAA looks blurry without edge smoothing, ensure your valid `width` and `height` are being passed to the RenderSystem.
