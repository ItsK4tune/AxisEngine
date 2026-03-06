#ifdef ENABLE_DEBUG_SYSTEM

#include <core/debug/i_debug_system.h>
#include <core/engine_context.h>
#include <rendering/core/shader.h>
#include <rendering/renderer/font.h>
#include <rendering/renderer/ui_model.h>
#include <memory>
#include <vector>

class Scene;
class IDebugModule;

class DebugSystem : public IDebugSystem
{
public:
    DebugSystem();
    ~DebugSystem();

    void Init(EngineContext ctx) override;
    void OnUpdate(float dt) override;
    void Render(Scene& scene) override;

private:
    EngineContext m_Ctx;

    std::shared_ptr<Font> m_DebugFont = nullptr;
    std::shared_ptr<Shader> m_TextShader = nullptr;
    std::shared_ptr<UIModel> m_TextQuad = nullptr;

    float m_FpsTimer = 0.0f;
    int m_FrameCount = 0;
    float m_CurrentFps = 0.0f;
    float m_CurrentFrameTime = 0.0f;

    std::vector<std::unique_ptr<IDebugModule>> m_Modules;
};

#endif
