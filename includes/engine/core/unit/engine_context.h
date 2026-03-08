#pragma once

class IOHandler;
class IPhysicsWorld;
class ResourceManager;
class RuntimeCore;
class Scene;
class SceneManager;
class SoundPlayer;
class SystemManager;

struct EngineContext
{
    Scene*           scene         = nullptr;
    IPhysicsWorld*   physics       = nullptr;
    ResourceManager* resources     = nullptr;
    SceneManager*    sceneManager  = nullptr;
    SoundPlayer*     soundPlayer   = nullptr;
    IOHandler*       io            = nullptr;
    SystemManager*   systems       = nullptr;
    RuntimeCore*     runtime       = nullptr;

    bool IsValid() const
    {
        return scene && physics && resources && sceneManager
            && soundPlayer && io && systems && runtime;
    }
};