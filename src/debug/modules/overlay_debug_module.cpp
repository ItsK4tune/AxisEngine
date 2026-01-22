#include <debug/modules/overlay_debug_module.h>

#ifdef ENABLE_DEBUG_SYSTEM

#include <app/application.h>
#include <iostream>
#include <GLFW/glfw3.h>
#include <sstream>
#include <iomanip>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

OverlayDebugModule::OverlayDebugModule() {}
OverlayDebugModule::~OverlayDebugModule() {}

void OverlayDebugModule::Init(Application *app)
{
    m_App = app;
}

void OverlayDebugModule::SetSharedResources(Font *font, Shader *shader, UIModel *quad)
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
    // Stats are updated externally via SetStats
}

void OverlayDebugModule::Render(Scene &scene)
{
    if (!m_App || !m_Enabled || !m_ShowStatsOverlay)
        return;

    if (!m_DebugFont || !m_TextShader || !m_TextQuad)
        return;

    int width = m_App->GetWidth();
    int height = m_App->GetHeight();

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    size_t totalEntities = scene.registry.storage<entt::entity>().size();
    int renderedEntities = m_App->GetRenderSystem().GetRenderedCount();

    std::stringstream ss;
    ss << std::fixed << std::setprecision(1);

    auto appendStats = [&]()
    {
        ss << "FPS: " << m_CurrentFps << " (" << m_CurrentFrameTime << " ms)\n";
        ss << "Entities: " << totalEntities << " | Rendered: " << renderedEntities << "\n";
        ss << "TimeScale: " << m_App->GetTimeScale() << "x | Paused: " << (m_App->IsPaused() ? "YES" : "NO") << "\n";
    };

    auto appendTools = [&]()
    {
        ss << "=== DEBUG TOOLS ===\n";
        auto boolStr = [](bool v)
        { return v ? "[ON]" : "[OFF]"; };
        ss << "F6: Wireframe  | S+F6: Skybox: " << boolStr(m_App->GetSkyboxRenderSystem().IsEnabled()) << "\n";
        ss << "F7: NoTexture  | S+F7: Shadows: " << boolStr(m_App->GetRenderSystem().IsShadowsEnabled()) << "\n";
        ss << "F8: Physics    | S+F8: Audio\n";
        ss << "F9: UI System: " << boolStr(m_App->GetUIRenderSystem().IsEnabled()) << " | S+F9: Particle\n";
        ss << "S+F3: Names    | S+F4: Gizmos\n";
        ss << "S+F5: Lights   | S+F11: Cam\n";
        ss << "F11: Paused:   " << boolStr(m_App->IsPaused());
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
        for (char c : line)
        {
            const Character &ch = m_DebugFont->GetCharacter(c);
            textWidth += (ch.Advance >> 6) * scale;
        }
        float x = (float)width - textWidth - 10.0f;

        RenderText(line, x, yStart, scale, glm::vec3(0.0f, 1.0f, 0.0f));
        yStart += 25.0f;
    }

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void OverlayDebugModule::ProcessInput(KeyboardManager &keyboard)
{
    if (!m_App || !m_Enabled)
        return;

    ProcessKey(keyboard, GLFW_KEY_F10, m_F10Pressed, [this, &keyboard]()
               {
        bool shift = keyboard.GetKey(GLFW_KEY_LEFT_SHIFT) || keyboard.GetKey(GLFW_KEY_RIGHT_SHIFT);
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
        std::cout << "  Font:   " << (fontOK ? "OK" : "MISSING (Check src/asset/fonts/time.ttf)") << std::endl;
        std::cout << "  Shader: " << (shaderOK ? "OK" : "MISSING (Check src/asset/shaders/text.vs/fs)") << std::endl;
        std::cout << "  Quad:   " << (quadOK ? "OK" : "MISSING") << std::endl;

        if (!fontOK || !shaderOK || !quadOK)
        {
            std::cout << "[WARNING] Overlay will NOT render due to missing resources!" << std::endl;
        }
    }
    std::cout << "=========================================" << std::endl;
}

void OverlayDebugModule::RenderText(const std::string &text, float x, float y, float scale, glm::vec3 color)
{
    int width = m_App->GetWidth();
    int height = m_App->GetHeight();

    m_TextShader->use();
    glm::mat4 projection = glm::ortho(0.0f, (float)width, (float)height, 0.0f, -1.0f, 1.0f);
    m_TextShader->setMat4("projection", projection);
    m_TextShader->setInt("text", 0);

    for (char c : text)
    {
        const Character &ch = m_DebugFont->GetCharacter(c);

        float xpos = x + ch.Bearing.x * scale;
        float ypos = y + (ch.Size.y - ch.Bearing.y) * scale;
        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;

        std::vector<float> vertices = {
            xpos, ypos - h, 0.0f, 0.0f,
            xpos, ypos, 0.0f, 1.0f,
            xpos + w, ypos, 1.0f, 1.0f,

            xpos, ypos - h, 0.0f, 0.0f,
            xpos + w, ypos, 1.0f, 1.0f,
            xpos + w, ypos - h, 1.0f, 0.0f};

        m_TextQuad->DrawDynamic(*m_TextShader, ch.TextureID, color, vertices);

        x += (ch.Advance >> 6) * scale;
    }
}

void OverlayDebugModule::ProcessKey(KeyboardManager &keyboard, int key, bool &pressedState, std::function<void()> action)
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
