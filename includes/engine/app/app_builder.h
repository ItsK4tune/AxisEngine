#pragma once

#include <app/config_loader.h>
#include <memory>

class IGraphicsContext;
class IAudioEngine;
class IPhysicsWorld;
class IWindow;

class AppBuilder
{
public:
    static std::unique_ptr<IGraphicsContext> CreateGraphicsContext(const AppConfig &config);
    static std::unique_ptr<IAudioEngine> CreateAudioEngine(const AppConfig &config);
    static std::unique_ptr<IPhysicsWorld> CreatePhysicsWorld(const AppConfig &config);
    static std::unique_ptr<IWindow> MakeWindow();
};
