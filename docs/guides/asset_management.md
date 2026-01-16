# Asset Management

The **ResourceManager** handles loading and retrieving game assets in the AXIS Engine. In most cases, you define these in your `.scene` file, but you can also load them manually via C++.

## 1. Supported Formats
- **Models**: `.fbx`, `.obj`, `.gltf`, `.glb`
- **Textures**: `.png`, `.jpg`, `.tga`
- **Audio**: `.wav`, `.mp3`, `.ogg`
- **Fonts**: `.ttf`, `.otf`
- **Shaders**: Text files (usually `.vs` and `.fs`)

## 2. Loading via Scene File (Recommended)
This is the standard way to load assets for a specific level.

```text
# Shaders
LOAD_SHADER    myshader       shaders/model.vs    shaders/model.fs

# Models
LOAD_MODEL     character      models/char.fbx
LOAD_STATIC_MODEL  house      models/house.obj  # Optimized for non-animated

# Textures (Loaded implicitly by models or for UI)
# Custom textures can be bound in shaders/materials

# Skybox
LOAD_SKYBOX    daylight       right.jpg left.jpg top.jpg bottom.jpg front.jpg back.jpg

# Audio
LOAD_SOUND     bgm            audio/music.mp3

# Particles
LOAD_PARTICLE  smoke          textures/smoke.png
```

## 3. Loading via C++ Code
You can access `ResourceManager` through the `Application` instance.

```cpp
auto& res = m_App->GetResourceManager();

// Load manually
res.LoadModel("sword", "resources/models/sword.fbx");
res.LoadSound("sfx_hit", "resources/audio/hit.wav", m_App->GetSoundManager().GetEngine());

// Retrieve
Model* sword = res.GetModel("sword");
```

## 4. UI Assets
UI elements use `UIModel`. You can create basic shapes programmatically:

```cpp
// Create a colored rectangle quad
res.CreateUIModel("health_bar", UIType::Color);
```
