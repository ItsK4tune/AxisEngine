#include "test_framework.h"

#include <render/interface/i_texture_manager.h>
#include <render/logic/video_decoder.h>

#include <atomic>
#include <chrono>
#include <thread>

namespace
{
class RecordingTextureManager final : public ITextureManager
{
public:
    unsigned int CreateTexture() override { return GenTexture(); }
    unsigned int GenTexture() override { return m_NextTexture++; }
    void BindTexture(TextureType, unsigned int) override {}
    void DeleteTexture(unsigned int) override {}
    void DeleteTextures(int, const unsigned int*) override {}
    void TexParameteri(TextureType, TextureParameter, int) override {}
    void TexParameterf(TextureType, TextureParameter, float) override {}
    void TexParameterfv(TextureType, TextureParameter, const float*) override {}
    void GenerateMipmap(TextureType) override {}
    void TexImage1D(TextureType, int, InternalFormat, int, int, TextureFormat, DataType, const void*) override {}
    void TexImage2D(TextureType, int, InternalFormat, int, int, int, TextureFormat, DataType, const void*) override {}
    void TexImage3D(TextureType, int, InternalFormat, int, int, int, int, TextureFormat, DataType,
                    const void*) override
    {
    }
    void TexSubImage2D(TextureType, int, int, int, int, int, TextureFormat, DataType, const void*) override
    {
        ++uploads;
    }
    void ActiveTexture(TextureUnit) override {}
    void PixelStorei(PixelStoreParam, int) override {}
    const char* GetBackendName() const override { return "Recording"; }

    std::atomic<int> uploads{0};

private:
    unsigned int m_NextTexture = 1;
};

class VideoManagerRegistration final
{
public:
    explicit VideoManagerRegistration(ITextureManager& manager)
    {
        VideoDecoder::SetTextureManager(manager);
        VideoDecoder::ClearBufferManager();
    }

    ~VideoManagerRegistration()
    {
        VideoDecoder::ClearBufferManager();
        VideoDecoder::ClearTextureManager();
    }
};
}  // namespace

AXIS_TEST_CASE("Async video presentation follows media timestamps instead of render FPS")
{
    using namespace std::chrono_literals;

    RecordingTextureManager textures;
    VideoManagerRegistration registration(textures);
    VideoDecoder decoder;
    decoder.SetOutputSize(16, 16);
    AXIS_CHECK(decoder.Load(AXIS_TEST_VIDEO_PATH));
    decoder.SetLoop(false);
    decoder.SetMaxDecodeSteps(3);
    decoder.SetAsyncDecodeEnabled(true);
    decoder.Play();

    // Wait until the decoder has produced and presented the timestamp-zero frame.
    // A zero delta deliberately keeps the media clock stationary.
    const auto deadline = std::chrono::steady_clock::now() + 2s;
    while (textures.uploads.load() == 0 && std::chrono::steady_clock::now() < deadline)
    {
        decoder.Update(0.0f);
        std::this_thread::sleep_for(2ms);
    }
    AXIS_CHECK(textures.uploads.load() == 1);

    // Let the worker fill its queue with future frames, then simulate a very
    // high-FPS render loop without advancing time. No future frame may present.
    std::this_thread::sleep_for(50ms);
    const int stationaryUploads = textures.uploads.load();
    for (int frame = 0; frame < 12; ++frame)
        decoder.Update(0.0f);
    AXIS_CHECK(textures.uploads.load() == stationaryUploads);

    const double startTime = decoder.GetCurrentTime();
    decoder.SetSpeed(0.5f);
    decoder.Update(0.2f);
    AXIS_CHECK_NEAR(static_cast<float>(decoder.GetCurrentTime() - startTime), 0.1f, 0.0001f);

    const double halfSpeedTime = decoder.GetCurrentTime();
    decoder.SetSpeed(2.0f);
    decoder.Update(0.2f);
    AXIS_CHECK_NEAR(static_cast<float>(decoder.GetCurrentTime() - halfSpeedTime), 0.4f, 0.0001f);
}
