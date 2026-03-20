#include <ecs/unit/core_components.h>
#include <ecs/logic/debug/modules/gizmo_debug_module.h>
#include <render/interface/i_buffer_manager.h>
#include <render/interface/i_draw_context.h>

#ifdef ENABLE_DEBUG_SYSTEM

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <core/app/application.h>
#include <platform/logic/input_manager.h>
#include <resource/logic/resource_manager.h>
#include <scene/logic/scene.h>
#include <ecs/unit/render_components.h>
#include <ecs/unit/light_components.h>
#include <ecs/unit/ui_components.h>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_render_state_manager.h>
#include <engine/platform/logic/io_handler.h>
#include <platform/logic/monitor_manager.h>
#include <ecs/logic/debug/debug_system.h>
#include <ecs/logic/entity_manager.h>
#include <core/logic/service_locator.h>

GizmoDebugModule::GizmoDebugModule() {}
GizmoDebugModule::~GizmoDebugModule() {}

void GizmoDebugModule::Initialize()
{
}

bool GizmoDebugModule::IsEntityNamesEnabled() const { return DebugConfig::ShowEntityNames; }
bool GizmoDebugModule::IsTransformGizmosEnabled() const { return DebugConfig::ShowGizmos; }
bool GizmoDebugModule::IsLightGizmosEnabled() const { return DebugConfig::ShowLightGizmos; }

void GizmoDebugModule::ToggleEntityNames() { DebugConfig::ShowEntityNames = !DebugConfig::ShowEntityNames; }
void GizmoDebugModule::ToggleTransformGizmos() { DebugConfig::ShowGizmos = !DebugConfig::ShowGizmos; }
void GizmoDebugModule::ToggleLightGizmos() { DebugConfig::ShowLightGizmos = !DebugConfig::ShowLightGizmos; }

void GizmoDebugModule::SetSharedResources(std::shared_ptr<Font> font, std::shared_ptr<Shader> shader, std::shared_ptr<UIModel> quad)
{
    m_DebugFont = font;
    m_TextShader = shader;
    m_TextQuad = quad;
}

void GizmoDebugModule::OnUpdate(float dt)
{
    if (!m_Enabled)
        return;

    auto& scene = ServiceLocator::Instance().Require<Scene>();
    UpdateDebugLabels(scene);
    UpdateLightLabels(scene);
}

