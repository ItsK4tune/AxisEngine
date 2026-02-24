#include <debug/modules/gizmo_debug_module.h>

#ifdef ENABLE_DEBUG_SYSTEM

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <app/application.h>
#include <input/keyboard_manager.h>
#include <resource/resource_manager.h>
#include <scene/scene.h>
#include <ecs/components/render_components.h>
#include <ecs/components/camera_component.h>
#include <ecs/components/light_components.h>
#include <ecs/components/info_component.h>
#include <interface/graphic/i_graphics_context.h>
#include <interface/graphic/i_render_state_manager.h>
#include <debug/debug_config.h>

GizmoDebugModule::GizmoDebugModule() {}
GizmoDebugModule::~GizmoDebugModule() {}

void GizmoDebugModule::Init(std::shared_ptr<Application> app)
{
    m_App = app;
}

void GizmoDebugModule::SetSharedResources(std::shared_ptr<Font> font, std::shared_ptr<Shader> shader, std::shared_ptr<UIModel> quad)
{
    m_DebugFont = font;
    m_TextShader = shader;
    m_TextQuad = quad;
}

void GizmoDebugModule::OnUpdate(float dt)
{
    if (!m_App || !m_Enabled)
        return;

    UpdateDebugLabels(m_App->GetScene());
    UpdateLightLabels(m_App->GetScene());
}

