#pragma once

#include <map>
#include <string>
#include <memory>
#include <iostream>
#include <irrKlang/irrKlang.h>

#include <engine/graphic/shader.h>
#include <engine/graphic/model.h>
#include <engine/graphic/ui_model.h>
#include <engine/graphic/animation.h>
#include <engine/graphic/font.h>
#include <engine/graphic/skybox.h>

class ResourceManager
{
public:
    ~ResourceManager();

    void LoadShader(const std::string &name, const std::string &vsPath, const std::string &fsPath);
    void LoadModel(const std::string &name, const std::string &path, bool isStatic = false);
    void LoadAnimation(const std::string &name, const std::string &path, const std::string &modelName);
    void LoadFont(const std::string &name, const std::string &path, unsigned int fontSize);
    void LoadSound(const std::string &name, const std::string &path, irrklang::ISoundEngine *engine);
    void LoadSkybox(const std::string &name, const std::vector<std::string> &faces);
    void CreateUIModel(const std::string &name, UIType type);

    Shader *GetShader(const std::string &name);
    Model *GetModel(const std::string &name);
    Animation *GetAnimation(const std::string &name);
    Font *GetFont(const std::string &name);
    irrklang::ISoundSource *GetSound(const std::string &name);
    Skybox *GetSkybox(const std::string &name);
    UIModel *GetUIModel(const std::string &name);

    void ClearResource();

private:
    std::map<std::string, std::unique_ptr<Shader>> shaders;
    std::map<std::string, std::unique_ptr<Model>> models;
    std::map<std::string, std::unique_ptr<Animation>> animations;
    std::map<std::string, std::unique_ptr<Font>> fonts;
    std::map<std::string, std::unique_ptr<UIModel>> uiModels;
    std::map<std::string, irrklang::ISoundSource *> sounds;
    std::map<std::string, std::unique_ptr<Skybox>> skyboxes;
};