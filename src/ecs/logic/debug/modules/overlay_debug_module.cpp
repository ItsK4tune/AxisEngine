#include <ecs/logic/debug/modules/overlay_debug_module.h>
#include <render/interface/i_graphics_context.h>
#include <resource/unit/font.h>

#ifdef ENABLE_EDITOR

#include <platform/logic/input_manager.h>
#include <scene/logic/scene.h>
#include <ecs/logic/render_system.h>
#include <ecs/logic/skybox_render_system.h>
#include <ecs/logic/system_manager.h>
#include <core/logic/time_service.h>
#include <platform/logic/io_handler.h>
#include <ecs/interface/i_render_service.h>
#include <ecs/interface/i_shadow_service.h>
#include <ecs/interface/i_skybox_service.h>
#include <ecs/interface/i_ui_service.h>
#include <platform/interface/i_window.h>
#include <iostream>
#include <sstream>
#include <iomanip>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <editor/editor_system.h>
#include <core/logic/service_locator.h>
#include <core/logic/localization_system.h>
#include <core/logic/string_utils.h>

OverlayDebugModule::OverlayDebugModule() {}
OverlayDebugModule::~OverlayDebugModule() {}

void OverlayDebugModule::Initialize()
{
}

void OverlayDebugModule::SetSharedResources(std::shared_ptr<Font> font, std::shared_ptr<Shader> shader, std::shared_ptr<UIModel> quad)
{
    m_DebugFont = font;
    m_TextShader = shader;
    m_TextQuad = quad;
}

void OverlayDebugModule::SetStats(float fps, float frameTime)
{
    m_CurrentFps = fps;
    m_CurrentFrameTime = frameTime;
}

void OverlayDebugModule::OnUpdate(float dt)
{
}

