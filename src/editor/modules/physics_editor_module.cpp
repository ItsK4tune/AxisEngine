#include <editor/modules/physics_editor_module.h>
#include <core/logic/config_manager.h>
#include <ecs/unit/core_components.h>
#include <editor/panels/scene_hierarchy_panel.h>
#include <physics/interface/i_physics_world.h>

#ifdef ENABLE_EDITOR

#include <core/app/application.h>
#include <ecs/logic/system_manager.h>
#include <ecs/unit/media_components.h>
#include <ecs/unit/render_components.h>
#include <platform/interface/i_window.h>
#include <platform/logic/input_manager.h>
#include <platform/logic/io_handler.h>
#include <platform/logic/monitor_manager.h>
#include <resource/logic/resource_manager.h>
#include <iostream>

#define GLM_ENABLE_EXPERIMENTAL
#include <core/logic/event_manager.h>
#include <core/logic/service_locator.h>
#include <core/type/event_types.h>
#include <render/interface/i_buffer_manager.h>
#include <render/interface/i_draw_context.h>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_render_state_manager.h>
#include <glm/glm.hpp>

PhysicsEditorModule::PhysicsEditorModule()
{
}
PhysicsEditorModule::~PhysicsEditorModule()
{
}

void PhysicsEditorModule::Initialize()
{
}

void PhysicsEditorModule::OnUpdate(float dt)
{
}

