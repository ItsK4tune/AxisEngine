#include <audio/sound_manager.h>
#include <utils/logger.h>
#include <utils/irrKlang_glm_helpers.h>

SoundManager::~SoundManager()
{
    if (m_Engine)
        m_Engine->drop();
}

void SoundManager::Init()
{
    m_Engine = createIrrKlangDevice(ESOD_AUTO_DETECT, ESEO_MULTI_THREADED | ESEO_LOAD_PLUGINS | ESEO_USE_3D_BUFFERS);
    if (!m_Engine)
    {
        LOGGER_ERROR("SoundManager") << "Could not startup irrKlang engine";
    }
}

void SoundManager::Play2D(const char *path, bool loop)
{
    if (m_Engine)
        m_Engine->play2D(path, loop);
}

ISound *SoundManager::Play2D(ISoundSource *source, bool loop)
{
    if (m_Engine && source)
        return m_Engine->play2D(source, loop);
    return nullptr;
}

ISound *SoundManager::Play3D(ISoundSource *source, glm::vec3 pos, bool loop)
{
    if (m_Engine && source)
        return m_Engine->play3D(source, IrrKlangGLMHelpers::convert(pos), loop);
    return nullptr;
}

void SoundManager::UpdateListener(glm::vec3 position, glm::vec3 lookDir, glm::vec3 up)
{
    if (m_Engine)
    {
        m_Engine->setListenerPosition(IrrKlangGLMHelpers::convert(position), IrrKlangGLMHelpers::convert(lookDir), vec3df(0, 0, 0), IrrKlangGLMHelpers::convert(up));
    }
}

void SoundManager::SetVolume(float volume)
{
    if (m_Engine)
        m_Engine->setSoundVolume(volume);
}

void SoundManager::SetVolume(ISoundSource *source, float volume)
{
    if (source)
    {
        source->setDefaultVolume(volume);
    }
}

void SoundManager::Stop(ISoundSource *source)
{
    if (m_Engine && source)
    {
        m_Engine->stopAllSoundsOfSoundSource(source);
    }
}

void SoundManager::StopAll()
{
    if (m_Engine)
        m_Engine->stopAllSounds();
}

std::vector<DeviceInfo> SoundManager::GetAllDevices() const
{
    std::vector<DeviceInfo> devices;
    ISoundDeviceList *deviceList = createSoundDeviceList();
    if (deviceList)
    {
        for (int i = 0; i < deviceList->getDeviceCount(); ++i)
        {
            DeviceInfo info;
            info.id = deviceList->getDeviceID(i);
            info.name = deviceList->getDeviceDescription(i);
            info.type = DeviceType::AudioOutput;
            info.isDefault = (i == 0);
            devices.push_back(info);
        }
        deviceList->drop();
    }
    return devices;
}

DeviceInfo SoundManager::GetCurrentDevice() const
{
    DeviceInfo info;
    info.type = DeviceType::AudioOutput;
    info.name = "Current Sound Device";
    info.name = (m_Engine) ? m_Engine->getDriverName() : "No Driver";
    return info;
}

bool SoundManager::SetActiveDevice(const std::string &deviceId)
{
    if (m_Engine)
    {
        m_Engine->drop();
        m_Engine = nullptr;
    }

    m_Engine = createIrrKlangDevice(ESOD_AUTO_DETECT, ESEO_MULTI_THREADED | ESEO_LOAD_PLUGINS | ESEO_USE_3D_BUFFERS, deviceId.c_str());

    if (!m_Engine)
    {
        LOGGER_ERROR("SoundManager") << "Failed to switch device to: " << deviceId;
        Init();
        return false;
    }
    return true;
}