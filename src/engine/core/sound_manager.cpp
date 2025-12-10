#include <engine/core/sound_manager.h>
#include <engine/utils/irrKlang_glm_helpers.h>

SoundManager::~SoundManager() {
    if (m_Engine) m_Engine->drop();
}

void SoundManager::Init() {
    m_Engine = createIrrKlangDevice();
    if (!m_Engine) {
        std::cerr << "[SoundManager] Could not startup irrKlang engine" << std::endl;
    }
}

void SoundManager::Play2D(const char* path, bool loop) {
    if (m_Engine) m_Engine->play2D(path, loop);
}

void SoundManager::Play2D(ISoundSource* source, bool loop) {
    if (m_Engine && source) m_Engine->play2D(source, loop);
}

void SoundManager::Play3D(ISoundSource* source, glm::vec3 pos, bool loop) {
    if (m_Engine && source) {
        m_Engine->play3D(source, IrrKlangGLMHelpers::convert(pos), loop);
    }
}

void SoundManager::UpdateListener(glm::vec3 position, glm::vec3 lookDir, glm::vec3 up) {
    if (m_Engine) {
        m_Engine->setListenerPosition(IrrKlangGLMHelpers::convert(position), IrrKlangGLMHelpers::convert(lookDir), vec3df(0,0,0), IrrKlangGLMHelpers::convert(up));
    }
}

void SoundManager::Stop(ISoundSource* source) {
    if (m_Engine && source) {
        m_Engine->stopAllSoundsOfSoundSource(source);
    }
}

void SoundManager::StopAll() {
    if (m_Engine) m_Engine->stopAllSounds();
}

void SoundManager::SetVolume(float volume) {
    if (m_Engine) m_Engine->setSoundVolume(volume);
}