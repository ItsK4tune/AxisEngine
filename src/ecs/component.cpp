#include <algorithm>
#include <ecs/component.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <rendering/core/video_decoder.h>
#include <iostream>
#include <core/utils/logger.h>
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
