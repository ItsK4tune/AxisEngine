#pragma once

#include <memory>
#include <resource/resource_manager.h>
#include <scene/scene_manager.h>
#include <audio/sound_player.h>

class ContentService
{
public:
    ContentService(ResourceManager& resources, SceneManager& sceneManager, SoundPlayer& soundPlayer);

    ResourceManager& GetResourceManager() { return m_Resources; }
    SceneManager& GetSceneManager() { return m_SceneManager; }
    SoundPlayer& GetSoundPlayer() { return m_SoundPlayer; }

private:
    ResourceManager& m_Resources;
    SceneManager& m_SceneManager;
    SoundPlayer& m_SoundPlayer;
};
