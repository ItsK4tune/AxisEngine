#include <audio/sound_player.h>
#include <utils/logger.h>

SoundPlayer::SoundPlayer(IAudioEngine* engine)
    : m_Engine(engine)
{
}

SoundPlayer::~SoundPlayer()
{
}

std::shared_ptr<ISound> SoundPlayer::Play2D(const std::string& path, bool loop)
{
    if (m_Engine)
        return m_Engine->Play2D(path, loop);
    return nullptr;
}

std::shared_ptr<ISound> SoundPlayer::Play3D(const std::string& path, glm::vec3 pos, bool loop)
{
    if (m_Engine)
        return m_Engine->Play3D(path, pos, loop);
    return nullptr;
}

void SoundPlayer::UpdateListener(glm::vec3 position, glm::vec3 lookDir)
{
    if (m_Engine)
    {
        m_Engine->SetListenerPosition(position, lookDir);
    }
}

void SoundPlayer::SetGlobalVolume(float volume)
{
    if (m_Engine)
        m_Engine->SetGlobalVolume(volume);
}

void SoundPlayer::StopAll()
{
    if (m_Engine)
        m_Engine->StopAllSounds();
}
