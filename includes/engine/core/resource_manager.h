#pragma once

#include <map>
#include <string>
#include <memory>
#include <iostream>

#include <engine/graphic/shader.h>
#include <engine/graphic/model.h>
#include <engine/graphic/ui_model.h>
#include <engine/graphic/animation.h>

class ResourceManager
{
public:
    void LoadShader(const std::string &name, const std::string &vsPath, const std::string &fsPath);
    void LoadModel(const std::string &name, const std::string &path);
    void LoadAnimation(const std::string &name, const std::string &path, const std::string &modelName);
    void CreateUIModel(const std::string &name, UIType type);

    Shader *GetShader(const std::string &name);
    Model *GetModel(const std::string &name);
    Animation *GetAnimation(const std::string &name);
    UIModel *GetUIModel(const std::string &name);

private:
    std::map<std::string, std::unique_ptr<Shader>> shaders;
    std::map<std::string, std::unique_ptr<Model>> models;
    std::map<std::string, std::unique_ptr<Animation>> animations;
    std::map<std::string, std::unique_ptr<UIModel>> uiModels;
};