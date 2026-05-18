#include <ecs/unit/core_components.h>
#include <editor/modules/gizmo_editor_module.h>
#include <render/interface/i_buffer_manager.h>
#include <render/interface/i_draw_context.h>

#ifdef ENABLE_EDITOR

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
#include <editor/editor_system.h>
#include <ecs/logic/entity_manager.h>
#include <core/logic/service_locator.h>

GizmoEditorModule::GizmoEditorModule() {}
GizmoEditorModule::~GizmoEditorModule() {}

void GizmoEditorModule::Initialize()
{
}

#include <core/logic/config_manager.h>

bool GizmoEditorModule::IsEntityNamesEnabled() const {
    auto cm = ServiceLocator::Instance().Resolve<ConfigManager>();
    return cm ? cm->GetConfig().debug.entityNames : false;
}
bool GizmoEditorModule::IsTransformGizmosEnabled() const {
    auto cm = ServiceLocator::Instance().Resolve<ConfigManager>();
    return cm ? cm->GetConfig().debug.gizmos : false;
}
bool GizmoEditorModule::IsLightGizmosEnabled() const {
    auto cm = ServiceLocator::Instance().Resolve<ConfigManager>();
    return cm ? cm->GetConfig().debug.lightGizmos : false;
}

void GizmoEditorModule::ToggleEntityNames() {
    auto cm = ServiceLocator::Instance().Resolve<ConfigManager>();
    if (cm) { auto c = cm->GetConfig(); c.debug.entityNames = !c.debug.entityNames; cm->UpdateConfig(c); }
}
void GizmoEditorModule::ToggleTransformGizmos() {
    auto cm = ServiceLocator::Instance().Resolve<ConfigManager>();
    if (cm) { auto c = cm->GetConfig(); c.debug.gizmos = !c.debug.gizmos; cm->UpdateConfig(c); }
}
void GizmoEditorModule::ToggleLightGizmos() {
    auto cm = ServiceLocator::Instance().Resolve<ConfigManager>();
    if (cm) { auto c = cm->GetConfig(); c.debug.lightGizmos = !c.debug.lightGizmos; cm->UpdateConfig(c); }
}

void GizmoEditorModule::SetSharedResources(std::shared_ptr<Font> font, std::shared_ptr<Shader> shader, std::shared_ptr<UIModel> quad)
{
    m_DebugFont = font;
    m_TextShader = shader;
    m_TextQuad = quad;
}

void GizmoEditorModule::OnUpdate(float dt)
{
    if (!m_Enabled)
        return;

    auto& sl = ServiceLocator::Instance();
    auto* scene = sl.Resolve<Scene>();
    if (!scene) return;

    UpdateDebugLabels(*scene);
    UpdateLightLabels(*scene);
}

