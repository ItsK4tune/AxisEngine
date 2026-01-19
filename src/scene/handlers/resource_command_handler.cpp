#include <scene/handlers/resource_command_handler.h>
#include <resource/resource_manager.h>
#include <audio/sound_manager.h>
#include <utils/filesystem.h>
#include <iostream>
#include <vector>

namespace SceneHandlers
{
    void ResourceCommandHandler::HandleLoadShader(std::stringstream& ss, ResourceManager& res)
    {
        std::string name, vs, fs;
        ss >> name >> vs >> fs;
        res.LoadShader(name, vs, fs);
    }

    void ResourceCommandHandler::HandleLoadModel(std::stringstream& ss, ResourceManager& res, bool isStatic)
    {
        std::string name, path;
        ss >> name >> path;
        res.LoadModel(name, path, isStatic);
    }

    void ResourceCommandHandler::HandleLoadAnimation(std::stringstream& ss, ResourceManager& res)
    {
        std::string name, modelName, path;
        ss >> name >> modelName >> path;
        res.LoadAnimation(name, path, modelName);
    }

    void ResourceCommandHandler::HandleLoadFont(std::stringstream& ss, ResourceManager& res)
    {
        std::string name, path;
        int size;
        ss >> name >> path >> size;
        res.LoadFont(name, path, size);
    }

    void ResourceCommandHandler::HandleLoadSound(std::stringstream& ss, ResourceManager& res, SoundManager& sound)
    {
        std::string name, path;
        ss >> name >> path;
        res.LoadSound(name, path, sound.GetEngine());
    }

    void ResourceCommandHandler::HandleLoadSkybox(std::stringstream& ss, ResourceManager& res)
    {
        std::string name;
        ss >> name;

        std::vector<std::string> faces(6);
        for (int i = 0; i < 6; i++)
        {
            std::string path;
            ss >> path;
            faces[i] = FileSystem::getPath(path);
        }

        res.LoadSkybox(name, faces);
    }

    void ResourceCommandHandler::HandleLoadParticle(std::stringstream& ss, ResourceManager& res)
    {
        std::string name, path;
        ss >> name >> path;
        res.LoadTexture(name, path);
    }
}
