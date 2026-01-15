#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <vector>
#include <functional>

#include <engine/core/state_machine.h>
#include <engine/ecs/component.h>
#include <engine/ecs/system.h>
#include <engine/physic/physic_world.h>
#include <engine/core/resource_manager.h>
#include <engine/core/scene_manager.h>
#include <engine/core/sound_manager.h>
#include <engine/core/post_process_pipeline.h>
#include <engine/core/post_process_pipeline.h>
#include <engine/core/input_manager.h>

enum class WindowMode
{
    WINDOWED,
    FULLSCREEN,
    BORDERLESS
};

struct AppConfig
{
    std::string title = "Game Engine";
    int width = 800;
    int height = 600;
    bool vsync = false;
    WindowMode mode = WindowMode::WINDOWED;
    int monitorIndex = 0;
    int refreshRate = 0; // 0 = unlimited/monitor default
    int frameRateLimit = 0; // 0 = unlimited
};

class Application
{
public:
    Application(const AppConfig &config);
    ~Application();

    bool Init();
    void Run();

    template <typename T, typename... Args>
    void PushState(Args &&...args)
    {
        m_StateMachine.PushState(std::make_unique<T>(std::forward<Args>(args)...));
    }

    Scene &GetScene() { return scene; }
    PhysicsWorld &GetPhysicsWorld() { return *physicsWorld; }

    ResourceManager &GetResourceManager() { return *resourceManager; }
    SceneManager &GetSceneManager() { return *sceneManager; }
    KeyboardManager &GetKeyboard() const { return *keyboardManager; }
    MouseManager &GetMouse() const { return *mouseManager; }
    InputManager &GetInputManager() const { return *inputManager; }

    GLFWwindow *GetWindow() const { return window; }
    int GetWidth() const { return m_Config.width; }
    int GetHeight() const { return m_Config.height; }

    RenderSystem &GetRenderSystem() { return renderSystem; }
    UIRenderSystem &GetUIRenderSystem() { return uiRenderSystem; }
    SkyboxRenderSystem &GetSkyboxRenderSystem() { return skyboxRenderSystem; }
    UIInteractSystem &GetUIInteractSystem() { return uiInteractSystem; }
    PhysicsSystem &GetPhysicsSystem() { return physicsSystem; }
    AnimationSystem &GetAnimationSystem() { return animationSystem; }
    ScriptableSystem &GetScriptSystem() { return scriptSystem; }
    SoundManager& GetSoundManager() { return *soundManager; }

    PostProcessPipeline& GetPostProcess() { return postProcess; }
    AudioSystem& GetAudioSystem() { return audioSystem; }
    ParticleSystem& GetParticleSystem() { return particleSystem; }

    void ProcessInput();
    void SetWindowConfiguration(int width, int height, WindowMode mode = WindowMode::WINDOWED, int monitorIndex = 0, int refreshRate = 0);
    void SetVsync(bool enable);
    void SetFrameRateLimit(int limit);
    void OnResize(int width, int height);
    void OnMouseMove(double xpos, double ypos);
    void OnMouseButton(int button, int action, int mods);
    void OnScroll(double xoffset, double yoffset);
    // void OnKey(int key, int scancode, int action, int mods);

private:
    AppConfig m_Config;
    GLFWwindow *window = nullptr;
    StateMachine m_StateMachine;

    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    std::unique_ptr<PhysicsWorld> physicsWorld;
    Scene scene;

    std::unique_ptr<KeyboardManager> keyboardManager;
    std::unique_ptr<MouseManager> mouseManager;
    std::unique_ptr<InputManager> inputManager;
    std::unique_ptr<SoundManager> soundManager;
    std::unique_ptr<SceneManager> sceneManager;
    std::unique_ptr<ResourceManager> resourceManager;

    PhysicsSystem physicsSystem;
    RenderSystem renderSystem;
    AnimationSystem animationSystem;
    UIInteractSystem uiInteractSystem;
    UIRenderSystem uiRenderSystem;
    ScriptableSystem scriptSystem;
    SkyboxRenderSystem skyboxRenderSystem;
    AudioSystem audioSystem;
    ParticleSystem particleSystem; // New

    PostProcessPipeline postProcess;
};