void GizmoEditorModule::Render(Scene &scene)
{
    if (!m_Enabled || !IsTransformGizmosEnabled())
        return;

    auto& sl = ServiceLocator::Instance();
    auto* io = sl.Resolve<IOHandler>();
    auto* resources = sl.Resolve<ResourceManager>();
    auto* graphics = sl.Resolve<IGraphicsContext>();
    if (!io || !resources || !graphics) return;

    int width = io->GetMonitorManager().GetWidth();
    int height = io->GetMonitorManager().GetHeight();

    auto debugShader = resources->GetShader("debug_line");
    if (!debugShader) return;

    entt::entity camEntity = EntityManager::GetActiveCamera(scene);
    if (camEntity == entt::null) return;

    auto& cam = scene.registry.get<CameraComponent>(camEntity);
    auto* camPosComp = scene.registry.try_get<PositionComponent>(camEntity);
    glm::vec3 camPos = camPosComp ? camPosComp->value : glm::vec3(0.0f);

    float aspect = (float)width / (float)height;
    glm::mat4 proj = glm::perspective(glm::radians(cam.fov), aspect, cam.nearPlane, cam.farPlane);
    auto& camRot = scene.registry.get<RotationComponent>(camEntity);
    glm::vec3 front = camRot.value * glm::vec3(0, 0, -1);
    glm::vec3 up = camRot.value * glm::vec3(0, 1, 0);
    glm::mat4 view = glm::lookAt(camPos, camPos + front, up);

    debugShader->use();
    debugShader->setMat4("projection", proj);
    debugShader->setMat4("view", view);

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
        

        float lenR = glm::length(glm::vec3(modelMatrix[0]));
        float lenU = glm::length(glm::vec3(modelMatrix[1]));
        float lenF = glm::length(glm::vec3(modelMatrix[2]));

        if (lenR < 0.0001f || lenU < 0.0001f || lenF < 0.0001f)
            continue;

        glm::vec3 right = glm::normalize(glm::vec3(modelMatrix[0]));
        glm::vec3 up = glm::normalize(glm::vec3(modelMatrix[1]));
        glm::vec3 forward = glm::normalize(glm::vec3(modelMatrix[2]));

        float length = 1.0f;
        addLine(pos, pos + right * length, glm::vec3(1.0f, 0.0f, 0.0f));
        addLine(pos, pos + up * length, glm::vec3(0.0f, 1.0f, 0.0f));
        addLine(pos, pos + forward * length, glm::vec3(0.0f, 0.0f, 1.0f));
    }

    if (lineVertices.empty()) return;

    auto& bm = graphics->GetBufferManager();
    auto& dc = graphics->GetDrawContext();

    if (m_LineVAO == 0) m_LineVAO = bm.GenVertexArray();
    if (m_LineVBO == 0) m_LineVBO = bm.GenBuffer();
    
    bm.BindVertexArray(m_LineVAO);
    bm.BindBuffer(BufferType::ArrayBuffer, m_LineVBO);
    bm.BufferData(BufferType::ArrayBuffer, lineVertices.size() * sizeof(float), lineVertices.data(), BufferUsage::StreamDraw);

    bm.EnableVertexAttribArray(0);
    bm.VertexAttribPointer(0, 3, DataType::Float, false, 6 * sizeof(float), (void*)0);
    bm.EnableVertexAttribArray(1);
    bm.VertexAttribPointer(1, 3, DataType::Float, false, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    graphics->GetRenderStateManager().Disable(ServerCapability::DepthTest);
    dc.DrawArrays(Primitive::Lines, 0, (int)(lineVertices.size() / 6));
    graphics->GetRenderStateManager().Enable(ServerCapability::DepthTest);

    bm.BindVertexArray(0);
}

void GizmoEditorModule::ProcessInput(KeyboardManager &keyboard)
{
}

void GizmoEditorModule::ClearDebugLabels(Scene &scene)
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

