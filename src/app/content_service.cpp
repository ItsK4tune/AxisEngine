#include <app/content_service.h>

ContentService::ContentService(ResourceManager& resources, SceneManager& sceneManager, SoundPlayer& soundPlayer)
    : m_Resources(resources)
    , m_SceneManager(sceneManager)
    , m_SoundPlayer(soundPlayer)
{
}
