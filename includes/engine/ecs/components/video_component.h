#pragma once

#include <graphics/core/video_decoder.h>
#include <memory>
#include <string>

struct VideoPlayerComponent
{
    std::string filePath = "";
    bool isLooping = false;
    bool playOnAwake = false;
    float volume = 1.0f;
    bool isPlaying = false;
    bool isLoaded = false;
    float speed = 1.0f;
    int maxDecodes = 1;

    std::shared_ptr<VideoDecoder> decoder = nullptr;

    void Play();
    void Pause();
    void Stop();
    void Replay();
    void Seek(double time);
};