void PhysicsEditorModule::Render(Scene& scene)
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
    if (cm)
    {
        auto conf = cm->GetConfig();
        showPhysics = conf.debug.physicsDebug;
        showAudio = conf.debug.audioDebug;
        showParticle = conf.debug.particleDebug;
    }

    if (showPhysics)
    {
        EventManager::Instance().Publish(PhysicsDebugRenderEvent{&scene, width, height});
    }

    bool showGrid = false;
    if (cm)
    {
        showGrid = cm->GetConfig().debug.gridIndicatorEnabled;
    }

    if (!showAudio && !showParticle && !showGrid)
        return;

    auto debugShader = resources.GetShader("debug_line");
    if (!debugShader)
        return;

    entt::entity camEntity = scene.GetActiveCamera();
    if (camEntity == entt::null)
        return;

    auto& cam = scene.GetComponent<CameraComponent>(camEntity);
    auto* camPosComp = scene.TryGetComponent<PositionComponent>(camEntity);
    glm::vec3 camPos = camPosComp ? camPosComp->value : glm::vec3(0.0f);

    float aspect = (float)width / (float)height;
    if (aspect <= 0.0f)
        aspect = 1.0f;
    glm::mat4 proj = glm::perspective(glm::radians(cam.fov), aspect, cam.nearPlane, cam.farPlane);

    auto& camRot = scene.GetComponent<RotationComponent>(camEntity);
    glm::vec3 front = camRot.value * glm::vec3(0, 0, -1);
    glm::vec3 up = camRot.value * glm::vec3(0, 1, 0);
    glm::mat4 view = glm::lookAt(camPos, camPos + front, up);

    debugShader->use();
    debugShader->setMat4("projection", proj);
    debugShader->setMat4("view", view);

    std::vector<float> lineVertices;
    auto addLine = [&](const glm::vec3& start, const glm::vec3& end, const glm::vec3& color) {
        lineVertices.push_back(start.x);
        lineVertices.push_back(start.y);
        lineVertices.push_back(start.z);
        lineVertices.push_back(color.r);
        lineVertices.push_back(color.g);
        lineVertices.push_back(color.b);
        lineVertices.push_back(end.x);
        lineVertices.push_back(end.y);
        lineVertices.push_back(end.z);
        lineVertices.push_back(color.r);
        lineVertices.push_back(color.g);
        lineVertices.push_back(color.b);
    };

    if (showAudio)
    {
        auto viewAudio = scene.View<AudioSourceComponent>();
        for (auto entity : viewAudio)
        {
            glm::vec3 pos(0.0f);
            if (auto* tr = scene.TryGetComponent<WorldTransformComponent>(entity))
                pos = glm::vec3(tr->worldMatrix[3]);
            else if (auto* p = scene.TryGetComponent<PositionComponent>(entity))
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
        auto viewParticle = scene.View<ParticleEmitterComponent>();
        for (auto entity : viewParticle)
        {
            glm::vec3 pos(0.0f);
            if (auto* tr = scene.TryGetComponent<WorldTransformComponent>(entity))
                pos = glm::vec3(tr->worldMatrix[3]);
            else if (auto* p = scene.TryGetComponent<PositionComponent>(entity))
                pos = p->value;

            float s = 1.0f;
            glm::vec3 c(0.0f, 1.0f, 0.5f);

            // Draw a basic box around the emitter
            glm::vec3 p1 = pos + glm::vec3(-s, -s, -s);
            glm::vec3 p2 = pos + glm::vec3(s, -s, -s);
            glm::vec3 p3 = pos + glm::vec3(s, s, -s);
            glm::vec3 p4 = pos + glm::vec3(-s, s, -s);
            glm::vec3 p5 = pos + glm::vec3(-s, -s, s);
            glm::vec3 p6 = pos + glm::vec3(s, -s, s);
            glm::vec3 p7 = pos + glm::vec3(s, s, s);
            glm::vec3 p8 = pos + glm::vec3(-s, s, s);

            addLine(p1, p2, c);
            addLine(p2, p3, c);
            addLine(p3, p4, c);
            addLine(p4, p1, c);
            addLine(p5, p6, c);
            addLine(p6, p7, c);
            addLine(p7, p8, c);
            addLine(p8, p5, c);
            addLine(p1, p5, c);
            addLine(p2, p6, c);
            addLine(p3, p7, c);
            addLine(p4, p8, c);
        }
    }

    // Draw 3D Grid Indicator reflecting Grid Snap step centered at Camera (snapped chunks)
    if (auto* cm = ServiceLocator::Instance().Resolve<ConfigManager>())
    {
        auto conf = cm->GetConfig();
        if (conf.debug.gridIndicatorEnabled)
        {
            float tSnap = conf.debug.gridSnapTranslation;
            if (tSnap < 0.1f)
                tSnap = 0.1f;  // Safety clamp

            float extent = 5000.0f; // Looks infinite
            float minorExtent = 200.0f;
            
            float minorStep = tSnap;
            float majorStep = tSnap * 10.0f;
            if (majorStep < 10.0f)
                majorStep = 10.0f;

            glm::vec3 gridColorMajor(0.20f, 0.20f, 0.20f);
            glm::vec3 gridColorMinor(0.08f, 0.08f, 0.08f); // Fainter minor lines
            glm::vec3 axisColorX(0.7f, 0.2f, 0.2f);  // Red for X-axis
            glm::vec3 axisColorZ(0.2f, 0.2f, 0.7f);  // Blue for Z-axis
            glm::vec3 axisColorY(0.2f, 0.7f, 0.2f);  // Green for Y-axis

            // 1. Draw horizontal XZ Major Grid (extent = 5000)
            int halfMajorLines = static_cast<int>(extent / majorStep);
            for (int i = -halfMajorLines; i <= halfMajorLines; ++i)
            {
                float val = i * majorStep;
                
                // Lines parallel to Z
                glm::vec3 startZ(val, 0.0f, -extent);
                glm::vec3 endZ(val, 0.0f, extent);
                glm::vec3 colorZ = (std::abs(val) < 0.001f) ? axisColorZ : gridColorMajor;
                addLine(startZ, endZ, colorZ);

                // Lines parallel to X
                glm::vec3 startX(-extent, 0.0f, val);
                glm::vec3 endX(extent, 0.0f, val);
                glm::vec3 colorX = (std::abs(val) < 0.001f) ? axisColorX : gridColorMajor;
                addLine(startX, endX, colorX);
            }

            // 2. Draw horizontal XZ Minor Grid (extent = 200) centered at camera
            int halfMinorLines = static_cast<int>(minorExtent / minorStep);
            float snapX = std::round(camPos.x / minorStep) * minorStep;
            float snapZ = std::round(camPos.z / minorStep) * minorStep;
            for (int i = -halfMinorLines; i <= halfMinorLines; ++i)
            {
                float x = snapX + i * minorStep;
                float z = snapZ + i * minorStep;

                // Lines parallel to Z
                glm::vec3 startZ(x, 0.0f, snapZ - minorExtent);
                glm::vec3 endZ(x, 0.0f, snapZ + minorExtent);
                if (std::abs(x) > 0.001f) // Skip axis lines
                    addLine(startZ, endZ, gridColorMinor);

                // Lines parallel to X
                glm::vec3 startX(snapX - minorExtent, 0.0f, z);
                glm::vec3 endX(snapX + minorExtent, 0.0f, z);
                if (std::abs(z) > 0.001f)
                    addLine(startX, endX, gridColorMinor);
            }

            // 3. Draw vertical XY Grid at Z = 0 (extent = 5000, step = majorStep)
            for (int i = -halfMajorLines; i <= halfMajorLines; ++i)
            {
                float val = i * majorStep;
                
                // Vertical lines (parallel to Y)
                glm::vec3 startY(val, -extent, 0.0f);
                glm::vec3 endY(val, extent, 0.0f);
                glm::vec3 colorY = (std::abs(val) < 0.001f) ? axisColorY : gridColorMajor;
                addLine(startY, endY, colorY);

                // Horizontal lines (parallel to X)
                glm::vec3 startX(-extent, val, 0.0f);
                glm::vec3 endX(extent, val, 0.0f);
                glm::vec3 colorX = (std::abs(val) < 0.001f) ? axisColorX : gridColorMajor;
                addLine(startX, endX, colorX);
            }

            // 4. Draw vertical YZ Grid at X = 0 (extent = 5000, step = majorStep)
            for (int i = -halfMajorLines; i <= halfMajorLines; ++i)
            {
                float val = i * majorStep;
                
                // Vertical lines (parallel to Y)
                glm::vec3 startY(0.0f, -extent, val);
                glm::vec3 endY(0.0f, extent, val);
                glm::vec3 colorY = (std::abs(val) < 0.001f) ? axisColorY : gridColorMajor;
                addLine(startY, endY, colorY);

                // Horizontal lines (parallel to Z)
                glm::vec3 startZ(0.0f, val, -extent);
                glm::vec3 endZ(0.0f, val, extent);
                glm::vec3 colorZ = (std::abs(val) < 0.001f) ? axisColorZ : gridColorMajor;
                addLine(startZ, endZ, colorZ);
            }
        }
    }

    // Draw Rotation, Scale, and Translation visual indicators for Selected Entity
    if (SceneHierarchyPanel::s_SelectedEntity != entt::null &&
        scene.IsValid(SceneHierarchyPanel::s_SelectedEntity))
    {
        glm::vec3 pos(0.0f);
        if (auto* tr = scene.TryGetComponent<WorldTransformComponent>(SceneHierarchyPanel::s_SelectedEntity))
            pos = glm::vec3(tr->worldMatrix[3]);
        else if (auto* p = scene.TryGetComponent<PositionComponent>(SceneHierarchyPanel::s_SelectedEntity))
            pos = p->value;

        auto* ioHandler = ServiceLocator::Instance().Resolve<IOHandler>();
        if (ioHandler)
        {
            auto& keyboard = ioHandler->GetKeyboard();
            bool alt = keyboard.GetKey(Key::LeftAlt) || keyboard.GetKey(Key::RightAlt);
            bool ctrl = keyboard.GetKey(Key::LeftControl) || keyboard.GetKey(Key::RightControl);
            bool shift = keyboard.GetKey(Key::LeftShift) || keyboard.GetKey(Key::RightShift);

            float indicatorScale = 1.0f;
            if (auto* s = scene.TryGetComponent<ScaleComponent>(SceneHierarchyPanel::s_SelectedEntity))
            {
                indicatorScale = glm::max(0.5f, glm::max(s->value.x, glm::max(s->value.y, s->value.z)));
            }

            // Helper to draw pitch/yaw/roll circles
            auto drawRotationIndicator = [&](const glm::vec3& center, float radius) {
                const int segments = 32;
                // Red circle: Pitch (YZ plane)
                for (int i = 0; i < segments; ++i)
                {
                    float theta1 = (i / (float)segments) * glm::two_pi<float>();
                    float theta2 = ((i + 1) / (float)segments) * glm::two_pi<float>();
                    glm::vec3 p1 = center + glm::vec3(0.0f, radius * std::cos(theta1), radius * std::sin(theta1));
                    glm::vec3 p2 = center + glm::vec3(0.0f, radius * std::cos(theta2), radius * std::sin(theta2));
                    addLine(p1, p2, glm::vec3(0.9f, 0.1f, 0.1f));
                }
                // Green circle: Yaw (XZ plane)
                for (int i = 0; i < segments; ++i)
                {
                    float theta1 = (i / (float)segments) * glm::two_pi<float>();
                    float theta2 = ((i + 1) / (float)segments) * glm::two_pi<float>();
                    glm::vec3 p1 = center + glm::vec3(radius * std::cos(theta1), 0.0f, radius * std::sin(theta1));
                    glm::vec3 p2 = center + glm::vec3(radius * std::cos(theta2), 0.0f, radius * std::sin(theta2));
                    addLine(p1, p2, glm::vec3(0.1f, 0.9f, 0.1f));
                }
                // Blue circle: Roll (XY plane)
                for (int i = 0; i < segments; ++i)
                {
                    float theta1 = (i / (float)segments) * glm::two_pi<float>();
                    float theta2 = ((i + 1) / (float)segments) * glm::two_pi<float>();
                    glm::vec3 p1 = center + glm::vec3(radius * std::cos(theta1), radius * std::sin(theta1), 0.0f);
                    glm::vec3 p2 = center + glm::vec3(radius * std::cos(theta2), radius * std::sin(theta2), 0.0f);
                    addLine(p1, p2, glm::vec3(0.1f, 0.1f, 0.9f));
                }
            };

            // Helper to draw scale handles
            auto drawScaleIndicator = [&](const glm::vec3& center, float length) {
                glm::vec3 xEnd = center + glm::vec3(length, 0.0f, 0.0f);
                glm::vec3 yEnd = center + glm::vec3(0.0f, length, 0.0f);
                glm::vec3 zEnd = center + glm::vec3(0.0f, 0.0f, length);

                addLine(center, xEnd, glm::vec3(0.9f, 0.1f, 0.1f));
                addLine(center, yEnd, glm::vec3(0.1f, 0.9f, 0.1f));
                addLine(center, zEnd, glm::vec3(0.1f, 0.1f, 0.9f));

                float boxSize = 0.06f * length;
                auto addMiniBox = [&](const glm::vec3& pBox, const glm::vec3& color) {
                    glm::vec3 p1 = pBox + glm::vec3(-boxSize, -boxSize, -boxSize);
                    glm::vec3 p2 = pBox + glm::vec3(boxSize, -boxSize, -boxSize);
                    glm::vec3 p3 = pBox + glm::vec3(boxSize, boxSize, -boxSize);
                    glm::vec3 p4 = pBox + glm::vec3(-boxSize, boxSize, -boxSize);
                    glm::vec3 p5 = pBox + glm::vec3(-boxSize, -boxSize, boxSize);
                    glm::vec3 p6 = pBox + glm::vec3(boxSize, -boxSize, boxSize);
                    glm::vec3 p7 = pBox + glm::vec3(boxSize, boxSize, boxSize);
                    glm::vec3 p8 = pBox + glm::vec3(-boxSize, boxSize, boxSize);

                    addLine(p1, p2, color);
                    addLine(p2, p3, color);
                    addLine(p3, p4, color);
                    addLine(p4, p1, color);
                    addLine(p5, p6, color);
                    addLine(p6, p7, color);
                    addLine(p7, p8, color);
                    addLine(p8, p5, color);
                    addLine(p1, p5, color);
                    addLine(p2, p6, color);
                    addLine(p3, p7, color);
                    addLine(p4, p8, color);
                };

                addMiniBox(xEnd, glm::vec3(0.9f, 0.1f, 0.1f));
                addMiniBox(yEnd, glm::vec3(0.1f, 0.9f, 0.1f));
                addMiniBox(zEnd, glm::vec3(0.1f, 0.1f, 0.9f));
            };

            if (alt && ctrl && !shift)
            {
                drawRotationIndicator(pos, indicatorScale * 1.5f);
            }
            else if (alt && shift && !ctrl)
            {
                drawScaleIndicator(pos, indicatorScale * 1.6f);
            }
            else if (alt && !ctrl && !shift)
            {
                float len = indicatorScale * 1.6f;
                glm::vec3 xEnd = pos + glm::vec3(len, 0.0f, 0.0f);
                glm::vec3 yEnd = pos + glm::vec3(0.0f, len, 0.0f);
                glm::vec3 zEnd = pos + glm::vec3(0.0f, 0.0f, len);

                addLine(pos, xEnd, glm::vec3(0.9f, 0.1f, 0.1f));
                addLine(pos, yEnd, glm::vec3(0.1f, 0.9f, 0.1f));
                addLine(pos, zEnd, glm::vec3(0.1f, 0.1f, 0.9f));

                float arr = 0.04f * len;
                addLine(xEnd, xEnd - glm::vec3(arr * 2.0f, -arr, -arr), glm::vec3(0.9f, 0.1f, 0.1f));
                addLine(xEnd, xEnd - glm::vec3(arr * 2.0f, arr, -arr), glm::vec3(0.9f, 0.1f, 0.1f));
                addLine(xEnd, xEnd - glm::vec3(arr * 2.0f, arr, arr), glm::vec3(0.9f, 0.1f, 0.1f));
                addLine(xEnd, xEnd - glm::vec3(arr * 2.0f, -arr, arr), glm::vec3(0.9f, 0.1f, 0.1f));

                addLine(yEnd, yEnd - glm::vec3(-arr, arr * 2.0f, -arr), glm::vec3(0.1f, 0.9f, 0.1f));
                addLine(yEnd, yEnd - glm::vec3(arr, arr * 2.0f, -arr), glm::vec3(0.1f, 0.9f, 0.1f));
                addLine(yEnd, yEnd - glm::vec3(arr, arr * 2.0f, arr), glm::vec3(0.1f, 0.9f, 0.1f));
                addLine(yEnd, yEnd - glm::vec3(-arr, arr * 2.0f, arr), glm::vec3(0.1f, 0.9f, 0.1f));

                addLine(zEnd, zEnd - glm::vec3(-arr, -arr, arr * 2.0f), glm::vec3(0.1f, 0.1f, 0.9f));
                addLine(zEnd, zEnd - glm::vec3(arr, -arr, arr * 2.0f), glm::vec3(0.1f, 0.1f, 0.9f));
                addLine(zEnd, zEnd - glm::vec3(arr, arr, arr * 2.0f), glm::vec3(0.1f, 0.1f, 0.9f));
                addLine(zEnd, zEnd - glm::vec3(-arr, arr, arr * 2.0f), glm::vec3(0.1f, 0.1f, 0.9f));
            }
            else
            {
                float s = indicatorScale * 0.7f;
                glm::vec3 c(0.9f, 0.9f, 0.2f);
                glm::vec3 p1 = pos + glm::vec3(-s, -s, -s);
                glm::vec3 p2 = pos + glm::vec3(s, -s, -s);
                glm::vec3 p3 = pos + glm::vec3(s, s, -s);
                glm::vec3 p4 = pos + glm::vec3(-s, s, -s);
                glm::vec3 p5 = pos + glm::vec3(-s, -s, s);
                glm::vec3 p6 = pos + glm::vec3(s, -s, s);
                glm::vec3 p7 = pos + glm::vec3(s, s, s);
                glm::vec3 p8 = pos + glm::vec3(-s, s, s);

                addLine(p1, p2, c);
                addLine(p2, p3, c);
                addLine(p3, p4, c);
                addLine(p4, p1, c);
                addLine(p5, p6, c);
                addLine(p6, p7, c);
                addLine(p7, p8, c);
                addLine(p8, p5, c);
                addLine(p1, p5, c);
                addLine(p2, p6, c);
                addLine(p3, p7, c);
                addLine(p4, p8, c);
            }
        }
    }

    if (lineVertices.empty())
        return;

    auto* graphics = sl.Resolve<IGraphicsContext>();
    if (!graphics)
        return;

    auto& bm = graphics->GetBufferManager();
    auto& dc = graphics->GetDrawContext();

    if (m_LineVAO == 0)
        m_LineVAO = bm.GenVertexArray();
    if (m_LineVBO == 0)
        m_LineVBO = bm.GenBuffer();

    bm.BindVertexArray(m_LineVAO);
    bm.BindBuffer(BufferType::ArrayBuffer, m_LineVBO);
    bm.BufferData(BufferType::ArrayBuffer, lineVertices.size() * sizeof(float), lineVertices.data(),
                  BufferUsage::StreamDraw);

    bm.EnableVertexAttribArray(0);
    bm.VertexAttribPointer(0, 3, DataType::Float, false, 6 * sizeof(float), (void*)0);
    bm.EnableVertexAttribArray(1);
    bm.VertexAttribPointer(1, 3, DataType::Float, false, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    graphics->GetRenderStateManager().Disable(ServerCapability::DepthTest);
    dc.DrawArrays(Primitive::Lines, 0, (int)(lineVertices.size() / 6));
    graphics->GetRenderStateManager().Enable(ServerCapability::DepthTest);

    bm.BindVertexArray(0);
}

void PhysicsEditorModule::ProcessInput(KeyboardManager& keyboard)
{
    if (!m_Enabled)
        return;

    ProcessKey(keyboard, Key::F8, m_F8Pressed, [this, &keyboard]() {
        bool shift = keyboard.GetKey(Key::LeftShift) || keyboard.GetKey(Key::RightShift);
        if (shift)
        {
            auto cm = ServiceLocator::Instance().Resolve<ConfigManager>();
            if (cm)
            {
                auto conf = cm->GetConfig();
                conf.debug.audioDebug = !conf.debug.audioDebug;
                cm->UpdateConfig(conf, ConfigChangedEvent::Debug);
            }
        }
        else
        {
            TogglePhysicsDebug();
        }
    });

    ProcessKey(keyboard, Key::F9, m_F9Pressed, [this, &keyboard]() {
        bool shift = keyboard.GetKey(Key::LeftShift) || keyboard.GetKey(Key::RightShift);
        if (shift)
        {
            auto cm = ServiceLocator::Instance().Resolve<ConfigManager>();
            if (cm)
            {
                auto conf = cm->GetConfig();
                conf.debug.particleDebug = !conf.debug.particleDebug;
                cm->UpdateConfig(conf, ConfigChangedEvent::Debug);
            }
        }
        else
        {
            auto* sysMgr = ServiceLocator::Instance().Resolve<SystemManager>();
            if (sysMgr)
            {
                auto* uiSys = sysMgr->GetSystem("UIRenderSystem");
                if (uiSys)
                {
                    EventManager::Instance().Publish(SystemEnabledEvent{"UIRenderSystem", !uiSys->IsEnabled()});
                }
            }
        }
    });
}

void PhysicsEditorModule::TogglePhysicsDebug()
{
    auto cm = ServiceLocator::Instance().Resolve<ConfigManager>();
    if (!cm)
        return;
    auto conf = cm->GetConfig();
    conf.debug.physicsDebug = !conf.debug.physicsDebug;
    cm->UpdateConfig(conf, ConfigChangedEvent::Debug);
}

void PhysicsEditorModule::ProcessKey(KeyboardManager& keyboard, Key key, bool& pressedState,
                                     std::function<void()> action)
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