void GizmoEditorModule::UpdateDebugLabels(Scene &scene)
{
    if (!IsEntityNamesEnabled())
    {
        if (!m_EntityLabelMap.empty())
        {
            ClearDebugLabels(scene);
        }
        return;
    }

    auto& sl = ServiceLocator::Instance();
    auto* io = sl.Resolve<IOHandler>();
    if (!io) return;

    int width = io->GetMonitorManager().GetWidth();
    int height = io->GetMonitorManager().GetHeight();

    const float REF_WIDTH = 1920.0f;
    const float REF_HEIGHT = 1080.0f;
    float scaleFactor = std::min((float)width / REF_WIDTH, (float)height / REF_HEIGHT);
    if (scaleFactor < 0.0001f) scaleFactor = 1.0f;

    auto &registry = scene.registry;
    entt::entity camEntity = EntityManager::GetActiveCamera(scene);

    glm::mat4 vp = glm::mat4(1.0f);
    if (registry.valid(camEntity) && registry.all_of<CameraComponent, PositionComponent>(camEntity))
    {
        auto &cam = registry.get<CameraComponent>(camEntity);
        auto &camPos = registry.get<PositionComponent>(camEntity);

        float aspect = (float)width / (float)height;
        glm::mat4 proj = glm::perspective(glm::radians(cam.fov), aspect, cam.nearPlane, cam.farPlane);
        
        auto& camRot = registry.get<RotationComponent>(camEntity);
        glm::vec3 front = camRot.value * glm::vec3(0, 0, -1);
        glm::vec3 up = camRot.value * glm::vec3(0, 1, 0);
        glm::mat4 view = glm::lookAt(camPos.value, camPos.value + front, up);
        vp = proj * view;
    }
    else
    {
        return;
    }

    std::vector<entt::entity> toDestroy;

    auto view = registry.view<InfoComponent>();

    std::unordered_map<entt::entity, entt::entity> nextMap;
    std::vector<std::pair<entt::entity, std::string>> pendingLabels;

    for (auto entity : view)
    {
        if (entity == camEntity)
            continue;

        auto &entityInfo = view.get<InfoComponent>(entity);
        if (entityInfo.tag == "DebugLabel" || entityInfo.tag == "DebugLight")
            continue;


        if (registry.any_of<UITransformComponent>(entity) || 
            registry.any_of<UIRendererComponent>(entity) || 
            registry.any_of<UITextComponent>(entity))
            continue;

        entt::entity labelEntity = entt::null;
        auto it = m_EntityLabelMap.find(entity);
        if (it != m_EntityLabelMap.end())
        {
            labelEntity = it->second;
            if (!registry.valid(labelEntity))
                labelEntity = entt::null;
        }

        glm::mat4 modelMatrix = glm::mat4(1.0f);
        if (auto* tr = registry.try_get<WorldTransformComponent>(entity)) {
            modelMatrix = tr->worldMatrix;
        } else if (registry.all_of<PositionComponent, RotationComponent, ScaleComponent>(entity)) {
            auto& pos = registry.get<PositionComponent>(entity);
            auto& rot = registry.get<RotationComponent>(entity);
            auto& scale = registry.get<ScaleComponent>(entity);
            modelMatrix = glm::translate(glm::mat4(1.0f), pos.value) *
                          glm::mat4_cast(rot.value) *
                          glm::scale(glm::mat4(1.0f), scale.value);
        }
        
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
            if (modelMatrix == glm::mat4(1.0f) && registry.any_of<PositionComponent>(entity)) {
                labelPos = registry.get<PositionComponent>(entity).value;
            } else {
                labelPos = glm::vec3(modelMatrix[3]);
            }
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
                screenPos.x = (ndc.x * 0.5f + 0.5f) * (float)width;
                screenPos.y = (1.0f - (ndc.y * 0.5f + 0.5f)) * (float)height;
                visible = true;
            }
        }

        if (visible)
        {
            if (labelEntity == entt::null)
            {
                pendingLabels.push_back({entity, entityInfo.name});
            }
            else
            {
                auto &uiTr = registry.get<UITransformComponent>(labelEntity);
                auto &text = registry.get<UITextComponent>(labelEntity);

                float textW = 0.0f;
                if (m_DebugFont)
                    textW = (float)text.text.length() * 11.0f * text.scale;

                uiTr.anchorMin = glm::vec2(0.0f);
                uiTr.anchorMax = glm::vec2(0.0f);
                uiTr.offsetMin = (screenPos / scaleFactor) - glm::vec2(textW / 2.0f, 0.0f);
                uiTr.offsetMax = uiTr.offsetMin + glm::vec2(textW, 20.0f * text.scale);
                uiTr.zIndex = 100;

                nextMap[entity] = labelEntity;
            }
        }
        else
        {
            if (labelEntity != entt::null)
            {
                toDestroy.push_back(labelEntity);
            }
        }
    }

    for (auto e : toDestroy)
    {
        if (registry.valid(e))
            registry.destroy(e);
    }
    toDestroy.clear();


    for (auto& pending : pendingLabels)
    {
        entt::entity labelEntity = EntityManager::CreateEntity(scene, "Label_" + pending.second, "DebugLabel");
        registry.emplace<UITransformComponent>(labelEntity);
        auto &text = registry.emplace<UITextComponent>(labelEntity);

        text.model = m_TextQuad;
        text.shader = m_TextShader;
        text.font = m_DebugFont;
        text.text = pending.second;
        text.color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
        text.scale = 2.0f;
        

        nextMap[pending.first] = labelEntity;
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

void GizmoEditorModule::ClearLightLabels(Scene &scene)
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

void GizmoEditorModule::UpdateLightLabels(Scene &scene)
{
    if (!IsLightGizmosEnabled())
    {
        if (!m_LightLabelMap.empty())
        {
            ClearLightLabels(scene);
        }
        return;
    }

    auto& sl = ServiceLocator::Instance();
    auto* io = sl.Resolve<IOHandler>();
    if (!io) return;

    int width = io->GetMonitorManager().GetWidth();
    int height = io->GetMonitorManager().GetHeight();

    const float REF_WIDTH = 1920.0f;
    const float REF_HEIGHT = 1080.0f;
    float scaleFactor = std::min((float)width / REF_WIDTH, (float)height / REF_HEIGHT);
    if (scaleFactor < 0.0001f) scaleFactor = 1.0f;

    auto &registry = scene.registry;
    entt::entity camEntity = EntityManager::GetActiveCamera(scene);

    glm::mat4 vp = glm::mat4(1.0f);
    if (registry.valid(camEntity) && registry.all_of<CameraComponent, PositionComponent>(camEntity))
    {
        auto &cam = registry.get<CameraComponent>(camEntity);
        auto &camPos = registry.get<PositionComponent>(camEntity);

        float aspect = (float)width / (float)height;
        glm::mat4 proj = glm::perspective(glm::radians(cam.fov), aspect, cam.nearPlane, cam.farPlane);
        auto& camRot = registry.get<RotationComponent>(camEntity);
        glm::vec3 front = camRot.value * glm::vec3(0, 0, -1);
        glm::vec3 up = camRot.value * glm::vec3(0, 1, 0);
        glm::mat4 view = glm::lookAt(camPos.value, camPos.value + front, up);
        vp = proj * view;
    }
    else
    {
        return;
    }

    std::vector<entt::entity> toDestroy;

    std::unordered_map<entt::entity, entt::entity> nextMap;

    struct PendingLight { entt::entity entity; glm::vec3 pos; std::string typeName; glm::vec3 color; };
    std::vector<PendingLight> pendingLights;

    auto processLight = [&](entt::entity entity, const glm::vec3 &pos, const std::string &typeName, const glm::vec3 &color)
    {
        entt::entity labelEntity = entt::null;
        auto it = m_LightLabelMap.find(entity);
        if (it != m_LightLabelMap.end())
        {
            labelEntity = it->second;
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
                screenPos.x = (ndc.x * 0.5f + 0.5f) * (float)width;
                screenPos.y = (1.0f - (ndc.y * 0.5f + 0.5f)) * (float)height;
                visible = true;
            }
        }

        if (visible)
        {
            if (labelEntity == entt::null)
            {
                pendingLights.push_back({entity, pos, typeName, color});
            }
            else
            {
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
                uiTr.offsetMin = (screenPos / scaleFactor) - glm::vec2(textW / 2.0f, 0.0f);
                uiTr.offsetMax = uiTr.offsetMin + glm::vec2(textW, 20.0f * text.scale);
                uiTr.zIndex = 90;

                nextMap[entity] = labelEntity;
            }
        }
        else
        {
            if (labelEntity != entt::null)
            {
                toDestroy.push_back(labelEntity);
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

    for (auto e : toDestroy)
    {
        if (registry.valid(e))
            registry.destroy(e);
    }
    toDestroy.clear();

    for (auto& pending : pendingLights)
    {
        entt::entity labelEntity = EntityManager::CreateEntity(scene, "LightLabel_" + pending.typeName, "DebugLabel");
        registry.emplace<UITransformComponent>(labelEntity);
        auto &text = registry.emplace<UITextComponent>(labelEntity);

        text.model = m_TextQuad;
        text.shader = m_TextShader;
        text.font = m_DebugFont;
        text.text = pending.typeName;
        text.color = glm::vec4(pending.color, 1.0f);
        text.scale = 2.0f;

        nextMap[pending.entity] = labelEntity;
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

void GizmoEditorModule::ProcessKey(KeyboardManager &keyboard, Key key, bool &pressedState, std::function<void()> action)
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
