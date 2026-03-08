#include <algorithm>
#include <ecs/unit/media_components.h>
#include <ecs/unit/render_components.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <render/logic/video_decoder.h>
#include <iostream>
#include <core/logic/logger.h>
#include <vector>



void VideoPlayerComponent::Play()
{
    isPlaying = true;
    if (decoder) decoder->Play();
}

void VideoPlayerComponent::Pause()
{
    isPlaying = false;
    if (decoder) decoder->Pause();
}

void VideoPlayerComponent::Stop()
{
    isPlaying = false;
    if (decoder) decoder->Stop();
}

void VideoPlayerComponent::Replay()
{
    isPlaying = true;
    if (decoder)
    {
        decoder->Seek(0);
        decoder->Play();
    }
}

void VideoPlayerComponent::Seek(double time)
{
    if (decoder) decoder->Seek(time);
}
