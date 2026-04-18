#include <ecs/unit/core_components.h>
#include <editor/modules/physics_editor_module.h>
#include <physics/interface/i_physics_world.h>
#include <core/logic/config_manager.h>

#ifdef ENABLE_EDITOR

#include <core/app/application.h>
#include <platform/logic/input_manager.h>
#include <resource/logic/resource_manager.h>
#include <ecs/unit/render_components.h>
#include <ecs/unit/media_components.h>

#include <platform/logic/io_handler.h>
#include <platform/logic/monitor_manager.h>
#include <ecs/logic/system_manager.h>
#include <platform/interface/i_window.h>
#include <iostream>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_render_state_manager.h>
#include <render/interface/i_buffer_manager.h>
#include <render/interface/i_draw_context.h>
#include <ecs/logic/entity_manager.h>
#include <core/logic/service_locator.h>
#include <core/logic/event_manager.h>
#include <core/type/event_types.h>
PhysicsEditorModule::PhysicsEditorModule() {}
PhysicsEditorModule::~PhysicsEditorModule() {}

void PhysicsEditorModule::Initialize()
{
}

void PhysicsEditorModule::OnUpdate(float dt)
{
}

void PhysicsEditorModule::Render(Scene &scene)
{
    if (!m_Enabled)
        return;

    auto& sl = ServiceLocator::Instance();
    auto& io = sl.Require<IOHandler>();
    auto& resources = sl.Require<ResourceManager>();
    int width = io.GetMonitorManager().GetWidth();
    int height = io.GetMonitorManager().GetHeight();

    auto cm = sl.Resolve<ConfigManager>();
    bool showPhysics = false;
    bool showAudio = false;
    bool showParticle = false;
    if (cm) {
        auto& conf = cm->GetConfig();
        showPhysics = conf.debug.physicsDebug;
        showAudio = conf.debug.audioDebug;
        showParticle = conf.debug.particleDebug;
    }
    
    if (showPhysics)
    {
        EventManager::Instance().Publish(PhysicsDebugRenderEvent{&scene, width, height});
    }

    if (!showAudio && !showParticle)
        return;

    auto debugShader = resources.GetShader("debug_line");
    if (!debugShader) return;

    entt::entity camEntity = EntityManager::GetActiveCamera(scene);
    if (camEntity == entt::null) return;

    auto& cam = scene.registry.get<CameraComponent>(camEntity);
    auto* camPosComp = scene.registry.try_get<PositionComponent>(camEntity);
    glm::vec3 camPos = camPosComp ? camPosComp->value : glm::vec3(0.0f);

    float aspect = (float)width / (float)height;
    if (aspect <= 0.0f) aspect = 1.0f;
    glm::mat4 proj = glm::perspective(glm::radians(cam.fov), aspect, cam.nearPlane, cam.farPlane);
    
    auto& camRot = scene.registry.get<RotationComponent>(camEntity);
    glm::vec3 front = camRot.value * glm::vec3(0, 0, -1);
    glm::vec3 up = camRot.value * glm::vec3(0, 1, 0);
    glm::mat4 view = glm::lookAt(camPos, camPos + front, up);

    debugShader->use();
    debugShader->setMat4("projection", proj);
    debugShader->setMat4("view", view);

    std::vector<float> lineVertices;
    auto addLine = [&](const glm::vec3& start, const glm::vec3& end, const glm::vec3& color) {
        lineVertices.push_back(start.x); lineVertices.push_back(start.y); lineVertices.push_back(start.z);
        lineVertices.push_back(color.r); lineVertices.push_back(color.g); lineVertices.push_back(color.b);
        lineVertices.push_back(end.x); lineVertices.push_back(end.y); lineVertices.push_back(end.z);
        lineVertices.push_back(color.r); lineVertices.push_back(color.g); lineVertices.push_back(color.b);
    };

    if (showAudio)
    {
        auto viewAudio = scene.registry.view<AudioSourceComponent>();
        for (auto entity : viewAudio)
        {
            glm::vec3 pos(0.0f);
            if (auto* tr = scene.registry.try_get<WorldTransformComponent>(entity))
                pos = glm::vec3(tr->worldMatrix[3]);
            else if (auto* p = scene.registry.try_get<PositionComponent>(entity))
                pos = p->value;
            
            float s = 0.5f;
            glm::vec3 c(1.0f, 0.0f, 1.0f);
            addLine(pos - glm::vec3(s, 0, 0), pos + glm::vec3(s, 0, 0), c);
            addLine(pos - glm::vec3(0, s, 0), pos + glm::vec3(0, s, 0), c);
            addLine(pos - glm::vec3(0, 0, s), pos + glm::vec3(0, 0, s), c);
        }
    }

    if (showParticle)
    {
        auto viewParticle = scene.registry.view<ParticleEmitterComponent>();
        for (auto entity : viewParticle)
        {
            glm::vec3 pos(0.0f);
            if (auto* tr = scene.registry.try_get<WorldTransformComponent>(entity))
                pos = glm::vec3(tr->worldMatrix[3]);
            else if (auto* p = scene.registry.try_get<PositionComponent>(entity))
                pos = p->value;

            float s = 1.0f;
            glm::vec3 c(0.0f, 1.0f, 0.5f);
            
            // Draw a basic box around the emitter
            glm::vec3 p1 = pos + glm::vec3(-s, -s, -s);
            glm::vec3 p2 = pos + glm::vec3( s, -s, -s);
            glm::vec3 p3 = pos + glm::vec3( s,  s, -s);
            glm::vec3 p4 = pos + glm::vec3(-s,  s, -s);
            glm::vec3 p5 = pos + glm::vec3(-s, -s,  s);
            glm::vec3 p6 = pos + glm::vec3( s, -s,  s);
            glm::vec3 p7 = pos + glm::vec3( s,  s,  s);
            glm::vec3 p8 = pos + glm::vec3(-s,  s,  s);

            addLine(p1, p2, c); addLine(p2, p3, c); addLine(p3, p4, c); addLine(p4, p1, c);
            addLine(p5, p6, c); addLine(p6, p7, c); addLine(p7, p8, c); addLine(p8, p5, c);
            addLine(p1, p5, c); addLine(p2, p6, c); addLine(p3, p7, c); addLine(p4, p8, c);
        }
    }

    if (lineVertices.empty()) return;

    auto* graphics = sl.Resolve<IGraphicsContext>();
    if (!graphics) return;

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

void PhysicsEditorModule::ProcessInput(KeyboardManager &keyboard)
{
    if (!m_Enabled)
        return;

    ProcessKey(keyboard, Key::F8, m_F8Pressed, [this, &keyboard]()
               {
        bool shift = keyboard.GetKey(Key::LeftShift) || keyboard.GetKey(Key::RightShift);
        if (shift) {
            auto cm = ServiceLocator::Instance().Resolve<ConfigManager>();
            if (cm) {
                auto conf = cm->GetConfig();
                conf.debug.audioDebug = !conf.debug.audioDebug;
                cm->UpdateConfig(conf);
            }
        } else {
            TogglePhysicsDebug();
        } });

    ProcessKey(keyboard, Key::F9, m_F9Pressed, [this, &keyboard]()
               {
        bool shift = keyboard.GetKey(Key::LeftShift) || keyboard.GetKey(Key::RightShift);
        if (shift) {
            auto cm = ServiceLocator::Instance().Resolve<ConfigManager>();
            if (cm) {
                auto conf = cm->GetConfig();
                conf.debug.particleDebug = !conf.debug.particleDebug;
                cm->UpdateConfig(conf);
            }
        } });
}

void PhysicsEditorModule::TogglePhysicsDebug()
{
    auto cm = ServiceLocator::Instance().Resolve<ConfigManager>();
    if (!cm) return;
    auto conf = cm->GetConfig();
    conf.debug.physicsDebug = !conf.debug.physicsDebug;
    cm->UpdateConfig(conf);
}

void PhysicsEditorModule::ProcessKey(KeyboardManager &keyboard, Key key, bool &pressedState, std::function<void()> action)
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
