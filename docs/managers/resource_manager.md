# ResourceManager API

**Include:** `<engine/core/resource_manager.h>`

The `ResourceManager` handles loading, caching, and retrieving assets. It ensures assets are not loaded multiple times.

## Access
Accessible via `m_App->GetResourceManager()`.

## Loading Assets
Note: Most loading is done automatically via `.scene` files or `SceneManager`, but can be done manually.

*   `void LoadTexture(std::string name, std::string path, bool async)`
*   `void LoadModel(std::string name, std::string path, bool isStatic)`
*   `void LoadShader(std::string name, std::string vs, std::string fs)`
*   `void LoadSound(std::string name, std::string path, ISoundEngine* engine)`
*   `void LoadFont(std::string name, std::string path, int fontSize)`

## Retrieving Assets
Returns raw pointers to the loaded resources. Returns `nullptr` if not found.

*   `Texture* GetTexture(const std::string& name)`
*   `Model* GetModel(const std::string& name)`
*   `Shader* GetShader(const std::string& name)`
*   `Animation* GetAnimation(const std::string& name)`
*   `Font* GetFont(const std::string& name)`
*   `ISoundSource* GetSound(const std::string& name)`
*   `Skybox* GetSkybox(const std::string& name)`
*   `UIModel* GetUIModel(const std::string& name)`

## Caching Behavior
The ResourceManager uses **name-based caching** for resources:
- **Models**: Each unique name loads a separate instance, even if pointing to the same file. This allows different textures/states for models sharing the same mesh data.
- **Textures/Shaders**: Shared by path to reduce memory usage.

> **Example**: Loading `plane.fbx` as both "planeModel" and "planeVideoModel" creates two independent Model instances. Modifying textures on one won't affect the other.

## Hot Reload
The resource manager runs a background thread to check for file changes (Shaders, Textures) and reloads them automatically in Debug mode.