void GizmoDebugModule::Render(Scene &scene)
{
    if (!m_Enabled || !DebugConfig::ShowGizmos)
        return;

    auto& sl = ServiceLocator::Instance();
    auto& io = sl.Require<IOHandler>();
    auto& resources = sl.Require<ResourceManager>();
    auto& graphics = sl.Require<IGraphicsContext>();

    int width = io.GetMonitorManager().GetWidth();
    int height = io.GetMonitorManager().GetHeight();

    auto debugShader = resources.GetShader("debugLine");
    if (!debugShader) return;

    entt::entity camEntity = EntityManager::GetActiveCamera(scene);
    if (camEntity == entt::null) return;

    auto& cam = scene.registry.get<CameraComponent>(camEntity);
    auto& camPos = scene.registry.get<PositionComponent>(camEntity);

    float aspect = (float)width / (float)height;
    glm::mat4 proj = glm::perspective(glm::radians(cam.fov), aspect, cam.nearPlane, cam.farPlane);
    glm::mat4 view = glm::lookAt(camPos.value, camPos.value + cam.front, cam.worldUp);

    debugShader->use();
    debugShader->setMat4("projection", proj);
    debugShader->setMat4("view", view);
    debugShader->setMat4("model", glm::mat4(1.0f));

    auto viewEntities = scene.registry.view<WorldTransformComponent>();

    std::vector<float> lineVertices;
    auto addLine = [&](const glm::vec3& start, const glm::vec3& end, const glm::vec3& color) {
        lineVertices.push_back(start.x); lineVertices.push_back(start.y); lineVertices.push_back(start.z);
        lineVertices.push_back(color.r); lineVertices.push_back(color.g); lineVertices.push_back(color.b);
        lineVertices.push_back(end.x); lineVertices.push_back(end.y); lineVertices.push_back(end.z);
        lineVertices.push_back(color.r); lineVertices.push_back(color.g); lineVertices.push_back(color.b);
    };

    for (auto entity : viewEntities)
    {
        auto& tr = viewEntities.get<WorldTransformComponent>(entity);
        glm::mat4 modelMatrix = tr.worldMatrix;

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

    auto& bm = graphics.GetBufferManager();
    auto& dc = graphics.GetDrawContext();

    unsigned int vao = bm.GenVertexArray();
    unsigned int vbo = bm.GenBuffer();
    
    bm.BindVertexArray(vao);
    bm.BindBuffer(BufferType::ArrayBuffer, vbo);
    bm.BufferData(BufferType::ArrayBuffer, lineVertices.size() * sizeof(float), lineVertices.data(), BufferUsage::StreamDraw);

    bm.EnableVertexAttribArray(0);
    bm.VertexAttribPointer(0, 3, DataType::Float, false, 6 * sizeof(float), (void*)0);
    bm.EnableVertexAttribArray(1);
    bm.VertexAttribPointer(1, 3, DataType::Float, false, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    graphics.GetRenderStateManager().Disable(ServerCapability::DepthTest);
    dc.DrawArrays(Primitive::Lines, 0, (int)(lineVertices.size() / 6));
    graphics.GetRenderStateManager().Enable(ServerCapability::DepthTest);

    bm.BindVertexArray(0);
    bm.DeleteVertexArray(vao);
    bm.DeleteBuffer(vbo);
}

void GizmoDebugModule::ProcessInput(KeyboardManager &keyboard)
{
    if (!m_Enabled)
        return;

    ProcessKey(keyboard, Key::F3, m_F3Pressed, [this, &keyboard]()
               {
        bool shift = keyboard.GetKey(Key::LeftShift) || keyboard.GetKey(Key::RightShift);
        if (shift) {
            ToggleEntityNames();
            std::cout << "\n========== Entity Names (Shift+F3) ==========" << std::endl;
            std::cout << "[Debug] Entity Names: " << (DebugConfig::ShowEntityNames ? "ON" : "OFF") << std::endl;
            std::cout << "=============================================" << std::endl;
        } });

    ProcessKey(keyboard, Key::F4, m_F4Pressed, [this, &keyboard]()
               {
        bool shift = keyboard.GetKey(Key::LeftShift) || keyboard.GetKey(Key::RightShift);
        if (shift) {
            DebugConfig::ShowGizmos = !DebugConfig::ShowGizmos;
            std::cout << "\n========== Transform Gizmos (Shift+F4) ==========" << std::endl;
            std::cout << "[Debug] Transform Gizmos: " << (DebugConfig::ShowGizmos ? "ON" : "OFF") << std::endl;
            std::cout << "=================================================" << std::endl;
        } });

    ProcessKey(keyboard, Key::F5, m_F5Pressed, [this, &keyboard]()
               {
        bool shift = keyboard.GetKey(Key::LeftShift) || keyboard.GetKey(Key::RightShift);
        if (shift) {
            ToggleLightGizmos();

            auto& scene = ServiceLocator::Instance().Require<Scene>();
            auto& reg = scene.registry;
            int p = 0; for(auto e : reg.view<PointLightComponent>()) p++;
            int s = 0; for(auto e : reg.view<SpotLightComponent>()) s++;

            int d_total = 0;
            int d_transform = 0;
            for(auto e : reg.view<DirectionalLightComponent>()) {
                d_total++;
                if(reg.all_of<PositionComponent>(e)) d_transform++;
            }

            std::cout << "\n========== Light Gizmos (Shift+F5) ==========" << std::endl;
            std::cout << "[Debug] Light Gizmos: " << (DebugConfig::ShowLightGizmos ? "ON" : "OFF") << std::endl;
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
    if (!DebugConfig::ShowEntityNames)
    {
        if (!m_EntityLabelMap.empty())
        {
            ClearDebugLabels(scene);
        }
        return;
    }

    auto &registry = scene.registry;
    auto& io = ServiceLocator::Instance().Require<IOHandler>();
    int width = io.GetMonitorManager().GetWidth();
    int height = io.GetMonitorManager().GetHeight();

    entt::entity camEntity = EntityManager::GetActiveCamera(scene);

    glm::mat4 vp = glm::mat4(1.0f);
    if (registry.valid(camEntity) && registry.all_of<CameraComponent, PositionComponent>(camEntity))
    {
        auto &cam = registry.get<CameraComponent>(camEntity);
        auto &camPos = registry.get<PositionComponent>(camEntity);

        float aspect = (float)width / (float)height;
        glm::mat4 proj = glm::perspective(glm::radians(cam.fov), aspect, cam.nearPlane, cam.farPlane);
        glm::mat4 view = glm::lookAt(camPos.value, camPos.value + cam.front, cam.worldUp);
        vp = proj * view;
    }
    else
    {
        return;
    }

    auto view = registry.view<InfoComponent, WorldTransformComponent>();

    std::unordered_map<entt::entity, entt::entity> nextMap;

    for (auto entity : view)
    {
        if (entity == camEntity)
            continue;

        auto &entityInfo = view.get<InfoComponent>(entity);
        if (entityInfo.tag == "DebugLabel" || entityInfo.tag == "DebugLight")
            continue;

        // Skip UI entities aggressively
        if (registry.any_of<UITransformComponent>(entity) || 
            registry.any_of<UIRendererComponent>(entity) || 
            registry.any_of<UITextComponent>(entity))
            continue;

        entt::entity labelEntity = entt::null;
        if (m_EntityLabelMap.find(entity) != m_EntityLabelMap.end())
        {
            labelEntity = m_EntityLabelMap[entity];
            if (!registry.valid(labelEntity))
                labelEntity = entt::null;
        }

        auto &tr = view.get<WorldTransformComponent>(entity);

        glm::mat4 modelMatrix = tr.worldMatrix;
        glm::vec3 labelPos;
        bool usedAABB = false;

        if (registry.all_of<MeshRendererComponent>(entity))
        {
            auto &mrc = registry.get<MeshRendererComponent>(entity);
            if (mrc.model)
            {
                glm::vec3 localTop = glm::vec3(
                    (mrc.model->aabb.minBound.x + mrc.model->aabb.maxBound.x) * 0.5f,
                    mrc.model->aabb.maxBound.y,
                    (mrc.model->aabb.minBound.z + mrc.model->aabb.maxBound.z) * 0.5f);
                labelPos = glm::vec3(modelMatrix * glm::vec4(localTop, 1.0f));
                labelPos.y += 0.2f;
                usedAABB = true;
            }
        }

        if (!usedAABB)
        {
            labelPos = glm::vec3(modelMatrix[3]);
            labelPos.y += 1.0f;
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
                labelEntity = EntityManager::CreateEntity(scene, "Label_" + entityInfo.name, "DebugLabel");
                registry.emplace<UITransformComponent>(labelEntity);
                auto &text = registry.emplace<UITextComponent>(labelEntity);

                text.model = m_TextQuad;
                text.shader = m_TextShader;
                text.font = m_DebugFont;
                text.text = entityInfo.name;
                text.color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
                text.scale = 2.0f;
            }

            auto &uiTr = registry.get<UITransformComponent>(labelEntity);
            auto &text = registry.get<UITextComponent>(labelEntity);

            float textW = 0.0f;
            if (m_DebugFont)
                textW = (float)text.text.length() * 11.0f * text.scale;

            uiTr.anchorMin = glm::vec2(0.0f);
            uiTr.anchorMax = glm::vec2(0.0f);
            uiTr.offsetMin = screenPos - glm::vec2(textW / 2.0f, 0.0f);
            uiTr.offsetMax = uiTr.offsetMin + glm::vec2(textW, 20.0f * text.scale);
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
    if (!DebugConfig::ShowLightGizmos)
    {
        if (!m_LightLabelMap.empty())
        {
            ClearLightLabels(scene);
        }
        return;
    }

    auto &registry = scene.registry;
    auto& io = ServiceLocator::Instance().Require<IOHandler>();
    int width = io.GetMonitorManager().GetWidth();
    int height = io.GetMonitorManager().GetHeight();

    entt::entity camEntity = EntityManager::GetActiveCamera(scene);

    glm::mat4 vp = glm::mat4(1.0f);
    if (registry.valid(camEntity) && registry.all_of<CameraComponent, PositionComponent>(camEntity))
    {
        auto &cam = registry.get<CameraComponent>(camEntity);
        auto &camPos = registry.get<PositionComponent>(camEntity);

        float aspect = (float)width / (float)height;
        glm::mat4 proj = glm::perspective(glm::radians(cam.fov), aspect, cam.nearPlane, cam.farPlane);
        glm::mat4 view = glm::lookAt(camPos.value, camPos.value + cam.front, cam.worldUp);
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
                labelEntity = EntityManager::CreateEntity(scene, "LightLabel_" + typeName, "DebugLabel");
                registry.emplace<UITransformComponent>(labelEntity);
                auto &text = registry.emplace<UITextComponent>(labelEntity);

                text.model = m_TextQuad;
                text.shader = m_TextShader;
                text.font = m_DebugFont;
                text.text = typeName;
                text.color = glm::vec4(color, 1.0f);
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

            uiTr.anchorMin = glm::vec2(0.0f);
            uiTr.anchorMax = glm::vec2(0.0f);
            uiTr.offsetMin = screenPos - glm::vec2(textW / 2.0f, 0.0f);
            uiTr.offsetMax = uiTr.offsetMin + glm::vec2(textW, 20.0f * text.scale);
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

    auto pointLights = registry.view<PointLightComponent, PositionComponent>();
    for (auto entity : pointLights)
    {
        auto [pl, tr] = pointLights.get<PointLightComponent, PositionComponent>(entity);
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1);
        ss << "[POINT]\nInt: " << pl.intensity << "\nCol: " << pl.color.r << "," << pl.color.g << "," << pl.color.b;
        processLight(entity, tr.value, ss.str(), glm::vec3(1.0f, 1.0f, 0.0f));
    }

    auto spotLights = registry.view<SpotLightComponent, PositionComponent>();
    for (auto entity : spotLights)
    {
        auto [sl, tr] = spotLights.get<SpotLightComponent, PositionComponent>(entity);
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1);
        ss << "[SPOT]\nInt: " << sl.intensity << "\nCol: " << sl.color.r << "," << sl.color.g << "," << sl.color.b;
        processLight(entity, tr.value, ss.str(), glm::vec3(0.0f, 1.0f, 1.0f));
    }

    auto dirLights = registry.view<DirectionalLightComponent>();
    for (auto entity : dirLights)
    {
        auto& dl = registry.get<DirectionalLightComponent>(entity);
        glm::vec3 pos(0.0f);
        if (registry.all_of<PositionComponent>(entity))
        {
            auto &tr = registry.get<PositionComponent>(entity);
            pos = tr.value;
        }
        else
        {
            pos = glm::vec3(0.0f, 5.0f, 0.0f);
        }
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1);
        ss << "[DIR]\nInt: " << dl.intensity << "\nCol: " << dl.color.r << "," << dl.color.g << "," << dl.color.b;
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

void GizmoDebugModule::ProcessKey(KeyboardManager &keyboard, Key key, bool &pressedState, std::function<void()> action)
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
