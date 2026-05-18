#pragma once

#include <memory>
#include <string>

class Model;
class UIModel;

enum class UIType;

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
