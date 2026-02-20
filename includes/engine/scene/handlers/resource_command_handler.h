#pragma once

#include <sstream>
#include <string>

class ResourceManager;
class SoundPlayer;

namespace SceneHandlers
{
namespace ResourceCommandHandler
{
    void HandleLoadShader(std::stringstream &ss, ResourceManager &res);
    void HandleLoadTexture(std::stringstream &ss, ResourceManager &res);
    void HandleLoadModel(std::stringstream &ss, ResourceManager &res, bool isStatic = false);
    void HandleLoadAnimation(std::stringstream &ss, ResourceManager &res);
    void HandleLoadFont(std::stringstream &ss, ResourceManager &res);
    void HandleLoadSound(std::stringstream &ss, ResourceManager &res, SoundPlayer &sound);
    void HandleLoadSkybox(std::stringstream &ss, ResourceManager &res);
    void HandleLoadParticle(std::stringstream &ss, ResourceManager &res);
}
}
