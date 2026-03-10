#include <core/logic/content_service.h>

ContentService::ContentService() {}

void ContentService::Initialize(EngineContext ctx)
{
    m_Resources = ctx.resources;
    m_SceneManager = ctx.sceneManager;
    m_SoundPlayer = ctx.soundPlayer;
}