void GizmoDebugModule::Render(Scene &scene)
{
    if (!m_App || !m_Enabled || !DebugConfig::ShowGizmos)
        return;

    int width = m_App->GetWidth();
    int height = m_App->GetHeight();

    auto debugShader = m_App->GetResourceManager().GetShader("debugLine");
    if (!debugShader) return;

    entt::entity camEntity = scene.GetActiveCamera();
    if (camEntity == entt::null) return;

    auto& cam = scene.registry.get<CameraComponent>(camEntity);
    auto& camTrans = scene.registry.get<TransformComponent>(camEntity);

    float aspect = (float)width / (float)height;
    glm::mat4 proj = glm::perspective(glm::radians(cam.fov), aspect, cam.nearPlane, cam.farPlane);
    glm::mat4 view = glm::lookAt(camTrans.position, camTrans.position + cam.front, cam.worldUp);

    debugShader->use();
    debugShader->setMat4("projection", proj);
    debugShader->setMat4("view", view);
    debugShader->setMat4("model", glm::mat4(1.0f));

    auto viewEntities = scene.registry.view<TransformComponent>();

    std::vector<float> lineVertices;
    auto addLine = [&](const glm::vec3& start, const glm::vec3& end, const glm::vec3& color) {
        lineVertices.push_back(start.x); lineVertices.push_back(start.y); lineVertices.push_back(start.z);
        lineVertices.push_back(color.r); lineVertices.push_back(color.g); lineVertices.push_back(color.b);
        lineVertices.push_back(end.x); lineVertices.push_back(end.y); lineVertices.push_back(end.z);
        lineVertices.push_back(color.r); lineVertices.push_back(color.g); lineVertices.push_back(color.b);
    };

    for (auto entity : viewEntities)
    {
        auto& tr = viewEntities.get<TransformComponent>(entity);
        glm::mat4 modelMatrix = tr.GetWorldModelMatrix(scene.registry);

        glm::vec3 pos = glm::vec3(modelMatrix[3]);
        glm::vec3 right = glm::normalize(glm::vec3(modelMatrix[0]));
        glm::vec3 up = glm::normalize(glm::vec3(modelMatrix[1]));
        glm::vec3 forward = glm::normalize(glm::vec3(modelMatrix[2]));

        float length = 1.0f;
        addLine(pos, pos + right * length, glm::vec3(1.0f, 0.0f, 0.0f));
        addLine(pos, pos + up * length, glm::vec3(0.0f, 1.0f, 0.0f));
        addLine(pos, pos + forward * length, glm::vec3(0.0f, 0.0f, 1.0f));
    }

    if (lineVertices.empty()) return;

    unsigned int vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, lineVertices.size() * sizeof(float), lineVertices.data(), GL_STREAM_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    m_App->GetGraphicsContext().GetRenderStateManager().Disable(Graphics::ServerCapability::DepthTest);
    glDrawArrays(GL_LINES, 0, lineVertices.size() / 6);
    m_App->GetGraphicsContext().GetRenderStateManager().Enable(Graphics::ServerCapability::DepthTest);

    glBindVertexArray(0);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

void GizmoDebugModule::ProcessInput(KeyboardManager &keyboard)
{
    if (!m_App || !m_Enabled)
        return;

    ProcessKey(keyboard, Input::Key::F3, m_F3Pressed, [this, &keyboard]()
               {
        bool shift = keyboard.GetKey(Input::Key::LeftShift) || keyboard.GetKey(Input::Key::RightShift);
        if (shift) {
            ToggleEntityNames();
            std::cout << "\n========== Entity Names (Shift+F3) ==========" << std::endl;
            std::cout << "[Debug] Entity Names: " << (m_ShowEntityNames ? "ON" : "OFF") << std::endl;
            std::cout << "=============================================" << std::endl;
        } });

    ProcessKey(keyboard, Input::Key::F4, m_F4Pressed, [this, &keyboard]()
               {
        bool shift = keyboard.GetKey(Input::Key::LeftShift) || keyboard.GetKey(Input::Key::RightShift);
        if (shift) {
            DebugConfig::ShowGizmos = !DebugConfig::ShowGizmos;
            std::cout << "\n========== Transform Gizmos (Shift+F4) ==========" << std::endl;
            std::cout << "[Debug] Transform Gizmos: " << (DebugConfig::ShowGizmos ? "ON" : "OFF") << std::endl;
            std::cout << "=================================================" << std::endl;
        } });

    ProcessKey(keyboard, Input::Key::F5, m_F5Pressed, [this, &keyboard]()
               {
        bool shift = keyboard.GetKey(Input::Key::LeftShift) || keyboard.GetKey(Input::Key::RightShift);
        if (shift) {
            ToggleLightGizmos();

            auto& reg = m_App->GetScene().registry;
            int p = 0; for(auto e : reg.view<PointLightComponent>()) p++;
            int s = 0; for(auto e : reg.view<SpotLightComponent>()) s++;

            int d_total = 0;
            int d_transform = 0;
            for(auto e : reg.view<DirectionalLightComponent>()) {
                d_total++;
                if(reg.all_of<TransformComponent>(e)) d_transform++;
            }

            std::cout << "\n========== Light Gizmos (Shift+F5) ==========" << std::endl;
            std::cout << "[Debug] Light Gizmos: " << (m_ShowLightGizmos ? "ON" : "OFF") << std::endl;
            std::cout << "[Debug] Stats: " << p << " Points, " << s << " Spots" << std::endl;
            std::cout << "[Debug] Directional: " << d_total << " Total (" << d_transform << " with Transform)" << std::endl;
            std::cout << "=============================================" << std::endl;
        } });
}

void GizmoDebugModule::ClearDebugLabels(Scene &scene)
{
    auto &registry = scene.registry;
    for (auto &pair : m_EntityLabelMap)
    {
        if (registry.valid(pair.second))
        {
            registry.destroy(pair.second);
        }
    }
    m_EntityLabelMap.clear();
}

void GizmoDebugModule::UpdateDebugLabels(Scene &scene)
{
    if (!m_ShowEntityNames)
    {
        if (!m_EntityLabelMap.empty())
        {
            ClearDebugLabels(scene);
        }
        return;
    }

    auto &registry = scene.registry;
    int width = m_App->GetWidth();
    int height = m_App->GetHeight();

    entt::entity camEntity = scene.GetActiveCamera();

    glm::mat4 vp = glm::mat4(1.0f);
    if (registry.valid(camEntity) && registry.all_of<CameraComponent, TransformComponent>(camEntity))
    {
        auto &cam = registry.get<CameraComponent>(camEntity);
        auto &camTrans = registry.get<TransformComponent>(camEntity);

        float aspect = (float)width / (float)height;
        glm::mat4 proj = glm::perspective(glm::radians(cam.fov), aspect, cam.nearPlane, cam.farPlane);
        glm::mat4 view = glm::lookAt(camTrans.position, camTrans.position + cam.front, cam.worldUp);
        vp = proj * view;
    }
    else
    {
        return;
    }

    auto view = registry.view<InfoComponent, TransformComponent>();

    std::unordered_map<entt::entity, entt::entity> nextMap;

    for (auto entity : view)
    {
        if (entity == camEntity)
            continue;

        auto &entityInfo = view.get<InfoComponent>(entity);
        if (entityInfo.tag == "DebugLabel")
            continue;

        entt::entity labelEntity = entt::null;
        if (m_EntityLabelMap.find(entity) != m_EntityLabelMap.end())
        {
            labelEntity = m_EntityLabelMap[entity];
            if (!registry.valid(labelEntity))
                labelEntity = entt::null;
        }

        auto &tr = view.get<TransformComponent>(entity);

        glm::mat4 modelMatrix = tr.GetWorldModelMatrix(registry);
        glm::vec3 labelPos;
        bool usedAABB = false;

        if (registry.all_of<MeshRendererComponent>(entity))
        {
            auto &mrc = registry.get<MeshRendererComponent>(entity);
            if (mrc.model)
            {
                glm::vec3 localTop = glm::vec3(
                    (mrc.model->AABBmin.x + mrc.model->AABBmax.x) * 0.5f,
                    mrc.model->AABBmax.y,
                    (mrc.model->AABBmin.z + mrc.model->AABBmax.z) * 0.5f);
                labelPos = glm::vec3(modelMatrix * glm::vec4(localTop, 1.0f));
                labelPos.y += 0.2f;
                usedAABB = true;
            }
        }

        if (!usedAABB)
        {
            labelPos = glm::vec3(modelMatrix[3]);
            labelPos.y += tr.scale.y * 0.6f + 0.5f;
        }

        glm::vec4 clipPos = vp * glm::vec4(labelPos, 1.0f);
        bool visible = false;
        glm::vec2 screenPos(0.0f);

        if (clipPos.w > 0.1f)
        {
            glm::vec3 ndc = glm::vec3(clipPos) / clipPos.w;
            if (ndc.x >= -1.0f && ndc.x <= 1.0f && ndc.y >= -1.0f && ndc.y <= 1.0f && ndc.z < 1.0f)
            {
                screenPos.x = (ndc.x * 0.5f + 0.5f) * width;
                screenPos.y = (1.0f - (ndc.y * 0.5f + 0.5f)) * height;
                visible = true;
            }
        }

        if (visible)
        {
            if (labelEntity == entt::null)
            {
                labelEntity = scene.createEntity();
                registry.emplace<InfoComponent>(labelEntity, "DebugLabel_" + entityInfo.name, "DebugLabel");
                registry.emplace<UITransformComponent>(labelEntity);
                auto &text = registry.emplace<UITextComponent>(labelEntity);

                text.model = m_TextQuad;
                text.shader = m_TextShader;
                text.font = m_DebugFont;
                text.text = entityInfo.name;
                text.color = glm::vec3(1.0f, 1.0f, 0.0f);
                text.scale = 2.0f;
            }

            auto &uiTr = registry.get<UITransformComponent>(labelEntity);
            auto &text = registry.get<UITextComponent>(labelEntity);

            float textW = 0.0f;
            if (m_DebugFont)
                textW = (float)text.text.length() * 11.0f * text.scale;

            uiTr.position = screenPos - glm::vec2(textW / 2.0f, 0.0f);
            uiTr.size = glm::vec2(textW, 20.0f * text.scale);
            uiTr.zIndex = 100;

            nextMap[entity] = labelEntity;
        }
        else
        {
            if (labelEntity != entt::null)
            {
                registry.destroy(labelEntity);
            }
        }
    }

    for (auto &pair : m_EntityLabelMap)
    {
        if (nextMap.find(pair.first) == nextMap.end())
        {
            if (registry.valid(pair.second))
            {
                registry.destroy(pair.second);
            }
        }
    }

    m_EntityLabelMap = nextMap;
}

void GizmoDebugModule::ClearLightLabels(Scene &scene)
{
    auto &registry = scene.registry;
    for (auto &pair : m_LightLabelMap)
    {
        if (registry.valid(pair.second))
        {
            registry.destroy(pair.second);
        }
    }
    m_LightLabelMap.clear();
}

void GizmoDebugModule::UpdateLightLabels(Scene &scene)
{
    if (!m_ShowLightGizmos)
    {
        if (!m_LightLabelMap.empty())
        {
            ClearLightLabels(scene);
        }
        return;
    }

    auto &registry = scene.registry;
    int width = m_App->GetWidth();
    int height = m_App->GetHeight();

    entt::entity camEntity = scene.GetActiveCamera();

    glm::mat4 vp = glm::mat4(1.0f);
    if (registry.valid(camEntity) && registry.all_of<CameraComponent, TransformComponent>(camEntity))
    {
        auto &cam = registry.get<CameraComponent>(camEntity);
        auto &camTrans = registry.get<TransformComponent>(camEntity);

        float aspect = (float)width / (float)height;
        glm::mat4 proj = glm::perspective(glm::radians(cam.fov), aspect, cam.nearPlane, cam.farPlane);
        glm::mat4 view = glm::lookAt(camTrans.position, camTrans.position + cam.front, cam.worldUp);
        vp = proj * view;
    }
    else
    {
        return;
    }

    std::unordered_map<entt::entity, entt::entity> nextMap;

    auto processLight = [&](entt::entity entity, const glm::vec3 &pos, const std::string &typeName, const glm::vec3 &color)
    {
        entt::entity labelEntity = entt::null;
        if (m_LightLabelMap.find(entity) != m_LightLabelMap.end())
        {
            labelEntity = m_LightLabelMap[entity];
            if (!registry.valid(labelEntity))
                labelEntity = entt::null;
        }

        glm::vec4 clipPos = vp * glm::vec4(pos, 1.0f);
        bool visible = false;
        glm::vec2 screenPos(0.0f);

        if (clipPos.w > 0.1f)
        {
            glm::vec3 ndc = glm::vec3(clipPos) / clipPos.w;
            if (ndc.x >= -1.0f && ndc.x <= 1.0f && ndc.y >= -1.0f && ndc.y <= 1.0f && ndc.z < 1.0f)
            {
                screenPos.x = (ndc.x * 0.5f + 0.5f) * width;
                screenPos.y = (1.0f - (ndc.y * 0.5f + 0.5f)) * height;
                visible = true;
            }
        }

        if (visible)
        {
            if (labelEntity == entt::null)
            {
                labelEntity = scene.createEntity();
                registry.emplace<InfoComponent>(labelEntity, "DebugLight_" + typeName, "DebugLabel");
                registry.emplace<UITransformComponent>(labelEntity);
                auto &text = registry.emplace<UITextComponent>(labelEntity);

                text.model = m_TextQuad;
                text.shader = m_TextShader;
                text.font = m_DebugFont;
                text.text = typeName;
                text.color = color;
                text.scale = 2.0f;
            }

            auto &uiTr = registry.get<UITransformComponent>(labelEntity);
            auto &text = registry.get<UITextComponent>(labelEntity);

            float textW = 0.0f;
            if (m_DebugFont)
            {
                std::istringstream textStream(typeName);
                std::string line;
                float maxW = 0.0f;
                while (std::getline(textStream, line))
                {
                    float w = (float)line.length() * 11.0f * text.scale;
                    if (w > maxW) maxW = w;
                }
                textW = maxW;
            }

            uiTr.position = screenPos - glm::vec2(textW / 2.0f, 0.0f);
            uiTr.size = glm::vec2(textW, 20.0f * text.scale); // UITextComponent handles multiline internally if supported, otherwise lines will overlap or need custom logic. Update: AxisEngine UITextComponent handles \n.
            uiTr.zIndex = 90;

            nextMap[entity] = labelEntity;
        }
        else
        {
            if (labelEntity != entt::null)
            {
                registry.destroy(labelEntity);
            }
        }
    };

    auto pointLights = registry.view<PointLightComponent, TransformComponent>();
    for (auto entity : pointLights)
    {
        auto [pl, tr] = pointLights.get<PointLightComponent, TransformComponent>(entity);
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1);
        ss << "[POINT]\nInt: " << pl.intensity << "\nCol: " << pl.diffuse.r << "," << pl.diffuse.g << "," << pl.diffuse.b;
        processLight(entity, glm::vec3(tr.GetWorldModelMatrix(registry)[3]), ss.str(), glm::vec3(1.0f, 1.0f, 0.0f));
    }

    auto spotLights = registry.view<SpotLightComponent, TransformComponent>();
    for (auto entity : spotLights)
    {
        auto [sl, tr] = spotLights.get<SpotLightComponent, TransformComponent>(entity);
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1);
        ss << "[SPOT]\nInt: " << sl.intensity << "\nCol: " << sl.diffuse.r << "," << sl.diffuse.g << "," << sl.diffuse.b;
        processLight(entity, glm::vec3(tr.GetWorldModelMatrix(registry)[3]), ss.str(), glm::vec3(0.0f, 1.0f, 1.0f));
    }

    auto dirLights = registry.view<DirectionalLightComponent>();
    for (auto entity : dirLights)
    {
        auto& dl = registry.get<DirectionalLightComponent>(entity);
        glm::vec3 pos(0.0f);
        if (registry.all_of<TransformComponent>(entity))
        {
            auto &tr = registry.get<TransformComponent>(entity);
            pos = glm::vec3(tr.GetWorldModelMatrix(registry)[3]);
        }
        else
        {
            pos = glm::vec3(0.0f, 5.0f, 0.0f);
        }
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1);
        ss << "[DIR]\nInt: " << dl.intensity << "\nCol: " << dl.diffuse.r << "," << dl.diffuse.g << "," << dl.diffuse.b;
        processLight(entity, pos, ss.str(), glm::vec3(1.0f, 0.5f, 0.0f));
    }

    for (auto &pair : m_LightLabelMap)
    {
        if (nextMap.find(pair.first) == nextMap.end())
        {
            if (registry.valid(pair.second))
            {
                registry.destroy(pair.second);
            }
        }
    }

    m_LightLabelMap = nextMap;
}

void GizmoDebugModule::ProcessKey(KeyboardManager &keyboard, Input::Key key, bool &pressedState, std::function<void()> action)
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
