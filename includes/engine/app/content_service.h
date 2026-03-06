#pragma once

#include <audio/sound_player.h>
#include <memory>
#include <resource/resource_manager.h>
#include <scene/scene_manager.h>

class ContentService
{
public:
    ContentService();
    ~ContentService() = default;

    void Init(EngineContext ctx);

    ResourceManager* GetResourceManager() { return m_Resources; }
    SceneManager* GetSceneManager() { return m_SceneManager; }
    SoundPlayer* GetSoundPlayer() { return m_SoundPlayer; }

private:
    ResourceManager* m_Resources = nullptr;
    SceneManager* m_SceneManager = nullptr;
    SoundPlayer* m_SoundPlayer = nullptr;
};
