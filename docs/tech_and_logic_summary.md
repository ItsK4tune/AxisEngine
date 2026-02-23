# Axis Engine: Technology & Logic Implementation Summary

This document serves as a high-level summary of the technologies, architectural patterns, and logic implemented within the AXIS Engine project.

## Core Technologies
*   **Language:** C++17
*   **Graphics API:** OpenGL 3.3+ (Abstracted via backend interfaces `IGraphicsContext`, `IRenderTargetManager`, allowing future Vulkan/DirectX expansion).
*   **Windowing & Input:** GLFW (Window creation, OS-level input capture).
*   **Math Library:** GLM (Vectors, Matrices, Quaternions).
*   **ECS Framework:** EnTT (Entity-Component-System architecture for memory-efficient and fast entity data management).
*   **Physics Engine:** Bullet Physics (Abstracted via `IPhysicsWorld`, allowing easy swapping. Currently implements rigidbodies, constraints, and compound shapes).
*   **Audio Engine:** IrrKlang (Abstracted via `ISound`, `ISoundEngine`).
*   **Asset Loading:**
    *   `Assimp` (3D Model Loading: OBJ, FBX, GLTF with animations).
    *   `stb_image` (Texture loading: PNG, JPG).
    *   `FreeType` (Font loading and text rendering).
*   **Video Decoding:** FFmpeg (libavcodec, libavformat, libswscale) for asynchronous MP4 playback onto OpenGL textures.

## Architectural Patterns

1.  **Entity-Component-System (ECS):**
    *   Strict separation of data (Components) and logic (Systems).
    *   Examples: `TransformComponent`, `RigidBodyComponent` act purely as data pods. `PhysicsSystem`, `RenderSystem` iterate over these components in tight loops.
2.  **Interface Segregation / Backend Abstraction:**
    *   Core systems like Graphics, Physics, and Audio do not depend directly on OpenGL, Bullet, or IrrKlang.
    *   Instead, they interact through pure virtual interfaces (`IPhysicsWorld`, `IAudioBackend`). Connectors like `BulletPhysicsWorld` or `OpenGLContext` implement these.
3.  **Data-Oriented Scene Serialization:**
    *   Scenes are defined in a custom `.axs` (Axis Scene) format.
    *   Parsed using a lightweight custom `YAMLNode` hierarchy instead of heavy JSON/XML parsers to maintain engine independence and performance.
4.  **Observer / Event System:**
    *   Input events (Keyboard, Mouse, Gamepad) and Engine Events (Window Resize, Filesystem changes for Hot Reload) are distributed using delegates to avoid tight coupling.
5.  **Singleton / Service Locator (Via Application):**
    *   Core managers (`ResourceManager`, `SceneManager`, `SoundPlayer`) are instantiated once in `Application` and passed down or accessed safely as services.
6.  **Scriptable Object Pattern:**
    *   Gameplay code is written as natively compiled C++ classes deriving from `Scriptable`.
    *   These scripts are attached to entities and hook into the engine's `OnUpdate`, `OnCollisionEnter`, and `OnTriggerEnter` lifecycle.

## Major Logic & Systems

### Rendering Pipeline
*   **Forward Rendering Pipeline:** Fast and simple structure.
*   **Frustum Culling:** Checks AABBs of models against the camera's view frustum to drop off-screen renders early.
*   **Instance Batching:** Auto-groups static meshes with the same model/shader and renders them using `glDrawElementsInstanced` (drastically reducing draw calls).
*   **Deferred Shadows:** Renders depth maps for up to 4 lights (Directional, Point, Spot) into a texture atlas before the main pass.
*   **Post-Processing & Bloom:** Uses ping-pong FBOs to extract bright areas, blur them, and additively blend them over the final frame.
*   **Particle System:** CPU-simulated but GPU-rendered using instanced quads.

### Physics Automation
*   **Auto-Sync Integration:** The `PhysicsSystem` automatically mirrors Bullet's `btRigidBody` transforms back to the EnTT `TransformComponent` every fixed update tick.
*   **Collision Callbacks:** Internally resolves Bullet manifolds and translates them into `OnCollisionEnter/Stay/Exit` callbacks routed directly into user `Scriptable` components.
*   **Constraint Support:** Supports Hinge, Point2Point, and Fixed joints defined via C++ logic.

### Resource & Asset Management
*   **Hot Reloading:** `ResourceWatcher` utilizes `std::filesystem` to monitor shaders and scripts. If a `.fs` or `.vs` file is saved, the engine asynchronously recompiles it and swaps it in without restarting.
*   **Caching Pool:** `ResourceManager` prevents duplicate memory loading by caching textures and models in unordered maps.
*   **Asynchronous Loading:** Large assets (like models via Assimp) are pushed to a `std::thread` pool to avoid stalling the main rendering thread.

### Application Lifecycle
*   **Fixed Timestep / Catchup Loop:** The engine measures exact delta time but runs the Physics simulation at strict intervals (`fixedDt`). If rendering lags behind, physics steps multiple times to maintain deterministic collisions, preventing "tunneling" logic errors.
