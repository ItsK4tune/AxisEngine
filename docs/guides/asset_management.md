# Asset Management

The **ResourceManager** handles loading and retrieving game assets in the AXIS Engine. Assets can be defined in a `.axs` scene file (recommended) or loaded manually via C++.

## 1. Supported Formats
- **Models**: `.fbx`, `.obj`, `.gltf`, `.glb`
- **Animations**: `.fbx`, `.dae` (requires an associated rigged Model)
- **Textures**: `.png`, `.jpg`, `.tga`
- **Audio**: `.wav`, `.mp3`, `.ogg`
- **Fonts**: `.ttf`, `.otf`
- **Shaders**: Text files (usually `.vs` and `.fs`)

## 2. Loading via Scene File (Recommended)
This is the standard way to load assets for a specific scene using the AXS (YAML-like) format.

```yaml
axis_scene:
  Resources:
    Shader:
      Name: myShader
      VS: shaders/model.vs
      FS: shaders/model.fs
      
    Model:
      Name: character
      Path: models/char.fbx
      Static: 0  # Set to 1 for optimized non-animated static geometry
      
    Animation:
      Name: runAnimation
      Path: animations/run.fbx
      Model: character # Must match a loaded Model name
      
    Texture:
      Name: boxAlbedo
      Path: textures/box_albedo.png
      
    Font:
      Name: mainFont
      Path: fonts/roboto.ttf
      Size: 24

    Sound:
      Name: bgMusic
      Path: audio/music.ogg

    Skybox:
      Name: daylight
      Right: skybox/right.jpg
      Left: skybox/left.jpg
      Top: skybox/top.jpg
      Bottom: skybox/bottom.jpg
      Front: skybox/front.jpg
      Back: skybox/back.jpg
```

## 3. Loading via C++ Code
You can access `ResourceManager` through the `Application` instance.

```cpp
auto& res = m_App->GetResourceManager();

// Load manually
res.LoadModel("sword", "resources/models/sword.fbx");
res.LoadAnimation("swing", "resources/animations/swing.fbx", "sword");
res.LoadSound("sfx_hit", "resources/audio/hit.wav", m_App->GetSoundManager().GetEngine());

// Retrieve
std::shared_ptr<Model> sword = res.GetModel("sword");
std::shared_ptr<Animation> swingAnim = res.GetAnimation("swing");
```

## 4. UI Assets
UI elements use `UIModel`. You can create basic shapes programmatically:

```cpp
auto& res = m_App->GetResourceManager();

// Create a colored rectangle quad
res.CreateUIModel("health_bar", UIType::Color);

// Create an image/texture quad
res.CreateUIModel("avatar_icon", UIType::Image);
```

## 5. Unloading Assets
Assets loaded via `.axs` scenes are automatically unloaded by the `SceneManager` when their parent scenes are unloaded (using reference counting to prevent freeing resources still used by other scenes).

To manually unload resources loaded via C++:
```cpp
res.UnloadModel("sword");
res.UnloadTexture("boxAlbedo");
res.UnloadShader("myShader");
res.UnloadFont("mainFont");
res.UnloadSound("bgMusic");
res.UnloadAnimation("swing");
res.UnloadSkybox("daylight");
```