void OverlayDebugModule::Render(Scene &scene)
{
    if (!m_Enabled || !m_ShowStatsOverlay)
        return;

    if (!m_DebugFont || !m_TextShader || !m_TextQuad)
        return;

    auto& sl = ServiceLocator::Instance();
    auto* graphics = sl.Resolve<IGraphicsContext>();
    if (!graphics) return;

    auto& io = sl.Require<IOHandler>();
    auto& systems = sl.Require<SystemManager>();
    auto& timer = sl.Require<TimeService>();

    int width = io.GetMonitorManager().GetWidth();
    int height = io.GetMonitorManager().GetHeight();

    graphics->SetDepthTest(false);
    graphics->SetBlending(true);
    graphics->SetBlendFunc(BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);

    size_t totalEntities = scene.registry.storage<entt::entity>().size();
    int renderedEntities = 0;
    if (auto* rs = sl.Resolve<IRenderService>()) {
        renderedEntities = rs->GetRenderedCount();
    }

    std::stringstream ss;
    ss << std::fixed << std::setprecision(1);

    auto appendStats = [&]()
    {
        auto* l10n = sl.Resolve<LocalizationSystem>();
        std::string fpsLabel = l10n ? l10n->GetFormat("HUD.Score", m_CurrentFps) : "FPS: " + std::to_string(m_CurrentFps);
        
        ss << fpsLabel << " (" << m_CurrentFrameTime << " ms)\n";
        ss << "Entities: " << totalEntities << " | Rendered: " << renderedEntities << "\n";
        ss << "TimeScale: " << timer.GetTimeScale() << "x | Paused: " << (timer.IsPaused() ? "YES" : "NO") << "\n";
    };

    auto appendTools = [&]()
    {
        ss << "=== DEBUG TOOLS ===\n";
        auto boolStr = [](bool v)
        { return v ? "[ON]" : "[OFF]"; };
        bool noTexture = false;
        bool shadowsEnabled = false;
        if (auto* rs = sl.Resolve<IRenderService>()) noTexture = rs->IsDebugNoTexture();
        if (auto* ss = sl.Resolve<IShadowService>()) shadowsEnabled = ss->IsShadowsEnabled();

        ss << "F6: Wireframe: " << boolStr(DebugConfig::ShowWireframe) << "  | S+F6: Skybox: " << boolStr(sl.Resolve<ISkyboxService>() && sl.Resolve<ISkyboxService>()->IsEnabled()) << "\n";
        ss << "F7: NoTexture: " << boolStr(noTexture) << "  | S+F7: Shadows: " << boolStr(shadowsEnabled) << "\n";
        ss << "F8: Physics: " << boolStr(DebugConfig::ShowPhysics) << "    | S+F8: Audio: [NYI]\n";
        ss << "F9: UI System: " << boolStr(sl.Resolve<IUIService>() && sl.Resolve<IUIService>()->IsEnabled()) << " | S+F9: Particle: [NYI]\n";
        ss << "S+F3: Names: " << boolStr(DebugConfig::ShowEntityNames) << "    | S+F4: Gizmos: " << boolStr(DebugConfig::ShowGizmos) << "\n";
        ss << "S+F5: Lights: " << boolStr(DebugConfig::ShowLightGizmos) << "   | S+F11: Cam\n";
        ss << "F11: Paused:   " << boolStr(timer.IsPaused());
    };

    if (m_OverlayMode == 1)
    {
        appendStats();
    }
    else if (m_OverlayMode == 2)
    {
        appendTools();
    }
    else if (m_OverlayMode == 3)
    {
        appendStats();
        ss << "\n";
        appendTools();
    }

    std::string fullText = ss.str();

    std::istringstream textStream(fullText);
    std::string line;
    float yStart = 30.0f;
    float scale = 0.5f;

    while (std::getline(textStream, line))
    {
        float textWidth = 0.0f;
        auto codepoints = StringUtils::GetCodepoints(line);
        for (uint32_t cp : codepoints)
        {
            const Character &ch = m_DebugFont->GetCharacter(cp);
            textWidth += (ch.advance >> 6) * scale;
        }
        float x = (float)width - textWidth - 10.0f;

        RenderText(line, x, yStart, scale, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
        yStart += 25.0f;
    }

    graphics->SetBlending(false);
    graphics->SetDepthTest(true);
}

void OverlayDebugModule::ProcessInput(KeyboardManager &keyboard)
{
    if (!m_Enabled)
        return;

    ProcessKey(keyboard, Key::F10, m_F10Pressed, [this, &keyboard]()
               {
        bool shift = keyboard.GetKey(Key::LeftShift) || keyboard.GetKey(Key::RightShift);
        if (shift) {
            m_OverlayMode = (m_OverlayMode % 3) + 1;
            std::cout << "\n========== Overlay Mode (Shift+F10) ==========" << std::endl;
            std::cout << "[Debug] Mode " << m_OverlayMode << "/3: ";
            if (m_OverlayMode == 1) std::cout << "General Stats";
            else if (m_OverlayMode == 2) std::cout << "Debug Tools";
            else if (m_OverlayMode == 3) std::cout << "All Info";
            std::cout << std::endl;
            std::cout << "============================================" << std::endl;
        } else {
            ToggleStatsOverlay();
        } });

    ProcessKey(keyboard, Key::L, m_LPressed, [this, &keyboard]()
               {
        if (keyboard.GetKey(Key::LeftShift) || keyboard.GetKey(Key::RightShift)) {
            auto* l10n = ServiceLocator::Instance().Resolve<LocalizationSystem>();
            if (l10n) {
                static bool isVi = false;
                isVi = !isVi;
                l10n->SetLanguage(isVi ? "vi" : "en");
                std::cout << "[Debug] Language switched to: " << (isVi ? "vi" : "en") << std::endl;
            }
        } });
}

void OverlayDebugModule::ToggleStatsOverlay()
{
    m_ShowStatsOverlay = !m_ShowStatsOverlay;
    std::cout << "\n========== Stats Overlay (F10) ==========" << std::endl;
    std::cout << "[DebugSystem] Stats Overlay: " << (m_ShowStatsOverlay ? "ON" : "OFF") << std::endl;

    if (m_ShowStatsOverlay)
    {
        bool fontOK = (m_DebugFont != nullptr);
        bool shaderOK = (m_TextShader != nullptr);
        bool quadOK = (m_TextQuad != nullptr);

        std::cout << "[DebugSystem] Resources Status:" << std::endl;
        std::cout << "  Font:   " << (fontOK ? "OK" : "MISSING (Check include/engine/asset/fonts/time.ttf)") << std::endl;
        std::cout << "  Shader: " << (shaderOK ? "OK" : "MISSING (Check include/engine/asset/shaders/text.vs/fs)") << std::endl;
        std::cout << "  Quad:   " << (quadOK ? "OK" : "MISSING") << std::endl;

        if (!fontOK || !shaderOK || !quadOK)
        {
            std::cout << "[WARNING] Overlay will NOT render due to missing resources!" << std::endl;
        }
    }
    std::cout << "=========================================" << std::endl;
}

void OverlayDebugModule::RenderText(const std::string &text, float x, float y, float scale, glm::vec4 color)
{
    auto& io = ServiceLocator::Instance().Require<IOHandler>();
    int width = io.GetMonitorManager().GetWidth();
    int height = io.GetMonitorManager().GetHeight();

    m_TextShader->use();
    glm::mat4 projection = glm::ortho(0.0f, (float)width, (float)height, 0.0f, -1.0f, 1.0f);
    m_TextShader->setMat4("u_Projection", projection);
    m_TextShader->setInt("text", 0);

    auto codepoints = StringUtils::GetCodepoints(text);
    for (uint32_t cp : codepoints)
    {
        const Character &ch = m_DebugFont->GetCharacter(cp);

        float xpos = x + ch.bearing.x * scale;
        float ypos = y + (ch.size.y - ch.bearing.y) * scale;
        float w = ch.size.x * scale;
        float h = ch.size.y * scale;

        std::vector<float> vertices = {
            xpos, ypos - h, 0.0f, 0.0f,
            xpos, ypos, 0.0f, 1.0f,
            xpos + w, ypos, 1.0f, 1.0f,

            xpos, ypos - h, 0.0f, 0.0f,
            xpos + w, ypos, 1.0f, 1.0f,
            xpos + w, ypos - h, 1.0f, 0.0f};

        m_TextQuad->DrawDynamic(*m_TextShader, ch.textureID, color, vertices);

        x += (ch.advance >> 6) * scale;
    }
}

void OverlayDebugModule::ProcessKey(KeyboardManager &keyboard, Key key, bool &pressedState, std::function<void()> action)
{
    if (keyboard.GetKey(key))
    {
        if (!pressedState)
        {
            action();
            pressedState = true;
        }
    }
    else
    {
        pressedState = false;
    }
}

#endif
