#pragma once

#include <sstream>
#include <string>

// Forward declarations
class ResourceManager;
class SoundManager;

namespace SceneHandlers
{
    class ResourceCommandHandler
    {
    public:
        static void HandleLoadShader(std::stringstream& ss, ResourceManager& res);
        static void HandleLoadModel(std::stringstream& ss, ResourceManager& res, bool isStatic);
        static void HandleLoadAnimation(std::stringstream& ss, ResourceManager& res);
        static void HandleLoadFont(std::stringstream& ss, ResourceManager& res);
        static void HandleLoadSound(std::stringstream& ss, ResourceManager& res, SoundManager& sound);
        static void HandleLoadSkybox(std::stringstream& ss, ResourceManager& res);
        static void HandleLoadParticle(std::stringstream& ss, ResourceManager& res);
    };
}
