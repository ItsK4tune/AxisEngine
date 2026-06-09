#pragma once

#include <audio/interface/i_audio_engine.h>
#include <audio/interface/i_audio_source.h>
#include <audio/interface/i_sound.h>
#include <render/interface/i_texture_manager.h>

namespace axis_test_mocks
{
class FakeTextureManager : public ITextureManager
{
public:
    unsigned int CreateTexture() override
    {
        return GenTexture();
    }

    unsigned int GenTexture() override
    {
        return nextTextureId++;
    }

    void BindTexture(TextureType, unsigned int texture) override
    {
        boundTexture = texture;
    }

    void DeleteTexture(unsigned int texture) override
    {
        deletedTexture = texture;
    }

    void DeleteTextures(int n, const unsigned int* textures) override
    {
        deletedTextureCount += n;
        if (n > 0 && textures)
            deletedTexture = textures[0];
    }

    void TexParameteri(TextureType, TextureParameter, int) override
    {
    }

    void TexParameterf(TextureType, TextureParameter, float) override
    {
    }

    void TexParameterfv(TextureType, TextureParameter, const float*) override
    {
    }

    void GenerateMipmap(TextureType) override
    {
        ++generateMipmapCount;
    }

    void TexImage1D(TextureType, int, InternalFormat, int, int, TextureFormat, DataType, const void*) override
    {
    }

    void TexImage2D(TextureType, int, InternalFormat, int width, int height, int, TextureFormat, DataType,
                    const void*) override
    {
        lastWidth = width;
        lastHeight = height;
        ++texImage2DCount;
    }

    void TexImage3D(TextureType, int, InternalFormat, int, int, int, int, TextureFormat, DataType, const void*) override
    {
    }

    void TexSubImage2D(TextureType, int, int, int, int, int, TextureFormat, DataType, const void*) override
    {
    }

    void ActiveTexture(TextureUnit) override
    {
    }

    void PixelStorei(PixelStoreParam, int) override
    {
    }

    const char* GetBackendName() const override
    {
        return "FakeTextureManager";
    }

    unsigned int nextTextureId = 1;
    unsigned int boundTexture = 0;
    unsigned int deletedTexture = 0;
    int deletedTextureCount = 0;
    int generateMipmapCount = 0;
    int texImage2DCount = 0;
    int lastWidth = 0;
    int lastHeight = 0;
};

class FakeAudioEngine : public IAudioEngine
{
public:
    bool Initialize() override
    {
        return true;
    }

    void Update() override
    {
    }

    void Shutdown() override
    {
    }

    void SetListenerPosition(const glm::vec3&, const glm::vec3&) override
    {
    }

    void SetGlobalVolume(float volume) override
    {
        globalVolume = volume;
    }

    std::shared_ptr<ISound> Play2D(const std::string&, bool = false, bool = false) override
    {
        return nullptr;
    }

    std::shared_ptr<ISound> Play2D(IAudioSource*, bool = false, bool = false) override
    {
        return nullptr;
    }

    std::shared_ptr<ISound> Play3D(const std::string&, const glm::vec3&, bool = false, bool = false) override
    {
        return nullptr;
    }

    std::shared_ptr<ISound> Play3D(IAudioSource*, const glm::vec3&, bool = false, bool = false) override
    {
        return nullptr;
    }

    std::shared_ptr<IAudioSource> AddSoundSourceFromFile(const std::string&) override
    {
        return nullptr;
    }

    void StopAllSounds() override
    {
        stoppedAllSounds = true;
    }

    float globalVolume = 100.0f;
    bool stoppedAllSounds = false;
};
}  // namespace axis_test_mocks
