#pragma once

class Scene;
class IPhysicsWorld;
class SceneManager;
class ResourceManager;
class SoundPlayer;

struct WorldContext
{
    Scene&           scene;
    IPhysicsWorld&   physics;
    SceneManager&    sceneManager;
    ResourceManager& resources;
    SoundPlayer&     sound;
};
