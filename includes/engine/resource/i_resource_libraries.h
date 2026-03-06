#pragma once

#include <string>
#include <memory>
#include <vector>

class Shader;
class Texture;
class Model;
class IAudioSource;
class IAudioEngine;
class Font;
class Skybox;
class UIModel;
enum class UIType;

class IShaderLibrary
{
public:
    virtual ~IShaderLibrary() = default;
    virtual void LoadShader(const std::string& name, const std::string& vsPath, const std::string& fsPath, const std::string& gsPath = "") = 0;
    virtual void UnloadShader(const std::string& name) = 0;
    virtual std::shared_ptr<Shader> GetShader(const std::string& name) = 0;
};

class ITextureLibrary
{
public:
    virtual ~ITextureLibrary() = default;
    virtual void LoadTexture(const std::string& name, const std::string& path, bool async = true) = 0;
    virtual void UnloadTexture(const std::string& name) = 0;
    virtual std::shared_ptr<Texture> GetTexture(const std::string& name) = 0;
};

class IModelLibrary
{
public:
    virtual ~IModelLibrary() = default;
    virtual void LoadModel(const std::string& name, const std::string& path, bool isStatic = false) = 0;
    virtual void LoadModelAsync(const std::string& name, const std::string& path, bool isStatic = false) = 0;
    virtual void UnloadModel(const std::string& name) = 0;
    virtual std::shared_ptr<Model> GetModel(const std::string& name) = 0;
    virtual void CreateUIModel(const std::string& name, UIType type) = 0;
    virtual std::shared_ptr<UIModel> GetUIModel(const std::string& name) = 0;
};

class ISoundLibrary
{
public:
    virtual ~ISoundLibrary() = default;
    virtual void LoadSound(const std::string& name, const std::string& path, IAudioEngine* engine) = 0;
    virtual void UnloadSound(const std::string& name) = 0;
    virtual std::shared_ptr<IAudioSource> GetSound(const std::string& name) = 0;
};

class IFontLibrary
{
public:
    virtual ~IFontLibrary() = default;
    virtual void LoadFont(const std::string& name, const std::string& path, unsigned int fontSize) = 0;
    virtual void UnloadFont(const std::string& name) = 0;
    virtual std::shared_ptr<Font> GetFont(const std::string& name) = 0;
};

class ISkyboxLibrary
{
public:
    virtual ~ISkyboxLibrary() = default;
    virtual void LoadSkybox(const std::string& name, const std::vector<std::string>& faces) = 0;
    virtual void UnloadSkybox(const std::string& name) = 0;
    virtual std::shared_ptr<Skybox> GetSkybox(const std::string& name) = 0;
};
