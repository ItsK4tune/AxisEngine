#include "sample_state.h"
#include <ecs/logic/post_process_system.h>
#include <ecs/unit/post_process_component.h>
#include <platform/logic/input_serializer.h>
#include <core/logic/data_node_serializer.h>
#include <network/network_system.h>
#include <physics/logic/collision_matrix.h>
#include <audio/logic/audio_service.h>
#include <audio/interface/i_sound.h>
#include <ecs/unit/media_components.h>
#include <ecs/unit/reflection_components.h>
#include <scene/logic/scene_serializer.h>
#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <glm/gtx/quaternion.hpp>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dxgi1_4.h>
#include <pdh.h>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "pdh.lib")

#ifdef GetCurrentTime
#undef GetCurrentTime
#endif

namespace
{
struct PerfStats
{
    float cpu = 0.0f;
    float gpu = 0.0f;
    float ram = 0.0f;
    float vram = 0.0f;
};

PerfStats QueryPerfStats()
{
    static PerfStats smoothed;
    static bool hasSmoothed = false;
    static ULONGLONG lastUpdateMs = 0;
    ULONGLONG nowMs = GetTickCount64();
    if (lastUpdateMs != 0 && nowMs - lastUpdateMs < 500)
        return smoothed;
    lastUpdateMs = nowMs;

    PerfStats stats = smoothed;

    static ULARGE_INTEGER prevIdle{}, prevKernel{}, prevUser{};
    FILETIME idleTime{}, kernelTime{}, userTime{};
    if (GetSystemTimes(&idleTime, &kernelTime, &userTime))
    {
        ULARGE_INTEGER idle{}, kernel{}, user{};
        idle.LowPart = idleTime.dwLowDateTime;
        idle.HighPart = idleTime.dwHighDateTime;
        kernel.LowPart = kernelTime.dwLowDateTime;
        kernel.HighPart = kernelTime.dwHighDateTime;
        user.LowPart = userTime.dwLowDateTime;
        user.HighPart = userTime.dwHighDateTime;
        const auto sysDelta = (kernel.QuadPart - prevKernel.QuadPart) + (user.QuadPart - prevUser.QuadPart);
        const auto idleDelta = idle.QuadPart - prevIdle.QuadPart;
        if (prevKernel.QuadPart != 0 && sysDelta > 0)
            stats.cpu = glm::clamp((1.0f - static_cast<float>(idleDelta) / static_cast<float>(sysDelta)) * 100.0f,
                                   0.0f, 100.0f);
        prevIdle = idle;
        prevKernel = kernel;
        prevUser = user;
    }

    MEMORYSTATUSEX mem{};
    mem.dwLength = sizeof(mem);
    if (GlobalMemoryStatusEx(&mem))
        stats.ram = static_cast<float>(mem.dwMemoryLoad);

    static PDH_HQUERY gpuQuery = nullptr;
    static PDH_HCOUNTER gpuCounter = nullptr;
    if (!gpuQuery)
    {
        if (PdhOpenQueryW(nullptr, 0, &gpuQuery) == ERROR_SUCCESS)
        {
            if (PdhAddEnglishCounterW(gpuQuery, L"\\GPU Engine(*)\\Utilization Percentage", 0, &gpuCounter) !=
                ERROR_SUCCESS)
            {
                PdhCloseQuery(gpuQuery);
                gpuQuery = nullptr;
                gpuCounter = nullptr;
            }
            else
            {
                PdhCollectQueryData(gpuQuery);
            }
        }
    }
    else if (PdhCollectQueryData(gpuQuery) == ERROR_SUCCESS)
    {
        DWORD itemCount = 0;
        DWORD bufferSize = 0;
        PdhGetFormattedCounterArrayW(gpuCounter, PDH_FMT_DOUBLE, &bufferSize, &itemCount, nullptr);
        if (bufferSize > 0 && itemCount > 0)
        {
            std::vector<unsigned char> buffer(bufferSize);
            auto* items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(buffer.data());
            if (PdhGetFormattedCounterArrayW(gpuCounter, PDH_FMT_DOUBLE, &bufferSize, &itemCount, items) ==
                ERROR_SUCCESS)
            {
                double total = 0.0;
                for (DWORD i = 0; i < itemCount; ++i)
                {
                    if (items[i].FmtValue.CStatus == ERROR_SUCCESS)
                        total += items[i].FmtValue.doubleValue;
                }
                stats.gpu = glm::clamp(static_cast<float>(total), 0.0f, 100.0f);
            }
        }
    }

    IDXGIFactory1* factory = nullptr;
    if (CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&factory)) == S_OK && factory)
    {
        IDXGIAdapter1* adapter = nullptr;
        for (UINT i = 0; factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i)
        {
            DXGI_ADAPTER_DESC1 desc{};
            adapter->GetDesc1(&desc);
            if ((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0)
                break;
            adapter->Release();
            adapter = nullptr;
        }

        IDXGIAdapter3* adapter3 = nullptr;
        if (adapter && adapter->QueryInterface(__uuidof(IDXGIAdapter3), reinterpret_cast<void**>(&adapter3)) == S_OK)
        {
            DXGI_QUERY_VIDEO_MEMORY_INFO info{};
            if (adapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info) == S_OK && info.Budget > 0)
                stats.vram = glm::clamp(static_cast<float>(static_cast<double>(info.CurrentUsage) /
                                                           static_cast<double>(info.Budget) * 100.0),
                                        0.0f, 100.0f);
            adapter3->Release();
        }
        if (adapter)
            adapter->Release();
        factory->Release();
    }

    if (!hasSmoothed)
    {
        smoothed = stats;
        hasSmoothed = true;
    }
    else
    {
        constexpr float alpha = 0.22f;
        smoothed.cpu = glm::mix(smoothed.cpu, stats.cpu, alpha);
        smoothed.gpu = glm::mix(smoothed.gpu, stats.gpu, alpha);
        smoothed.ram = glm::mix(smoothed.ram, stats.ram, alpha);
        smoothed.vram = glm::mix(smoothed.vram, stats.vram, alpha);
    }

    return smoothed;
}

std::string SamplePath(const char* relativePath)
{
    std::filesystem::path path(relativePath);
    if (std::filesystem::exists(path))
        return path.generic_string();

    std::filesystem::path parentPath = std::filesystem::path("..") / path;
    if (std::filesystem::exists(parentPath))
        return parentPath.generic_string();

    return path.generic_string();
}

constexpr const char* kScenario12ScenePath = "sample/scene/sample.axs";
constexpr const char* kScenario12SceneLegacyPath = "sample/scene/sample.axis";
constexpr const char* kScenario15DataPath = "sample/resource/data/data.axs";
constexpr const char* kScenario15DataLegacyPath = "sample/resource/data/data.axis";

glm::quat RotationFromNegativeY(const glm::vec3& direction)
{
    glm::vec3 dir = glm::normalize(direction);
    if (!std::isfinite(dir.x) || !std::isfinite(dir.y) || !std::isfinite(dir.z) || glm::length2(dir) < 0.0001f)
        return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    return glm::rotation(glm::vec3(0.0f, -1.0f, 0.0f), dir);
}
}

void SampleState::OnEnter()
{
    // Enable all systems needed for our scenes
    EnablePhysics(true);
    EnableRender(true);
    EnableScript(true);
    EnableAudio(true);
    EnableAnimation(true);
    EnableVideo(true);
    EnableNavigation(true);
    EnableParticle(true);
    EnableUIRender(true);
    EnableSkybox(true);
    EnableSystem("ReflectionProbeSystem", true);
    EnableSystem("PlanarReflectionSystem", true);
    EnableSystem("NetworkSystem", true);

    SetCursorMode(CursorMode::Normal);

    // Disable Default Editor panels so we can draw our clean benchmark UI
    EnableSystem("EditorSystem", false);

    // Retrieve EditorSystem's ImGuiLayer
    auto* sysMgr = Resolve<SystemManager>();
    if (sysMgr)
    {
        auto* editorSys = dynamic_cast<EditorSystem*>(sysMgr->GetSystem("EditorSystem"));
        if (editorSys)
        {
            m_EditorImGuiLayer = &editorSys->GetImGuiLayer();
        }
    }

    // Register actions for Scenario 8 input handling
    auto* io = Resolve<IOHandler>();
    if (io)
    {
        auto& input = io->GetInputManager();
        input.BindAction("PlayerForward", InputType::Key, (int)Key::W);
        input.BindAction("PlayerBackward", InputType::Key, (int)Key::S);
        input.BindAction("PlayerLeft", InputType::Key, (int)Key::A);
        input.BindAction("PlayerRight", InputType::Key, (int)Key::D);
        input.BindAction("PlayerAction", InputType::MouseButton, (int)Mouse::Left);
        input.BindAction("PlayerJump", InputType::Key, (int)Key::Space);
    }

    srand(static_cast<unsigned int>(time(nullptr)));

    // Load initial scenario (Scene 1)
    LoadScenario(1);
}

void SampleState::OnUpdate(float dt)
{
    // Process deferred scenario switch (set by DrawGUI in OnRender previous frame)
    if (m_PendingScenario >= 0)
    {
        LoadScenario(m_PendingScenario);
        m_PendingScenario = -1;
        return;  // skip rest of update this frame — systems will run with fresh entities
    }

    // Benchmark counters
    m_FpsTime += dt;
    m_FpsCount++;
    if (m_FpsTime >= 1.0f)
    {
        m_CurrentFps = static_cast<float>(m_FpsCount) / m_FpsTime;
        m_FpsTime = 0.0f;
        m_FpsCount = 0;
    }

    // Pathfinding update (Scenario 5)
    if (m_CurrentScenario == 5 && m_NavFollower != entt::null && !m_NavWaypoints.empty())
    {
        auto& navSystem = GetSystem<NavigationSystem>();
        auto* follower = GetScene().registry.try_get<PathFollowerComponent>(m_NavFollower);
        if (follower)
        {
            follower->moveSpeed = m_S5FollowerSpeed;
            follower->lockXPitch = m_S5LockXPitch;
            follower->lockYYaw = m_S5LockYYaw;
            follower->lockZRoll = m_S5LockZRoll;
            follower->pathfindingOptions.criteria = static_cast<PathfindingCriteria>(m_S5PathfindingCriteria);
            follower->pathfindingOptions.preferredTags = {"road"};
            follower->pathfindingOptions.tagWeightBonus = 30.0f;

            if (!follower->pathPending && !follower->isMoving)
            {
                m_CurrentWaypointIndex = (m_CurrentWaypointIndex + 1) % m_NavWaypoints.size();
                navSystem.MoveTo(GetScene(), m_NavFollower, m_NavWaypoints[m_CurrentWaypointIndex]);
            }
        }
    }

    // Apply continuous wind force (Scenario 10)
    if (m_CurrentScenario == 10 && !m_S10ChainEntities.empty() && m_S10WindForce > 0.0f)
    {
        auto& scene = GetScene();
        auto lastEntity = m_S10ChainEntities.back();
        auto* rb = scene.registry.try_get<RigidBodyComponent>(lastEntity);
        if (rb && rb->body)
        {
            rb->body->Activate(true);
            static float windAccumTime = 0.0f;
            windAccumTime += dt;
            float windForceX = m_S10WindForce * sin(windAccumTime * 3.0f);
            rb->body->ApplyCentralForce(glm::vec3(windForceX, 0.0f, 0.0f));
        }
    }

    // Post-processing parameters update (Scenario 9)
    if (m_CurrentScenario == 9)
    {
        auto* sysMgr = Resolve<SystemManager>();
        if (sysMgr)
        {
            auto* ppSys = dynamic_cast<PostProcessSystem*>(sysMgr->GetSystem("PostProcessSystem"));
            if (ppSys)
            {
                auto& pipeline = ppSys->GetPipeline();
                pipeline.SetBloomEnabled(m_PPBloomEnabled);
                pipeline.SetBloomThreshold(m_PPBloomThreshold);
                pipeline.SetBloomIntensity(m_PPBloomIntensity);
                pipeline.SetBloomRadius(m_PPBloomRadius);
                pipeline.SetHDREnabled(m_PPHdrEnabled);
                if (m_PPHdrEnabled)
                {
                    pipeline.SetExposure(m_PPExposure);
                    pipeline.SetGamma(m_PPGamma);
                    pipeline.SetTonemappingMode(m_PPTonemappingMode);
                }

                auto view = GetScene().registry.view<PostProcessComponent, InfoComponent>();
                for (auto entity : view)
                {
                    auto& info = view.get<InfoComponent>(entity);
                    if (info.name != "Scenario9PostProcess")
                        continue;

                    auto& pp = view.get<PostProcessComponent>(entity);
                    pp.effects.clear();
                    if (m_PPVignetteEnabled)
                        pp.effects.push_back({"vignette", 100, 0, 0, 0, 0, true, false});
                    if (m_PPGlitchEnabled)
                        pp.effects.push_back({"glitch", 100, 0, 0, 0, 0, true, false});
                    if (m_PPFilmGrainEnabled)
                        pp.effects.push_back({"film_grain", 100, 0, 0, 0, 0, true, false});
                    break;
                }
            }
        }
    }

    if (m_CurrentScenario == 2)
    {
        m_S2MotionTime += dt;
        auto& scene = GetScene();

        auto dirView = scene.registry.view<DirectionalLightComponent, InfoComponent>();
        for (auto entity : dirView)
        {
            auto& info = dirView.get<InfoComponent>(entity);
            if (info.name != "DirLight")
                continue;
            auto& light = dirView.get<DirectionalLightComponent>(entity);
            light.color = m_S2DirectionalColor;
            light.intensity = m_S2DirectionalIntensity;
            if (m_S2LightMotionMode != 0)
            {
                float a = m_S2MotionTime * m_S2DirectionalSweepSpeed;
                light.direction = glm::normalize(glm::vec3(cos(a) * 0.65f, -1.0f, sin(a) * 0.65f));
                if (auto* rot = scene.registry.try_get<RotationComponent>(entity))
                    rot->value = RotationFromNegativeY(light.direction);
                if (auto* world = scene.registry.try_get<WorldTransformComponent>(entity))
                    world->isDirty = true;
            }
            if (auto* mat = scene.registry.try_get<AxisMaterialComponent>(entity))
            {
                mat->desc.emission = m_S2DirectionalColor * (m_S2DirectionalIntensity * 3.0f);
                mat->gpu.dirty = true;
            }
            break;
        }

        auto view = GetScene().registry.view<PositionComponent, PointLightComponent, InfoComponent>();
        for (auto entity : view)
        {
            auto& info = view.get<InfoComponent>(entity);
            if (info.name != "PointLight")
                continue;

            auto& pos = view.get<PositionComponent>(entity);
            float a = m_S2MotionTime * m_S2PointMotionSpeed;
            if (m_S2LightMotionMode == 1)
                pos.value = glm::vec3(cos(a) * m_S2PointOrbitRadius, m_S2PointMotionHeight, sin(a) * m_S2PointOrbitRadius);
            else if (m_S2LightMotionMode == 2)
                pos.value.y = m_S2PointMotionHeight + sin(a) * 8.0f;
            else if (m_S2LightMotionMode == 3)
                pos.value = glm::vec3(sin(a) * m_S2PointOrbitRadius, m_S2PointMotionHeight + sin(a * 1.7f) * 6.0f, sin(a * 2.0f) * m_S2PointOrbitRadius * 0.5f);

            if (auto* world = GetScene().registry.try_get<WorldTransformComponent>(entity))
                world->isDirty = true;

            auto& light = view.get<PointLightComponent>(entity);
            light.color = m_S2PointColor;
            light.intensity = m_S2PointIntensity;
            if (auto* mat = GetScene().registry.try_get<AxisMaterialComponent>(entity))
            {
                mat->desc.emission = m_S2PointColor * (m_S2PointIntensity * 2.0f);
                mat->gpu.dirty = true;
            }
            break;
        }

        auto spotView = scene.registry.view<PositionComponent, RotationComponent, SpotLightComponent, InfoComponent>();
        for (auto entity : spotView)
        {
            auto& info = spotView.get<InfoComponent>(entity);
            if (info.name != "SpotLight")
                continue;
            auto& light = spotView.get<SpotLightComponent>(entity);
            light.color = m_S2SpotColor;
            light.intensity = m_S2SpotIntensity;
            light.cutOff = glm::cos(glm::radians(22.5f));
            light.outerCutOff = glm::cos(glm::radians(32.5f));
            light.linear = 0.045f;
            light.quadratic = 0.0075f;

            auto& pos = spotView.get<PositionComponent>(entity);
            auto& rot = spotView.get<RotationComponent>(entity);
            float a = m_S2MotionTime * m_S2SpotMotionSpeed + 1.5708f;
            if (m_S2LightMotionMode == 1)
                pos.value = glm::vec3(cos(a) * m_S2SpotOrbitRadius, m_S2SpotMotionHeight, sin(a) * m_S2SpotOrbitRadius);
            else if (m_S2LightMotionMode == 2)
                pos.value.y = m_S2SpotMotionHeight + sin(a) * 10.0f;
            else if (m_S2LightMotionMode == 3)
                pos.value = glm::vec3(sin(a * 1.3f) * m_S2SpotOrbitRadius, m_S2SpotMotionHeight + sin(a) * 5.0f, sin(a * 2.0f) * m_S2SpotOrbitRadius * 0.55f);

            glm::vec3 target = glm::vec3(0.0f, 1.0f, 0.0f);
            light.direction = glm::normalize(target - pos.value);
            rot.value = RotationFromNegativeY(light.direction);
            if (auto* world = scene.registry.try_get<WorldTransformComponent>(entity))
                world->isDirty = true;
            if (auto* mat = scene.registry.try_get<AxisMaterialComponent>(entity))
            {
                mat->desc.emission = m_S2SpotColor * (m_S2SpotIntensity * 1.25f);
                mat->gpu.dirty = true;
            }
            break;
        }
    }

    if (m_CurrentScenario == 3)
    {
        auto& scene = GetScene();
        auto dirView = scene.registry.view<DirectionalLightComponent>();
        for (auto entity : dirView)
        {
            auto& light = dirView.get<DirectionalLightComponent>(entity);
            light.color = m_S3DirectionalColor;
            light.intensity = m_S3DirectionalIntensity;
        }
        auto pointView = scene.registry.view<PointLightComponent>();
        for (auto entity : pointView)
        {
            auto& light = pointView.get<PointLightComponent>(entity);
            light.color = m_S3PointColor;
            light.intensity = m_S3PointIntensity;
            if (auto* mat = scene.registry.try_get<AxisMaterialComponent>(entity))
            {
                mat->desc.emission = m_S3PointColor * (m_S3PointIntensity * 4.0f);
                mat->gpu.dirty = true;
            }
        }
        auto spotView = scene.registry.view<SpotLightComponent>();
        for (auto entity : spotView)
        {
            auto& light = spotView.get<SpotLightComponent>(entity);
            light.color = m_S3SpotColor;
            light.intensity = m_S3SpotIntensity;
            if (auto* mat = scene.registry.try_get<AxisMaterialComponent>(entity))
            {
                mat->desc.emission = m_S3SpotColor * (m_S3SpotIntensity * 3.0f);
                mat->gpu.dirty = true;
            }
        }

        glm::vec3 averageColor = (m_S3DirectionalColor + m_S3PointColor + m_S3SpotColor) / 3.0f;
        float visibleLight = glm::clamp((m_S3DirectionalIntensity / 0.2f + m_S3PointIntensity / 10.0f +
                                         m_S3SpotIntensity / 12.0f) /
                                            3.0f,
                                        0.0f, 1.0f);
        visibleLight = 0.18f + visibleLight * 0.82f;
        auto renderView = scene.registry.view<MeshRendererComponent, InfoComponent>();
        for (auto entity : renderView)
        {
            auto& info = renderView.get<InfoComponent>(entity);
            if (info.name == "Floor" || info.name == "Cylinder")
            {
                renderView.get<MeshRendererComponent>(entity).color = glm::vec4(averageColor * visibleLight, 1.0f);
            }
        }
    }

    if (m_CurrentScenario == 11)
    {
        auto view = GetScene().registry.view<DirectionalLightComponent, InfoComponent>();
        for (auto entity : view)
        {
            auto& info = view.get<InfoComponent>(entity);
            if (info.name != "DecalDirLight")
                continue;
            auto& light = view.get<DirectionalLightComponent>(entity);
            light.color = m_S11LightColor;
            light.intensity = m_S11LightIntensity;
            break;
        }
    }

    if (m_CurrentScenario == 13)
    {
        auto* io = Resolve<IOHandler>();
        if (io)
        {
            auto& input = io->GetInputManager();
            auto view = GetScene().registry.view<MeshRendererComponent, ScaleComponent, WorldTransformComponent, InfoComponent>();
            for (auto entity : view)
            {
                auto& info = view.get<InfoComponent>(entity);
                if (info.name.rfind("InputPad_", 0) != 0)
                    continue;

                const std::string action = info.name.substr(9);
                const bool pressed = input.GetAction(action);
                auto& renderer = view.get<MeshRendererComponent>(entity);
                auto& scale = view.get<ScaleComponent>(entity);
                renderer.color = pressed ? glm::vec4(0.0f, 0.85f, 0.25f, 1.0f)
                                         : glm::vec4(0.18f, 0.2f, 0.24f, 1.0f);
                scale.value.y = pressed ? 0.35f : 0.15f;
                view.get<WorldTransformComponent>(entity).isDirty = true;
            }
        }
    }

    // Scenario 16 Network Update
    if (m_CurrentScenario == 16)
    {
        auto* sysMgr = Resolve<SystemManager>();
        if (sysMgr)
        {
            auto* netSystem = dynamic_cast<NetworkSystem*>(sysMgr->GetSystem("NetworkSystem"));
            if (netSystem && netSystem->IsRunning())
            {
                netSystem->UpdateEvents(0);
                
                if (!netSystem->IsServer())
                {
                    m_S16SendTimer += dt;
                    if (m_S16SendTimer >= 1.5f)
                    {
                        m_S16SendTimer = 0.0f;
                        static int pingCount = 0;
                        std::string msg = "Ping #" + std::to_string(++pingCount) + " from Client!";
                        if (auto* peer = netSystem->GetServerPeer())
                        {
                            netSystem->SendPacket(peer, msg.c_str(), msg.size() + 1);
                        }
                    }
                }
            }
        }
    }

    // Scenario 17 Audio Orbit Update
    if (m_CurrentScenario == 17)
    {
        m_S17OrbitAngle += m_S17Speed * dt;
        glm::vec3 soundPos(sin(m_S17OrbitAngle) * 15.0f, 3.0f, cos(m_S17OrbitAngle) * 15.0f);

        auto view = GetScene().registry.view<PositionComponent, InfoComponent>();
        for (auto entity : view)
        {
            auto& info = view.get<InfoComponent>(entity);
            if (info.name == "AudioSource3D")
            {
                view.get<PositionComponent>(entity).value = soundPos;
                if (auto* world = GetScene().registry.try_get<WorldTransformComponent>(entity))
                    world->isDirty = true;
                break;
            }
        }

        if (m_S17Audio3D)
        {
            m_S17Audio3D->SetPosition(soundPos);
            m_S17Audio3D->SetVolume(m_S17Volume3D);
            m_S17Audio3D->SetPitch(m_S17Pitch);
            m_S17Audio3D->SetMinDistance(m_S17MinDistance);
            m_S17Audio3D->SetMaxDistance(m_S17MaxDistance);
        }
        if (m_S17Audio2D)
        {
            m_S17Audio2D->SetVolume(m_S17Volume2D);
            m_S17Audio2D->SetPitch(m_S17Pitch);
        }
    }

    if (m_CurrentScenario == 21 && m_S21AnimateObjects)
    {
        m_S17OrbitAngle += dt * 0.8f;
        auto view = GetScene().registry.view<PositionComponent, WorldTransformComponent, InfoComponent>();
        for (auto entity : view)
        {
            auto& info = view.get<InfoComponent>(entity);
            if (info.name == "OpaqueMover")
            {
                auto& pos = view.get<PositionComponent>(entity);
                pos.value.x = sin(m_S17OrbitAngle) * 7.5f;
                view.get<WorldTransformComponent>(entity).isDirty = true;
            }
        }
    }
}

void SampleState::OnRender()
{
    OnRenderDebug();
    if (m_EditorImGuiLayer && !m_EditorSystemEnabled)
    {
        m_EditorImGuiLayer->BeginFrame();
        DrawGUI();
        m_EditorImGuiLayer->EndFrame();
    }
}

void SampleState::OnRenderDebug()
{
    if (m_ShowDebugLines)
    {
        // Draw obstacles in RED (Scenario 5)
        if (m_CurrentScenario == 5)
        {
            auto physics_ptr = Resolve<IPhysicsWorld>();
            if (physics_ptr)
            {
                auto& scene = GetScene();
                auto view = scene.registry.view<PositionComponent, ScaleComponent, InfoComponent>();
                for (auto entity : view)
                {
                    auto& info = view.get<InfoComponent>(entity);
                    if (info.tag == "obstacle")
                    {
                        auto& pos = view.get<PositionComponent>(entity);
                        auto& scale = view.get<ScaleComponent>(entity);
                        glm::vec3 h = scale.value * 0.5f;
                        glm::vec3 p = pos.value;
                        glm::vec3 v[8] = {
                            p + glm::vec3(-h.x, -h.y, -h.z),
                            p + glm::vec3( h.x, -h.y, -h.z),
                            p + glm::vec3( h.x,  h.y, -h.z),
                            p + glm::vec3(-h.x,  h.y, -h.z),
                            p + glm::vec3(-h.x, -h.y,  h.z),
                            p + glm::vec3( h.x, -h.y,  h.z),
                            p + glm::vec3( h.x,  h.y,  h.z),
                            p + glm::vec3(-h.x,  h.y,  h.z)
                        };
                        glm::vec3 red(1.0f, 0.1f, 0.1f);
                        physics_ptr->DrawLine(v[0], v[1], red);
                        physics_ptr->DrawLine(v[1], v[2], red);
                        physics_ptr->DrawLine(v[2], v[3], red);
                        physics_ptr->DrawLine(v[3], v[0], red);
                        physics_ptr->DrawLine(v[4], v[5], red);
                        physics_ptr->DrawLine(v[5], v[6], red);
                        physics_ptr->DrawLine(v[6], v[7], red);
                        physics_ptr->DrawLine(v[7], v[4], red);
                        physics_ptr->DrawLine(v[0], v[4], red);
                        physics_ptr->DrawLine(v[1], v[5], red);
                        physics_ptr->DrawLine(v[2], v[6], red);
                        physics_ptr->DrawLine(v[3], v[7], red);
                    }
                }

                glm::vec3 yellow(1.0f, 0.9f, 0.1f);
                for (size_t i = 1; i < m_NavWaypoints.size(); ++i)
                {
                    physics_ptr->DrawLine(m_NavWaypoints[i - 1] + glm::vec3(0.0f, 0.15f, 0.0f),
                                          m_NavWaypoints[i] + glm::vec3(0.0f, 0.15f, 0.0f), yellow);
                }
            }
        }

        // Render physics debug lines (skip for scenario 7 - particle stress test)
        if (m_CurrentScenario != 7)
        try
        {
            auto* sysMgr = Resolve<SystemManager>();
            if (sysMgr)
            {
                auto* physicsSystem = dynamic_cast<PhysicsSystem*>(sysMgr->GetSystem("PhysicsSystem"));
                auto* resources = Resolve<ResourceManager>();
                auto* graphics = Resolve<IGraphicsContext>();
                auto* io = Resolve<IOHandler>();

                if (physicsSystem && resources && graphics && io)
                {
                    auto* window = io->GetMonitorManager().GetWindow();
                    auto shader = resources->GetShader("debug_line");
                    if (window && shader)
                    {
                        auto& rsm = graphics->GetRenderStateManager();
                        int w = window->GetWidth();
                        int h = window->GetHeight();
                        physicsSystem->RenderDebug(GetScene(), *shader, w, h, rsm);
                    }
                }
            }
        }
        catch (...)
        {
        }
    }
}

void SampleState::OnExit()
{
    // Clean up scene entities
    auto* sceneMgr = Resolve<SceneManager>();
    if (sceneMgr)
    {
        sceneMgr->UnloadSceneByName("scenario");
    }

    // No need to shutdown the shared ImGuiLayer
}

void SampleState::LoadScenario(int index)
{
    auto* sceneMgr = Resolve<SceneManager>();
    if (sceneMgr)
    {
        sceneMgr->UnloadSceneByName("scenario");
    }

    m_CurrentScenario = index;
    m_NavFollower = entt::null;
    m_S2MotionTime = 0.0f;
    m_S2DirLightEntity  = entt::null;
    m_S2PointLightEntity = entt::null;
    m_S2SpotLightEntity  = entt::null;
    m_S11DirLightEntity  = entt::null;
    if (index != 4 && index != 10)
    {
        if (auto* physics = Resolve<IPhysicsWorld>())
            physics->SetGravity(glm::vec3(0.0f, -9.81f, 0.0f));
    }

    // Reset post processing pipeline defaults
    auto* sysMgr = Resolve<SystemManager>();
    if (sysMgr)
    {
        auto* ppSys = dynamic_cast<PostProcessSystem*>(sysMgr->GetSystem("PostProcessSystem"));
        if (ppSys)
        {
            auto& pipeline = ppSys->GetPipeline();
            pipeline.SetBloomEnabled(true);
            pipeline.SetBloomThreshold(1.0f);
            pipeline.SetBloomIntensity(1.0f);
            pipeline.SetBloomRadius(0.005f);
            pipeline.SetExposure(1.0f);
            pipeline.SetGamma(2.2f);
            pipeline.SetTonemappingMode(1);
            pipeline.SetHDREnabled(false);
            pipeline.SetBloomEnabled(false);
            pipeline.ClearEffects();
        }
    }
    m_PPVignetteEnabled = false;
    m_PPGlitchEnabled = false;
    m_PPFilmGrainEnabled = false;
    m_S10ChainEntities.clear();
    if (auto* physics = Resolve<IPhysicsWorld>())
        physics->SetSolverIterations(10);
    if (auto* collisionMatrix = Resolve<CollisionMatrix>())
        collisionMatrix->Reset();

    // Reset camera for all scenarios
    SetupCamera();

    switch (index)
    {
        case 1: LoadScene1(); break;
        case 2: LoadScene2(); break;
        case 3: LoadScene3(); break;
        case 4: LoadScene4(); break;
        case 5: LoadScene5(); break;
        case 6: LoadScene6(); break;
        case 7: LoadScene7(); break;
        case 8: LoadScene8(); break;
        case 9: LoadScene9(); break;
        case 10: LoadScene10(); break;
        case 11: LoadScene11(); break;
        case 12: LoadScene12(); break;
        case 13: LoadScene13(); break;
        case 14: LoadScene14(); break;
        case 15: LoadScene15(); break;
        case 16: LoadScene16(); break;
        case 17: LoadScene17(); break;
        case 18: LoadScene18(); break;
        case 19: LoadScene19(); break;
        case 20: LoadScene20(); break;
        case 21: LoadScene21(); break;
        case 22: LoadScene22(); break;
        case 23: LoadScene23(); break;
        case 24: LoadScene24(); break;
        case 25: LoadScene25(); break;
        case 26: LoadScene26(); break;
        case 27: LoadScene27(); break;
        default: break;
    }

    // Force immediate transform update so worldMatrix is correct before next render
    auto& transformSys = GetSystem<TransformSystem>();
    transformSys.m_IsLinearTransformsDirty = true;
    transformSys.Update(GetScene(), 0.0f);
}

void SampleState::SetupCamera()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    auto camera = EntityBuilder(scene, res, "scenario")
        .WithName("MainCamera")
        .WithTransform(glm::vec3(0.0f, 15.0f, 60.0f), glm::vec3(-15.0f, 0.0f, 0.0f), glm::vec3(1.0f))
        .WithCamera(60.0f, 0.1f, 1000.0f, true)
        .Build();

    // Attach DefaultCameraController
    auto& script = scene.registry.emplace<ScriptComponent>(camera);
    script.className = "DefaultCameraController";
    script.InstantiateScript = []() {
        return std::make_unique<DefaultCameraController>();
    };
    script.DestroyScript = [](ScriptComponent* nsc) { nsc->instance.reset(); };
}

void SampleState::DrawGUI()
{
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(380, 780), ImGuiCond_Always);

    ImGui::Begin("AxisEngine Benchmark Dashboard", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    ImGui::TextColored(ImVec4(0.0f, 0.8f, 1.0f, 1.0f), "Performance Metrics");
    ImGui::Separator();
    PerfStats perf = QueryPerfStats();
    ImGui::Text("FPS: %.1f", m_CurrentFps);
    ImGui::Text("Frame Time: %.2f ms", 1000.0f / (m_CurrentFps > 0.0f ? m_CurrentFps : 60.0f));
    ImGui::Text("Active Entities: %d", (int)GetScene().registry.view<InfoComponent>().size());
    ImGui::Text("CPU: %.0f%%  GPU: %.0f%%", perf.cpu, perf.gpu);
    ImGui::Text("RAM: %.0f%%  VRAM: %.0f%%", perf.ram, perf.vram);
    
    // Physics rigid bodies count
    int rbCount = 0;
    auto rbView = GetScene().registry.view<RigidBodyComponent>();
    for (auto entity : rbView)
    {
        if (rbView.get<RigidBodyComponent>(entity).body) rbCount++;
    }
    ImGui::Text("Active Rigid Bodies: %d", rbCount);

    // Active particle count
    int pCount = 0;
    auto pView = GetScene().registry.view<ParticleEmitterComponent>();
    for (auto entity : pView)
    {
        pCount += pView.get<ParticleEmitterComponent>(entity).emitter.GetActiveParticleCount();
    }
    ImGui::Text("Active Particles: %d", pCount);

    ImGui::Spacing();
    if (ImGui::Checkbox("Enable Editor System Layout", &m_EditorSystemEnabled))
    {
        EnableSystem("EditorSystem", m_EditorSystemEnabled);
    }

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.0f, 0.8f, 1.0f, 1.0f), "Select Scenario");
    ImGui::Separator();
    ImGui::BeginChild("ScenarioListChild", ImVec2(0, 240), true);

    auto AddScenarioButton = [this](int index, const char* label, const char* desc)
    {
        bool active = (m_CurrentScenario == index);
        if (active)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.5f, 0.8f, 1.0f));
        }
        if (ImGui::Button(label, ImVec2(330, 24)))
        {
            m_PendingScenario = index;
        }
        if (active)
        {
            ImGui::PopStyleColor();
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("%s", desc);
        }
    };

    AddScenarioButton(1, "Scenario 1: Render Load Test", "10,000 sphere entities (deferred rendering check)");
    AddScenarioButton(2, "Scenario 2: Shadow Mapping Test", "1,000 cubes, plane, and 3 light types casting shadows");
    AddScenarioButton(3, "Scenario 3: Lighting Load Test", "1 cylinder, plane, and 999 dynamic lighting entities");
    AddScenarioButton(4, "Scenario 4: Physics stress Test", "1,000 dynamic Bullet rigid bodies falling and colliding");
    AddScenarioButton(5, "Scenario 5: Navigation Test", "Dynamic NavMesh generation and pathfinding movement");
    AddScenarioButton(6, "Scenario 6: Scriptable Stability Test", "100 entities with 6 complex C++ script behaviors");
    AddScenarioButton(7, "Scenario 7: Particle Stress Test", "50 emitters with high-density colorful particle vortex");
    AddScenarioButton(8, "Scenario 8: Interactive Playground", "Controllable player cube with W/A/S/D, Space and Mouse clicks");
    AddScenarioButton(9, "Scenario 9: Post-Process & Tonemap", "Test HDR Bloom parameters, Exposure, Gamma, and Tonemapping");
    AddScenarioButton(10, "Scenario 10: Physics Constraint Chain", "Pendulum test: cubes linked by Bullet point-to-point joints");
    AddScenarioButton(11, "Scenario 11: Decal Stress Test", "PBR Decals: project materials dynamically on surfaces");
    AddScenarioButton(12, "Scenario 12: Scene Save & Load", "Test scene saving/loading using sample.axs");
    AddScenarioButton(13, "Scenario 13: Input Binding Save/Load", "Test load/save key bindings from binding.axs");
    AddScenarioButton(14, "Scenario 14: Localization (l10n)", "Test load/apply localizations from vi.axs/en.axs");
    AddScenarioButton(15, "Scenario 15: DataNote YAML Test", "Test serialization of entity count and size using data.axs");
    AddScenarioButton(16, "Scenario 16: Network Messaging", "Test local network client/server packet transmission");
    AddScenarioButton(17, "Scenario 17: Audio 2D & 3D Test", "Test irrKlang audio play/orbit using sample.mp3");
    AddScenarioButton(18, "Scenario 18: Video Mesh & UI Render", "Test Video Decoder applying frames to texture and UI using sample.mp4");
    AddScenarioButton(19, "Scenario 19: Skeletal Anim & Blend", "Test animations blend/crossfade using defeated.fbx / spin.fbx");
    AddScenarioButton(20, "Scenario 20: SSR & Env Probes", "Test environment probes and planar reflections reflection mapping");
    AddScenarioButton(21, "Scenario 21: Transparent Objects", "Test transparent forward pass sorting, opacity, and lighting");
    AddScenarioButton(22, "Scenario 22: PBR Material Matrix", "Compare metallic/roughness material response under fixed lights");
    AddScenarioButton(23, "Scenario 23: LOD Selection", "Show distance based model swaps using LODComponent");
    AddScenarioButton(24, "Scenario 24: Layer Filter", "Toggle camera culling mask bits to show/hide render layers");
    AddScenarioButton(25, "Scenario 25: Render Order", "Show transparent object ordering with renderer order values");
    AddScenarioButton(26, "Scenario 26: Instanced Batching", "Compare many matching renderers against unique tint batching breaks");
    AddScenarioButton(27, "Scenario 27: Deferred Shadow Receiver", "Show deferred_lit_shadow writing the receive-shadow bit");

    ImGui::EndChild();

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.0f, 0.8f, 1.0f, 1.0f), "Scenario Parameters / Options");
    ImGui::Separator();

    if (m_CurrentScenario == 1)
    {
        ImGui::SliderInt("Entities count", &m_S1EntityCount, 1000, 50000);
        ImGui::Combo("Mesh Type", &m_S1MeshType, "Sphere\0Cube\0Cylinder\0Capsule\0");
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Click 'Reload Scenario' to apply changes.");
    }
    else if (m_CurrentScenario == 2)
    {
        ImGui::Combo("Light Motion", &m_S2LightMotionMode, "Static\0Circle\0Vertical Bob\0Figure Eight\0");
        ImGui::SliderFloat("Directional Sweep Speed", &m_S2DirectionalSweepSpeed, 0.0f, 2.0f);
        ImGui::SliderFloat("Point Motion Speed", &m_S2PointMotionSpeed, 0.1f, 5.0f);
        ImGui::SliderFloat("Point Orbit Radius", &m_S2PointOrbitRadius, 4.0f, 35.0f);
        ImGui::SliderFloat("Point Base Height", &m_S2PointMotionHeight, 3.0f, 30.0f);
        ImGui::SliderFloat("Spot Motion Speed", &m_S2SpotMotionSpeed, 0.1f, 5.0f);
        ImGui::SliderFloat("Spot Orbit Radius", &m_S2SpotOrbitRadius, 4.0f, 40.0f);
        ImGui::SliderFloat("Spot Base Height", &m_S2SpotMotionHeight, 5.0f, 40.0f);
        ImGui::Separator();
        ImGui::ColorEdit3("Directional Color", &m_S2DirectionalColor.x);
        ImGui::SliderFloat("Directional Brightness", &m_S2DirectionalIntensity, 0.0f, 5.0f);
        ImGui::ColorEdit3("Point Color", &m_S2PointColor.x);
        ImGui::SliderFloat("Point Brightness", &m_S2PointIntensity, 0.0f, 20.0f);
        ImGui::ColorEdit3("Spot Color", &m_S2SpotColor.x);
        ImGui::SliderFloat("Spot Brightness", &m_S2SpotIntensity, 0.0f, 25.0f);
    }
    else if (m_CurrentScenario == 3)
    {
        ImGui::ColorEdit3("Directional Color", &m_S3DirectionalColor.x);
        ImGui::SliderFloat("Directional Intensity", &m_S3DirectionalIntensity, 0.0f, 0.2f);
        ImGui::ColorEdit3("Point Color", &m_S3PointColor.x);
        ImGui::SliderFloat("Point Intensity", &m_S3PointIntensity, 0.0f, 10.0f);
        ImGui::ColorEdit3("Spot Color", &m_S3SpotColor.x);
        ImGui::SliderFloat("Spot Intensity", &m_S3SpotIntensity, 0.0f, 12.0f);
    }
    else if (m_CurrentScenario == 4)
    {
        ImGui::SliderInt("Rigid Bodies count", &m_S4EntityCount, 100, 3000);
        ImGui::Combo("Shape Type", &m_S4ShapeType, "Box\0Sphere\0Capsule\0");
        ImGui::SliderFloat("Mass", &m_S4Mass, 0.1f, 10.0f);
        ImGui::SliderFloat("Bounciness", &m_S4Restitution, 0.0f, 1.0f);
        ImGui::SliderFloat("Friction", &m_S4Friction, 0.0f, 1.0f);
        ImGui::DragFloat3("Gravity", &m_S4Gravity.x, 0.1f, -50.0f, 50.0f);
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Click 'Reload Scenario' to apply changes.");
        ImGui::Spacing();
        if (ImGui::Checkbox("Show Physics Debug Lines", &m_ShowDebugLines))
        {
            auto& navSystem = GetSystem<NavigationSystem>();
            navSystem.SetShowDebug(m_ShowDebugLines);
        }
    }
    else if (m_CurrentScenario == 5)
    {
        ImGui::SliderInt("Obstacles count", &m_S5ObstacleCount, 0, 50);
        ImGui::SliderFloat("Obstacle Size", &m_S5ObstacleSize, 1.0f, 10.0f);
        ImGui::SliderFloat("Follower Speed", &m_S5FollowerSpeed, 1.0f, 25.0f);
        ImGui::Combo("Pathfinding Method", &m_S5PathfindingCriteria, "Shortest\0Smoothest\0StayOnRoad\0");
        ImGui::Spacing();
        ImGui::Text("Axis / Rotation Locks:");
        ImGui::Checkbox("Lock X (Pitch)", &m_S5LockXPitch);
        ImGui::Checkbox("Lock Y (Yaw)", &m_S5LockYYaw);
        ImGui::Checkbox("Lock Z (Roll)", &m_S5LockZRoll);
        ImGui::Spacing();
        ImGui::DragFloat3("New Waypoint", &m_S5NewWaypoint.x, 0.5f, -24.0f, 24.0f);
        if (ImGui::Button("Add Waypoint", ImVec2(170, 24)))
        {
            m_NavWaypoints.push_back(glm::vec3(m_S5NewWaypoint.x, 0.5f, m_S5NewWaypoint.z));
            if (m_NavFollower != entt::null)
            {
                m_CurrentWaypointIndex = static_cast<int>(m_NavWaypoints.size()) - 1;
                auto& navSystem = GetSystem<NavigationSystem>();
                navSystem.StopMoving(GetScene(), m_NavFollower);
                navSystem.MoveTo(GetScene(), m_NavFollower, m_NavWaypoints[m_CurrentWaypointIndex]);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear Custom Path", ImVec2(170, 24)))
        {
            m_NavWaypoints = {
                glm::vec3(-20.0f, 0.5f, 20.0f),
                glm::vec3(20.0f, 0.5f, -20.0f)
            };
            m_CurrentWaypointIndex = 1;
            if (m_NavFollower != entt::null)
            {
                auto& navSystem = GetSystem<NavigationSystem>();
                navSystem.StopMoving(GetScene(), m_NavFollower);
                navSystem.MoveTo(GetScene(), m_NavFollower, m_NavWaypoints[m_CurrentWaypointIndex]);
            }
        }
        ImGui::Text("Waypoints: %d", (int)m_NavWaypoints.size());
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Click 'Reload Scenario' to apply changes.");
        ImGui::Spacing();
        if (ImGui::Checkbox("Show Debug Lines (Path/NavMesh)", &m_ShowDebugLines))
        {
            auto& navSystem = GetSystem<NavigationSystem>();
            navSystem.SetShowDebug(m_ShowDebugLines);
        }
    }
    else if (m_CurrentScenario == 7)
    {
        ImGui::SliderInt("Emitters count", &m_S7EmitterCount, 1, 200);
        ImGui::SliderFloat("Spawn Rate", &m_S7SpawnRate, 10.0f, 1000.0f);
        ImGui::SliderFloat("Life Time (s)", &m_S7LifeTime, 0.5f, 5.0f);
        ImGui::SliderFloat("Start Size", &m_S7StartSize, 0.05f, 2.0f);
        ImGui::SliderFloat("End Size", &m_S7EndSize, 0.0f, 2.0f);
        ImGui::SliderFloat("Min Speed", &m_S7MinSpeed, 0.0f, 20.0f);
        ImGui::SliderFloat("Max Speed", &m_S7MaxSpeed, 0.0f, 30.0f);
        ImGui::SliderFloat("Vertical Speed", &m_S7VerticalSpeed, -10.0f, 20.0f);
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Click 'Reload Scenario' to apply changes.");
    }
    else if (m_CurrentScenario == 8)
    {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "Controls:");
        ImGui::BulletText("W, A, S, D to Move player cube");
        ImGui::BulletText("Spacebar to Jump (Hold for scale pulse)");
        ImGui::BulletText("Left Mouse Click to Randomize color");
    }
    else if (m_CurrentScenario == 9)
    {
        ImGui::Checkbox("Enable Bloom", &m_PPBloomEnabled);
        ImGui::SliderFloat("Bloom Threshold", &m_PPBloomThreshold, 0.0f, 3.0f);
        ImGui::SliderFloat("Bloom Intensity", &m_PPBloomIntensity, 0.0f, 10.0f);
        ImGui::SliderFloat("Bloom Radius", &m_PPBloomRadius, 0.001f, 0.05f);
        ImGui::Checkbox("Enable HDR", &m_PPHdrEnabled);
        if (m_PPHdrEnabled)
        {
            ImGui::SliderFloat("Exposure", &m_PPExposure, 0.1f, 5.0f);
            ImGui::SliderFloat("Gamma", &m_PPGamma, 1.0f, 3.0f);
            ImGui::Combo("Tonemap Mode", &m_PPTonemappingMode, "None\0Reinhard\0ACESFilmic\0Uncharted2\0");
        }
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.0f, 0.8f, 1.0f, 1.0f), "Extra Post-Process Shaders:");
        ImGui::Checkbox("Vignette Effect", &m_PPVignetteEnabled);
        ImGui::Checkbox("Glitch Effect", &m_PPGlitchEnabled);
        ImGui::Checkbox("Film Grain Effect", &m_PPFilmGrainEnabled);
    }
    else if (m_CurrentScenario == 10)
    {
        ImGui::SliderInt("Chain Length", &m_S10ChainLength, 2, 20);
        ImGui::SliderFloat("Wind Force (Oscillating)", &m_S10WindForce, 0.0f, 200.0f);
        ImGui::DragFloat3("Gravity", &m_S10Gravity.x, 0.1f, -50.0f, 50.0f);
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Click 'Reload Scenario' to apply changes.");
        ImGui::Spacing();
        if (ImGui::Checkbox("Show Constraints Debug", &m_ShowDebugLines))
        {
            auto& navSystem = GetSystem<NavigationSystem>();
            navSystem.SetShowDebug(m_ShowDebugLines);
        }
    }
    else if (m_CurrentScenario == 11)
    {
        ImGui::SliderInt("Decal Count", &m_S11DecalCount, 1, 100);
        ImGui::SliderFloat("Decal Size", &m_S11DecalSize, 0.5f, 10.0f);
        ImGui::SliderFloat("Decal Opacity", &m_S11Opacity, 0.0f, 1.0f);
        ImGui::ColorEdit3("Decal Color", &m_S11Color.x);
        ImGui::ColorEdit3("Light Color", &m_S11LightColor.x);
        ImGui::SliderFloat("Light Intensity", &m_S11LightIntensity, 0.0f, 8.0f);
        ImGui::Checkbox("Rainbow Color Mode", &m_S11RainbowMode);
        auto decalView = GetScene().registry.view<DecalComponent>();
        int decalIndex = 0;
        for (auto entity : decalView)
        {
            auto& decal = decalView.get<DecalComponent>(entity);
            decal.opacity = m_S11Opacity;
            if (m_S11RainbowMode)
            {
                int decalDenom = m_S11DecalCount > 1 ? m_S11DecalCount : 1;
                float hue = static_cast<float>(decalIndex) / static_cast<float>(decalDenom);
                decal.tintColor = glm::vec4(
                    0.5f + 0.5f * sin(hue * 6.28318f),
                    0.5f + 0.5f * sin(hue * 6.28318f + 2.09439f),
                    0.5f + 0.5f * sin(hue * 6.28318f + 4.18879f),
                    1.0f);
            }
            else
            {
                decal.tintColor = glm::vec4(m_S11Color, 1.0f);
            }
            ++decalIndex;
        }
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Count/size require Reload Scenario.");
    }
    else if (m_CurrentScenario == 12)
    {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "Scene Save/Load (sample.axs)");
        ImGui::Text("Status: %s", m_S12Status.c_str());
        ImGui::Spacing();
        if (ImGui::Button("Add Random Entity", ImVec2(360, 24)))
        {
            auto& scene = GetScene();
            auto& res = Get<ResourceManager>();
            float x = static_cast<float>(rand() % 400 - 200) / 10.0f;
            float z = static_cast<float>(rand() % 400 - 200) / 10.0f;
            float size = 0.8f + static_cast<float>(rand() % 30) / 10.0f;
            auto entity = EntityBuilder(scene, res, "scenario")
                .WithName("S12Random_" + std::to_string(++m_S12RandomEntityCount))
                .WithTransform(glm::vec3(x, size * 0.5f, z), glm::vec3(0.0f, rand() % 360, 0.0f), glm::vec3(size))
                .WithMesh((m_S12RandomEntityCount % 2) ? "cubeModel" : "sphereModel", "deferred_lit")
                .WithPBRMaterial(0.1f, 0.5f, 1.0f)
                .Build();
            if (auto* renderer = scene.registry.try_get<MeshRendererComponent>(entity))
            {
                renderer->color = glm::vec4(static_cast<float>(rand() % 100) / 100.0f,
                                            static_cast<float>(rand() % 100) / 100.0f,
                                            static_cast<float>(rand() % 100) / 100.0f,
                                            1.0f);
            }
            auto& transformSys = GetSystem<TransformSystem>();
            transformSys.m_IsLinearTransformsDirty = true;
            transformSys.Update(scene, 0.0f);
            m_S12Status = "Added random entity. Save, reload scenario, then Load Scene to verify.";
        }
        if (ImGui::Button("Load Saved Scene (Deserialize)", ImVec2(360, 24)))
        {
            auto* sceneMgr = Resolve<SceneManager>();
            auto* audio = Resolve<AudioService>();
            auto* sysMgr = Resolve<SystemManager>();
            IPhysicsWorld* phys = nullptr;
            if (sysMgr)
            {
                auto* physicsSystem = dynamic_cast<PhysicsSystem*>(sysMgr->GetSystem("PhysicsSystem"));
                if (physicsSystem) phys = &physicsSystem->GetPhysicsWorld();
            }
            if (sceneMgr)
            {
                sceneMgr->UnloadSceneByName("scenario");
                SetupCamera();
                const char* path = std::filesystem::exists(kScenario12ScenePath)
                    ? kScenario12ScenePath
                    : kScenario12SceneLegacyPath;
                SceneLoadResult res = SceneSerializer::Deserialize(path, GetScene(), Get<ResourceManager>(), phys, audio);
                if (!res.entities.empty())
                {
                    for (auto entity : res.entities)
                    {
                        if (auto* info = GetScene().registry.try_get<InfoComponent>(entity))
                        {
                            info->sceneName = "scenario";
                        }
                        sceneMgr->AddEntity(entity, "scenario");
                    }
                    m_S12Status = "Scene loaded successfully. Spawns: " + std::to_string(res.entities.size()) + " entities.";
                    auto& transformSys = GetSystem<TransformSystem>();
                    transformSys.m_IsLinearTransformsDirty = true;
                    transformSys.Update(GetScene(), 0.0f);

                    bool hasLight = false;
                    auto lightView = GetScene().registry.view<DirectionalLightComponent>();
                    for (auto entity : lightView) { (void)entity; hasLight = true; break; }
                    if (!hasLight)
                    {
                        auto& scene = GetScene();
                        auto& resMgr = Get<ResourceManager>();
                        auto light = EntityBuilder(scene, resMgr, "scenario")
                            .WithName("LoadedSceneFallbackLight")
                            .WithTransform(glm::vec3(20.0f, 40.0f, 20.0f), glm::vec3(-45.0f, -45.0f, 0.0f))
                            .WithDirectionalLight(glm::normalize(glm::vec3(-0.7f, -1.0f, -0.7f)), glm::vec3(1.0f), 1.5f)
                            .Build();
                        sceneMgr->AddEntity(light, "scenario");
                    }
                }
                else
                {
                    m_S12Status = "Failed to deserialize scene from sample.axs.";
                }
            }
        }
        if (ImGui::Button("Save Current Scene (Serialize)", ImVec2(360, 24)))
        {
            bool ok = SceneSerializer::Serialize(kScenario12ScenePath, GetScene(), Get<ResourceManager>(), "scenario");
            if (ok)
            {
                m_S12Status = "Serialized current scene to sample.axs!";
            }
            else
            {
                m_S12Status = "Failed to serialize current scene.";
            }
        }
        if (ImGui::Button("Flush Scene (Clear All Entities)", ImVec2(360, 24)))
        {
            auto* sceneMgr = Resolve<SceneManager>();
            if (sceneMgr) sceneMgr->UnloadSceneByName("scenario");
            SetupCamera();
            m_S12RandomEntityCount = 0;
            m_S12Status = "Scene flushed. Add entities or Load to restore.";
        }
    }
    else if (m_CurrentScenario == 13)
    {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "Input Binding Save/Load");
        ImGui::Text("Status: %s", m_S13Status.c_str());
        ImGui::Spacing();

        // --- Step 1: Persist / Load ---
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "Step 1 - Persist bindings:");
        if (ImGui::Button("Load binding.axs", ImVec2(175, 24)))
        {
            auto* io = Resolve<IOHandler>();
            if (io)
            {
                InputSerializer serializer;
                m_S13Status = serializer.Deserialize("sample/resource/binding/binding.axs", io->GetInputManager()) ?
                                  "Loaded binding.axs." : "Failed to load binding.axs.";
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Save binding.axs", ImVec2(175, 24)))
        {
            auto* io = Resolve<IOHandler>();
            if (io)
            {
                InputSerializer serializer;
                m_S13Status = serializer.Serialize("sample/resource/binding/binding.axs", io->GetInputManager()) ?
                                  "Saved bindings to binding.axs." : "Failed to save.";
            }
        }
        if (ImGui::Button("Flush Bindings (Reset to Default)", ImVec2(360, 24)))
        {
            auto* io = Resolve<IOHandler>();
            if (io)
            {
                auto& input = io->GetInputManager();
                input.FlushBindings();
                input.BindAction("PlayerForward",  InputType::Key, (int)Key::W);
                input.BindAction("PlayerBackward", InputType::Key, (int)Key::S);
                input.BindAction("PlayerLeft",     InputType::Key, (int)Key::A);
                input.BindAction("PlayerRight",    InputType::Key, (int)Key::D);
                input.BindAction("PlayerAction",   InputType::MouseButton, (int)Mouse::Left);
                input.BindAction("PlayerJump",     InputType::Key, (int)Key::Space);
                m_S13Status = "Bindings reset to WASD defaults.";
            }
        }

        ImGui::Separator();
        // --- Step 2: Preset ---
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "Step 2 - Try a preset:");
        if (ImGui::Button("Apply Arrow-Key Preset", ImVec2(360, 24)))
        {
            auto* io = Resolve<IOHandler>();
            if (io)
            {
                auto& input = io->GetInputManager();
                input.BindAction("PlayerForward",  InputType::Key, (int)Key::Up);
                input.BindAction("PlayerBackward", InputType::Key, (int)Key::Down);
                input.BindAction("PlayerLeft",     InputType::Key, (int)Key::Left);
                input.BindAction("PlayerRight",    InputType::Key, (int)Key::Right);
                m_S13Status = "Arrow-key preset applied. Save to persist.";
            }
        }

        ImGui::Separator();
        // --- Step 3: Custom binding ---
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "Step 3 - Add custom binding:");
        static char actionBuf[64] = "PlayerCustom";
        ImGui::InputText("Action Name", actionBuf, sizeof(actionBuf));
        ImGui::InputInt("Key Code", &m_S13NewKey);
        if (ImGui::Button("Add Binding", ImVec2(360, 24)))
        {
            auto* io = Resolve<IOHandler>();
            if (io)
            {
                io->GetInputManager().BindAction(actionBuf, InputType::Key, m_S13NewKey);
                m_S13Status = std::string("Bound '") + actionBuf + "' to key " + std::to_string(m_S13NewKey) + ". Save to persist.";
            }
        }

        ImGui::Separator();
        // --- Live state ---
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "Step 4 - Live action state:");
        auto* io = Resolve<IOHandler>();
        if (io)
        {
            auto& input = io->GetInputManager();
            const char* liveActions[] = {"PlayerForward", "PlayerBackward", "PlayerLeft", "PlayerRight", "PlayerJump"};
            for (const char* action : liveActions)
            {
                bool pressed = input.GetAction(action);
                ImGui::TextColored(pressed ? ImVec4(0.0f, 1.0f, 0.25f, 1.0f) : ImVec4(0.6f, 0.6f, 0.65f, 1.0f),
                                   "  %-20s %s", action, pressed ? "[ACTIVE]" : "[ idle ]");
            }
            ImGui::Separator();
            ImGui::Text("All current bindings:");
            for (auto& [action, binding] : input.GetActionMap())
            {
                std::string bindText;
                for (auto& b : binding.bindings)
                {
                    if      (b.type == InputType::Key)          bindText += "Key:" + std::to_string(b.code) + " ";
                    else if (b.type == InputType::MouseButton)  bindText += "Mouse:" + std::to_string(b.code) + " ";
                    else if (b.type == InputType::GamepadButton) bindText += "Gamepad:" + std::to_string(b.code) + " ";
                }
                ImGui::BulletText("%-20s %s", action.c_str(), bindText.c_str());
            }
        }
    }
    else if (m_CurrentScenario == 14)
    {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "Localization (l10n)");
        ImGui::Spacing();

        auto& l10n = GetSystem<LocalizationSystem>();
        ImGui::Text("Current Language: %s", l10n.GetLanguage().c_str());
        
        if (ImGui::Button("Switch to Vietnamese (vi)", ImVec2(360, 24)))
        {
            l10n.LoadLanguage("sample/resource/l10n/vi.axs", "vi");
            l10n.SetLanguage("vi");
        }
        if (ImGui::Button("Switch to English (en)", ImVec2(360, 24)))
        {
            l10n.LoadLanguage("sample/resource/l10n/en.axs", "en");
            l10n.SetLanguage("en");
        }

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.0f, 0.8f, 1.0f, 1.0f), "Localized Outputs:");
        ImGui::Text("App Title: %s", l10n.Get("app.title").c_str());
        ImGui::Text("Select Scenario Label: %s", l10n.Get("menu.select_scenario").c_str());
        
        int entCount = (int)GetScene().registry.storage<entt::entity>().size();
        ImGui::Text("Localized format check: %s", l10n.GetFormat("scenario.active_entities", std::to_string(entCount)).c_str());
    }
    else if (m_CurrentScenario == 15)
    {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "DataNote YAML Save/Load");
        ImGui::Text("Status: %s", m_S15Status.c_str());
        ImGui::Spacing();
        ImGui::SliderInt("Entity Count", &m_S15EntityCount, 1, 250);
        ImGui::SliderFloat("Entity Size", &m_S15EntitySize, 0.2f, 10.0f);

        if (ImGui::Button("Save data.axs", ImVec2(360, 24)))
        {
            std::unordered_map<std::string, DataNode> dataMap;
            DataNode dataNote;
            dataNote.attributes["EntityCount"] = std::to_string(m_S15EntityCount);
            dataNote.attributes["EntitySize"] = std::to_string(m_S15EntitySize);
            dataNote.value = "DataNote";
            dataMap["DataNote"] = dataNote;

            DataNodeSerializer serializer;
            if (serializer.Serialize(kScenario15DataPath, dataMap))
            {
                m_S15Status = "Serialized data to data.axs successfully.";
            }
            else
            {
                m_S15Status = "Failed to serialize data.";
            }
        }
        if (ImGui::Button("Load data.axs", ImVec2(360, 24)))
        {
            std::unordered_map<std::string, DataNode> dataMap;
            DataNodeSerializer serializer;
            const char* path = std::filesystem::exists(kScenario15DataPath)
                ? kScenario15DataPath
                : kScenario15DataLegacyPath;
            if (serializer.Deserialize(path, dataMap))
            {
                auto it = dataMap.find("DataNote");
                if (it == dataMap.end())
                    it = dataMap.find("PlayerStats");
                if (it != dataMap.end())
                {
                    auto& stats = it->second;
                    if (stats.attributes.find("EntityCount") != stats.attributes.end())
                        m_S15EntityCount = std::stoi(stats.attributes["EntityCount"]);
                    if (stats.attributes.find("EntitySize") != stats.attributes.end())
                        m_S15EntitySize = std::stof(stats.attributes["EntitySize"]);
                }
                m_S15Status = "Deserialized data successfully. Reload Scenario to apply entity count/size.";
            }
            else
            {
                m_S15Status = "Failed to deserialize data.axs.";
            }
        }
    }
    else if (m_CurrentScenario == 16)
    {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "Network Messaging (ENet)");
        ImGui::Text("Status: %s", m_S16Status.c_str());
        ImGui::Checkbox("Host as Server", &m_S16IsServer);
        ImGui::InputText("Host IP", m_S16Host, sizeof(m_S16Host));
        ImGui::InputInt("Port", &m_S16Port);
        m_S16Port = glm::clamp(m_S16Port, 1024, 65535);

        auto* sysMgr = Resolve<SystemManager>();
        if (sysMgr)
        {
            auto* netSystem = dynamic_cast<NetworkSystem*>(sysMgr->GetSystem("NetworkSystem"));
            if (netSystem)
            {
                ImGui::Text("Running: %s | Mode: %s | Connected peers: %zu",
                            netSystem->IsRunning() ? "yes" : "no",
                            netSystem->IsServer() ? "server" : (netSystem->IsClient() ? "client" : "idle"),
                            netSystem->GetConnectedPeerCount());
                if (netSystem->IsClient())
                    ImGui::Text("Client peer state: %s", netSystem->GetClientPeerState());
                if (ImGui::Button("Start Connection", ImVec2(170, 24)))
                {
                    if (netSystem->IsRunning()) netSystem->Stop();
                    NetworkConfig config;
                    config.port = static_cast<uint16_t>(m_S16Port);
                    config.maxClients = 32;
                    m_S16Messages.clear();

                    if (m_S16IsServer)
                    {
                        config.host.clear();
                        netSystem->SetOnConnect([this](ENetPeer*) {
                            m_S16Status = "Server: client connected.";
                            m_S16Messages.push_back("Client connected.");
                        });
                        netSystem->SetOnDisconnect([this](ENetPeer*) {
                            m_S16Status = "Server: client disconnected.";
                            m_S16Messages.push_back("Client disconnected.");
                        });
                        netSystem->SetOnMessage([this, netSystem](ENetPeer* peer, const uint8_t* data, size_t size, uint8_t) {
                            std::string msg((const char*)data, size);
                            if (!msg.empty() && msg.back() == '\0') msg.pop_back();
                            m_S16Messages.push_back("Server recvd: " + msg);
                            std::string echo = "Echo: " + msg;
                            netSystem->SendPacket(peer, echo.c_str(), echo.size() + 1);
                        });
                        m_S16Status = netSystem->StartServer(config) ?
                            "Server started on port " + std::to_string(m_S16Port) + "." :
                            "Failed to start server.";
                    }
                    else
                    {
                        config.host = m_S16Host;
                        netSystem->SetOnConnect([this](ENetPeer*) {
                            m_S16Status = "Client connected to " + std::string(m_S16Host) + ":" + std::to_string(m_S16Port) + ".";
                            m_S16Messages.push_back("Connected to server.");
                        });
                        netSystem->SetOnDisconnect([this](ENetPeer*) {
                            m_S16Status = "Client disconnected.";
                            m_S16Messages.push_back("Disconnected from server.");
                        });
                        netSystem->SetOnMessage([this](ENetPeer*, const uint8_t* data, size_t size, uint8_t) {
                            std::string msg((const char*)data, size);
                            if (!msg.empty() && msg.back() == '\0') msg.pop_back();
                            m_S16Messages.push_back("Client recvd: " + msg);
                        });
                        if (netSystem->StartClient(config))
                        {
                            m_S16Status = "Client connecting to " + std::string(m_S16Host) + ":" + std::to_string(m_S16Port) + "...";
                            netSystem->UpdateEvents(200);
                        }
                        else
                        {
                            m_S16Status = "Failed to initiate client connection.";
                        }
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Stop Connection", ImVec2(170, 24)))
                {
                    netSystem->Stop();
                    m_S16Status = "Stopped.";
                }
            }
        }

        ImGui::Separator();
        ImGui::InputText("Message", m_S16MessageText, sizeof(m_S16MessageText));
        if (ImGui::Button("Send Message", ImVec2(360, 24)))
        {
            if (auto* sysMgr2 = Resolve<SystemManager>())
            {
                if (auto* netSystem = dynamic_cast<NetworkSystem*>(sysMgr2->GetSystem("NetworkSystem")))
                {
                    std::string msg = m_S16MessageText;
                    if (netSystem->IsServer())
                    {
                        if (netSystem->GetConnectedPeerCount() > 0)
                        {
                            netSystem->BroadcastPacket(msg.c_str(), msg.size() + 1);
                            m_S16Messages.push_back("Server sent: " + msg);
                        }
                        else
                        {
                            m_S16Messages.push_back("Server has no connected clients.");
                        }
                    }
                    else if (auto* peer = netSystem->GetServerPeer())
                    {
                        netSystem->SendPacket(peer, msg.c_str(), msg.size() + 1);
                        m_S16Messages.push_back("Client sent: " + msg);
                    }
                    else
                    {
                        m_S16Messages.push_back(std::string("No connected peer. State: ") + netSystem->GetClientPeerState());
                    }
                }
            }
        }
        ImGui::Separator();
        ImGui::Text("Network Logs:");
        ImGui::BeginChild("NetLogList", ImVec2(0, 100), true);
        for (auto& m : m_S16Messages)
            ImGui::TextUnformatted(m.c_str());
        ImGui::EndChild();
    }
    else if (m_CurrentScenario == 17)
    {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "irrKlang Audio Test");
        ImGui::SliderFloat("Orbit Speed", &m_S17Speed, 0.1f, 5.0f);
        ImGui::SliderFloat("2D Volume", &m_S17Volume2D, 0.0f, 1.0f);
        ImGui::SliderFloat("3D Volume", &m_S17Volume3D, 0.0f, 1.0f);
        ImGui::SliderFloat("Playback Speed / Pitch", &m_S17Pitch, 0.25f, 3.0f);
        ImGui::SliderFloat("3D Min Distance", &m_S17MinDistance, 0.1f, 20.0f);
        ImGui::SliderFloat("3D Max Distance", &m_S17MaxDistance, 1.0f, 120.0f);
        
        auto* audioService = Resolve<AudioService>();
        if (audioService)
        {
            if (ImGui::Checkbox("Play 2D Sound (sample.mp3)", &m_S17Play2D))
            {
                if (m_S17Play2D)
                {
                    m_S17Audio2D = audioService->Play2D(SamplePath("sample/resource/audio/sample.wav"), true);
                    if (m_S17Audio2D)
                    {
                        m_S17Audio2D->SetVolume(m_S17Volume2D);
                        m_S17Audio2D->SetPitch(m_S17Pitch);
                    }
                }
                else
                {
                    if (m_S17Audio2D) { m_S17Audio2D->Stop(); m_S17Audio2D = nullptr; }
                }
            }

            if (ImGui::Checkbox("Play 3D Orbiting Sound", &m_S17Play3D))
            {
                if (m_S17Play3D)
                {
                    m_S17Audio3D = audioService->Play3D(SamplePath("sample/resource/audio/sample.wav"), glm::vec3(0.0f), true);
                    if (m_S17Audio3D)
                    {
                        m_S17Audio3D->SetVolume(m_S17Volume3D);
                        m_S17Audio3D->SetPitch(m_S17Pitch);
                        m_S17Audio3D->SetMinDistance(m_S17MinDistance);
                        m_S17Audio3D->SetMaxDistance(m_S17MaxDistance);
                    }
                }
                else
                {
                    if (m_S17Audio3D) { m_S17Audio3D->Stop(); m_S17Audio3D = nullptr; }
                }
            }

            if (ImGui::Button("Stop All Sounds", ImVec2(360, 24)))
            {
                audioService->StopAll();
                m_S17Play2D = false;
                m_S17Play3D = false;
                m_S17Audio2D = nullptr;
                m_S17Audio3D = nullptr;
            }
        }
    }
    else if (m_CurrentScenario == 18)
    {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "Video Mesh & UI Playback");
        ImGui::Checkbox("Loop Video", &m_S18VideoPlaying);
        
        auto view = GetScene().registry.view<VideoPlayerComponent>();
        for (auto entity : view)
        {
            auto& video = view.get<VideoPlayerComponent>(entity);
            if (video.decoder)
            {
                ImGui::PushID(static_cast<int>(static_cast<uint32_t>(entity)));
                ImGui::Text("Time: %.2f / %.2f s", video.decoder->GetCurrentTime(), video.decoder->GetDuration());
                ImGui::SliderFloat("Volume", &video.volume, 0.0f, 1.0f);
                ImGui::SliderFloat("Speed", &video.speed, 0.1f, 3.0f);
                
                if (ImGui::Button("Pause Video", ImVec2(170, 24))) PauseVideo(video);
                ImGui::SameLine();
                if (ImGui::Button("Play Video", ImVec2(170, 24))) PlayVideo(video);
                ImGui::PopID();
            }
        }
    }
    else if (m_CurrentScenario == 19)
    {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "Skeletal Animation & Blending");
        ImGui::Combo("Animation", &m_S19AnimIndex, "defeated\0spin\0");
        ImGui::SliderFloat("Speed", &m_S19Speed, 0.0f, 3.0f);
        ImGui::SliderFloat("Blend Factor", &m_S19Blend, 0.0f, 1.0f);

        auto view = GetScene().registry.view<AnimationComponent>();
        for (auto entity : view)
        {
            auto& anim = view.get<AnimationComponent>(entity);
            if (anim.animator)
            {
                ImGui::PushID(static_cast<int>(static_cast<uint32_t>(entity)));
                anim.speed = m_S19Speed;
                anim.blendFactor = m_S19Blend;

                if (ImGui::Button("Play Animation Directly", ImVec2(360, 24)))
                {
                    anim.animator->PlayAnimation(m_S19AnimIndex == 0 ? "defeated" : "spin");
                }
                if (ImGui::Button("CrossFade (1.0s Transition)", ImVec2(360, 24)))
                {
                    anim.animator->CrossFade(m_S19AnimIndex == 0 ? "defeated" : "spin", 1.0f);
                }
                if (ImGui::Button("Play Blend (A & B)", ImVec2(360, 24)))
                {
                    anim.animator->PlayBlend("defeated", "spin", m_S19Blend);
                }
                ImGui::PopID();
            }
        }
    }
    else if (m_CurrentScenario == 20)
    {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "SSR & Environment Probes");
        ImGui::SliderInt("Probe Resolution", &m_S20ProbeResolution, 128, 1024);
        ImGui::SliderFloat("Reflectivity", &m_S20Reflectivity, 0.0f, 1.0f);
        ImGui::SliderFloat("Fresnel Bias", &m_S20FresnelBias, 0.0f, 0.5f);
        ImGui::SliderFloat("Fresnel Power", &m_S20FresnelPower, 0.1f, 12.0f);

        auto refView = GetScene().registry.view<ReflectiveComponent>();
        for (auto entity : refView)
        {
            auto& ref = refView.get<ReflectiveComponent>(entity);
            ref.reflectivity = m_S20Reflectivity;
            ref.fresnelBias = m_S20FresnelBias;
            ref.fresnelPower = m_S20FresnelPower;
        }
        auto probeView = GetScene().registry.view<ReflectionProbeComponent>();
        for (auto entity : probeView)
        {
            auto& probe = probeView.get<ReflectionProbeComponent>(entity);
            if (probe.resolution != m_S20ProbeResolution)
            {
                probe.resolution = m_S20ProbeResolution;
                probe.isDirty = true;
            }
        }
        
        if (ImGui::Button("Force Probe Re-Capture", ImVec2(360, 24)))
        {
            auto view = GetScene().registry.view<ReflectionProbeComponent>();
            for (auto entity : view)
            {
                view.get<ReflectionProbeComponent>(entity).isDirty = true;
            }
        }
    }
    else if (m_CurrentScenario == 21)
    {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "Transparent Object Sorting");
        ImGui::SliderFloat("Glass Opacity", &m_S21GlassOpacity, 0.05f, 0.95f);
        ImGui::SliderFloat("Glass Roughness", &m_S21GlassRoughness, 0.0f, 1.0f);
        ImGui::Checkbox("Animate Objects", &m_S21AnimateObjects);

        auto view = GetScene().registry.view<AxisMaterialComponent, InfoComponent>();
        for (auto entity : view)
        {
            auto& info = view.get<InfoComponent>(entity);
            if (info.name.rfind("Glass_", 0) != 0)
                continue;
            auto& mat = view.get<AxisMaterialComponent>(entity);
            mat.desc.opacity = m_S21GlassOpacity;
            mat.desc.pbr.roughness = m_S21GlassRoughness;
            mat.gpu.dirty = true;
        }
    }
    else if (m_CurrentScenario == 22)
    {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "PBR Material Matrix");
        ImGui::Text("Rows: metallic 0.0 -> 1.0");
        ImGui::Text("Columns: roughness 0.05 -> 0.95");
    }
    else if (m_CurrentScenario == 23)
    {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "LOD Selection");
        ImGui::Text("Near: sphere, mid: cube, far: capsule");
        ImGui::Text("Orange center = solid reference (no LOD).");
        ImGui::Text("Move camera forward/back to cross LOD thresholds.");
    }
    else if (m_CurrentScenario == 24)
    {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "Layer Filter");
        bool layer0 = (m_S24LayerMask & 0x1) != 0;
        bool layer1 = (m_S24LayerMask & 0x2) != 0;
        bool layer2 = (m_S24LayerMask & 0x4) != 0;
        if (ImGui::Checkbox("Layer 0: neutral/floor", &layer0)) m_S24LayerMask = (m_S24LayerMask & ~0x1) | (layer0 ? 0x1 : 0);
        if (ImGui::Checkbox("Layer 1: red cubes", &layer1)) m_S24LayerMask = (m_S24LayerMask & ~0x2) | (layer1 ? 0x2 : 0);
        if (ImGui::Checkbox("Layer 2: blue spheres", &layer2)) m_S24LayerMask = (m_S24LayerMask & ~0x4) | (layer2 ? 0x4 : 0);
        if (auto camEntity = EntityManager::GetActiveCamera(GetScene()); camEntity != entt::null)
            GetScene().registry.get<CameraComponent>(camEntity).cullingMask = static_cast<uint32_t>(m_S24LayerMask);
    }
    else if (m_CurrentScenario == 25)
    {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "Render Order");
        if (ImGui::Checkbox("Reverse panel order", &m_S25ReverseOrder))
        {
            auto view = GetScene().registry.view<MeshRendererComponent, InfoComponent>();
            for (auto entity : view)
            {
                auto& info = view.get<InfoComponent>(entity);
                if (info.name == "Order_Red") view.get<MeshRendererComponent>(entity).order = m_S25ReverseOrder ? 30 : 10;
                if (info.name == "Order_Green") view.get<MeshRendererComponent>(entity).order = 20;
                if (info.name == "Order_Blue") view.get<MeshRendererComponent>(entity).order = m_S25ReverseOrder ? 10 : 30;
            }
        }
        ImGui::Text("Three transparent panels overlap at similar depth.");
    }
    else if (m_CurrentScenario == 26)
    {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "Instanced Batching");
        ImGui::SliderInt("Instance Count", &m_S26InstanceCount, 1000, 50000);
        ImGui::Checkbox("Unique Tint (break batches)", &m_S26UniqueTint);
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Reload to apply.");
    }
    else if (m_CurrentScenario == 27)
    {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "Deferred Shadow Receiver");
        ImGui::Text("Floor uses deferred_lit_shadow.");
        ImGui::Text("Objects cast shadows but do not self receive.");
    }
    else
    {
        if (ImGui::Checkbox("Show Physics / NavMesh Debug Lines", &m_ShowDebugLines))
        {
            auto& navSystem = GetSystem<NavigationSystem>();
            navSystem.SetShowDebug(m_ShowDebugLines);
        }
    }

    ImGui::Spacing();
    if (ImGui::Button("Reload Scenario", ImVec2(360, 30)))
    {
        m_PendingScenario = m_CurrentScenario;
    }

    ImGui::End();
}

void SampleState::LoadScene1()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    int count = 0;
    int size = static_cast<int>(std::ceil(std::pow(m_S1EntityCount, 1.0f / 3.0f))) + 2;
    float spacing = 3.5f;
    float offset = -(size * spacing) * 0.5f;

    std::string modelName = "sphereModel";
    if (m_S1MeshType == 1) modelName = "cubeModel";
    else if (m_S1MeshType == 2) modelName = "cylinderModel";
    else if (m_S1MeshType == 3) modelName = "capsuleModel";

    for (int x = 0; x < size && count < m_S1EntityCount; ++x)
    {
        for (int y = 0; y < size && count < m_S1EntityCount; ++y)
        {
            for (int z = 0; z < size && count < m_S1EntityCount; ++z)
            {
                glm::vec3 pos(
                    offset + x * spacing + static_cast<float>(rand() % 100) / 200.0f,
                    y * spacing + static_cast<float>(rand() % 100) / 200.0f,
                    offset + z * spacing + static_cast<float>(rand() % 100) / 200.0f
                );

                EntityBuilder(scene, res, "scenario")
                    .WithName("Entity_" + std::to_string(count))
                    .WithTransform(pos, glm::vec3(0.0f), glm::vec3(1.0f))
                    .WithMesh(modelName, "deferred_lit")
                    .WithPBRMaterial(0.1f, 0.5f, 1.0f)
                    .Build();

                count++;
            }
        }
    }
}

void SampleState::LoadScene2()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    EntityBuilder(scene, res, "scenario")
        .WithName("Floor")
        .WithTransform(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(100.0f, 1.0f, 100.0f))
        .WithMesh("planeModel", "deferred_lit_shadow")
        .WithPBRMaterial(0.0f, 0.8f, 1.0f)
        .Build();

    for (int i = 0; i < 1000; ++i)
    {
        float x = static_cast<float>(rand() % 2000) / 10.0f - 100.0f;
        float z = static_cast<float>(rand() % 2000) / 10.0f - 100.0f;
        float h = 1.0f + static_cast<float>(rand() % 50) / 10.0f;

        auto cube = EntityBuilder(scene, res, "scenario")
            .WithName("Cube_" + std::to_string(i))
            .WithTransform(glm::vec3(x, h * 0.5f + 0.5f, z), glm::vec3(0.0f, rand() % 360, 0.0f), glm::vec3(1.0f, h, 1.0f))
            .WithMesh("cubeModel", "deferred_lit_shadow")
            .WithPBRMaterial(0.1f, 0.6f, 1.0f)
            .Build();
        if (auto* renderer = scene.registry.try_get<MeshRendererComponent>(cube))
        {
            renderer->receiveShadow = false;
        }
    }

    auto dir = EntityBuilder(scene, res, "scenario")
        .WithName("DirLight")
        .WithTransform(glm::vec3(20.0f, 40.0f, 20.0f), glm::vec3(-45.0f, -45.0f, 0.0f), glm::vec3(2.0f))
        .WithMesh("sphereModel", "deferred_lit")
        .WithPBRMaterial(0.0f, 0.5f, 1.0f)
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.7f, -1.0f, -0.7f)), m_S2DirectionalColor, m_S2DirectionalIntensity)
        .Build();
    m_S2DirLightEntity = dir;
    auto& matDir = scene.registry.get<AxisMaterialComponent>(dir);
    matDir.desc.emission = m_S2DirectionalColor * (m_S2DirectionalIntensity * 3.0f);
    matDir.gpu.dirty = true;

    auto point = EntityBuilder(scene, res, "scenario")
        .WithName("PointLight")
        .WithTransform(glm::vec3(0.0f, 8.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.5f))
        .WithMesh("sphereModel", "deferred_lit")
        .WithPBRMaterial(0.0f, 0.5f, 1.0f)
        .WithPointLight(m_S2PointColor, m_S2PointIntensity, 30.0f)
        .Build();
    auto& matPoint = scene.registry.get<AxisMaterialComponent>(point);
    matPoint.desc.emission = m_S2PointColor * (m_S2PointIntensity * 2.0f);
    matPoint.gpu.dirty = true;
    scene.registry.get<PointLightComponent>(point).isCastShadow = true;

    auto spot = EntityBuilder(scene, res, "scenario")
        .WithName("SpotLight")
        .WithTransform(glm::vec3(m_S2SpotOrbitRadius, m_S2SpotMotionHeight, 0.0f), glm::vec3(-90.0f, 0.0f, 0.0f), glm::vec3(0.8f))
        .WithMesh("sphereModel", "deferred_lit")
        .WithPBRMaterial(0.0f, 0.5f, 1.0f)
        .WithSpotLight(glm::vec3(0.0f, -1.0f, 0.0f), m_S2SpotColor, m_S2SpotIntensity)
        .Build();
    auto& spotLight = scene.registry.get<SpotLightComponent>(spot);
    spotLight.cutOff = glm::cos(glm::radians(22.5f));
    spotLight.outerCutOff = glm::cos(glm::radians(32.5f));
    spotLight.linear = 0.045f;
    spotLight.quadratic = 0.0075f;
    scene.registry.get<RotationComponent>(spot).value =
        RotationFromNegativeY(glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f) - glm::vec3(m_S2SpotOrbitRadius, m_S2SpotMotionHeight, 0.0f)));
    auto& matSpot = scene.registry.get<AxisMaterialComponent>(spot);
    matSpot.desc.emission = m_S2SpotColor * (m_S2SpotIntensity * 1.25f);
    matSpot.gpu.dirty = true;
    scene.registry.get<SpotLightComponent>(spot).isCastShadow = true;
}

void SampleState::LoadScene3()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    EntityBuilder(scene, res, "scenario")
        .WithName("Floor")
        .WithTransform(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(100.0f, 1.0f, 100.0f))
        .WithMesh("planeModel", "deferred_lit")
        .WithPBRMaterial(0.0f, 0.8f, 1.0f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("Cylinder")
        .WithTransform(glm::vec3(0.0f, 5.0f, 0.0f), glm::vec3(0.0f), glm::vec3(5.0f, 10.0f, 5.0f))
        .WithMesh("cylinderModel", "deferred_lit")
        .WithPBRMaterial(0.2f, 0.4f, 1.0f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("DirLight_Key")
        .WithTransform(glm::vec3(0.0f, 30.0f, 0.0f), glm::vec3(-45.0f, 35.0f, 0.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.5f, -1.0f, -0.35f)), m_S3DirectionalColor,
                              m_S3DirectionalIntensity)
        .Build();

    for (int i = 0; i < 499; ++i)
    {
        float radius = 10.0f + static_cast<float>(rand() % 40);
        float angle = static_cast<float>(rand() % 360) * 3.14159f / 180.0f;
        float h = 1.0f + static_cast<float>(rand() % 100) / 10.0f;
        glm::vec3 pPos(cos(angle) * radius, h, sin(angle) * radius);
        glm::vec3 pColor = m_S3PointColor * (0.5f + static_cast<float>(rand() % 50) / 100.0f);
        EntityBuilder(scene, res, "scenario")
            .WithName("PointLight_" + std::to_string(i))
            .WithTransform(pPos)
            .WithPointLight(pColor, m_S3PointIntensity, 15.0f)
            .Build();

        float sRadius = 15.0f + static_cast<float>(rand() % 35);
        float sAngle = static_cast<float>(rand() % 360) * 3.14159f / 180.0f;
        glm::vec3 sPos(cos(sAngle) * sRadius, 15.0f, sin(sAngle) * sRadius);
        glm::vec3 sColor = m_S3SpotColor * (0.5f + static_cast<float>(rand() % 50) / 100.0f);
        EntityBuilder(scene, res, "scenario")
            .WithName("SpotLight_" + std::to_string(i))
            .WithTransform(sPos, glm::vec3(0.0f))
            .WithSpotLight(glm::vec3(0.0f, -1.0f, 0.0f), sColor, m_S3SpotIntensity)
            .Build();
    }

    auto spotView = scene.registry.view<PositionComponent, RotationComponent, SpotLightComponent, InfoComponent>();
    for (auto entity : spotView)
    {
        auto& info = spotView.get<InfoComponent>(entity);
        if (info.name.rfind("SpotLight_", 0) != 0)
            continue;
        auto& pos = spotView.get<PositionComponent>(entity);
        auto& rot = spotView.get<RotationComponent>(entity);
        auto& spot = spotView.get<SpotLightComponent>(entity);
        spot.direction = glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f) - pos.value);
        spot.cutOff = glm::cos(glm::radians(20.0f));
        spot.outerCutOff = glm::cos(glm::radians(30.0f));
        spot.linear = 0.07f;
        spot.quadratic = 0.017f;
        rot.value = RotationFromNegativeY(spot.direction);
    }
}

void SampleState::LoadScene4()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();
    if (auto* physics = Resolve<IPhysicsWorld>())
    {
        physics->SetGravity(m_S4Gravity);
    }

    EntityBuilder(scene, res, "scenario")
        .WithName("DirLight")
        .WithTransform(glm::vec3(0.0f, 40.0f, 0.0f), glm::vec3(-45.0f, -45.0f, 0.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.7f, -1.0f, -0.7f)), glm::vec3(1.0f), 1.2f)
        .Build();

    auto floor = EntityBuilder(scene, res, "scenario")
        .WithName("Floor")
        .WithTransform(glm::vec3(0.0f, -0.5f, 0.0f), glm::vec3(0.0f), glm::vec3(80.0f, 1.0f, 80.0f))
        .WithMesh("planeModel", "deferred_lit")
        .WithPBRMaterial(0.0f, 0.8f, 1.0f)
        .Build();

    auto& floorShape = EntityManager::AddComponent<RigidShapeComponent>(scene, floor);
    floorShape.type = ShapeType::Box;
    floorShape.size = glm::vec3(80.0f, 1.0f, 80.0f);
    floorShape.restitution = m_S4Restitution;
    floorShape.friction = m_S4Friction;

    auto& floorRB = EntityManager::AddComponent<RigidBodyComponent>(scene, floor);
    floorRB.mass = 0.0f;
    floorRB.isStatic = true;

    std::string modelName = "cubeModel";
    ShapeType st = ShapeType::Box;
    if (m_S4ShapeType == 1) { modelName = "sphereModel"; st = ShapeType::Sphere; }
    else if (m_S4ShapeType == 2) { modelName = "capsuleModel"; st = ShapeType::Capsule; }

    for (int i = 0; i < m_S4EntityCount; ++i)
    {
        float x = static_cast<float>((i % 10) - 5) * 2.5f + (static_cast<float>(rand() % 100) / 400.0f - 0.125f);
        float y = static_cast<float>(i / 100) * 2.5f + 5.0f;
        float z = static_cast<float>(((i / 10) % 10) - 5) * 2.5f + (static_cast<float>(rand() % 100) / 400.0f - 0.125f);

        auto bodyEntity = EntityBuilder(scene, res, "scenario")
            .WithName("PhysicsEntity_" + std::to_string(i))
            .WithTransform(glm::vec3(x, y, z), glm::vec3(rand() % 360, rand() % 360, rand() % 360), glm::vec3(1.0f))
            .WithMesh(modelName, "deferred_lit")
            .WithPBRMaterial(0.1f, 0.6f, 1.0f)
            .Build();

        auto& shape = EntityManager::AddComponent<RigidShapeComponent>(scene, bodyEntity);
        shape.type = st;
        shape.size = glm::vec3(1.0f);
        shape.radius = 0.5f;
        shape.height = 1.0f;
        shape.restitution = m_S4Restitution;
        shape.friction = m_S4Friction;

        auto& rb = EntityManager::AddComponent<RigidBodyComponent>(scene, bodyEntity);
        rb.mass = m_S4Mass;
        rb.isStatic = false;
    }
}

void SampleState::LoadScene5()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();
    auto& navSystem = GetSystem<NavigationSystem>();
    navSystem.ClearWalkableTags();
    navSystem.AddWalkableTag("walkable");
    navSystem.ClearCarveTags();
    navSystem.AddCarveTag("obstacle");

    EntityBuilder(scene, res, "scenario")
        .WithName("DirLight")
        .WithTransform(glm::vec3(0.0f, 40.0f, 0.0f), glm::vec3(-45.0f, -45.0f, 0.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.7f, -1.0f, -0.7f)), glm::vec3(1.0f), 1.2f)
        .Build();

    auto ground = EntityBuilder(scene, res, "scenario")
        .WithName("Ground")
        .WithTag("walkable")
        .WithTransform(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(50.0f, 1.0f, 50.0f))
        .WithMesh("planeModel", "deferred_lit")
        .WithPBRMaterial(0.0f, 0.9f, 1.0f)
        .Build();
    if (auto* renderer = scene.registry.try_get<MeshRendererComponent>(ground))
        renderer->color = glm::vec4(0.1f, 0.85f, 0.2f, 1.0f);

    auto navSurface = EntityBuilder(scene, res, "scenario")
        .WithName("NavigationGridSurface")
        .WithTag("walkable")
        .WithTransform(glm::vec3(-25.0f, 0.0f, -25.0f), glm::vec3(0.0f), glm::vec3(1.0f))
        .Build();
    auto& terrain = scene.registry.emplace<TerrainComponent>(navSurface);
    terrain.terrainSize = glm::vec3(50.0f, 0.0f, 50.0f);
    terrain.maxHeight = 0.0f;
    terrain.isWalkable = true;
    terrain.generatePhysics = false;

    const auto makeRoad = [&](const char* name, const glm::vec3& pos, const glm::vec3& scale) {
        auto road = EntityBuilder(scene, res, "scenario")
            .WithName(name)
            .WithTransform(pos, glm::vec3(0.0f), scale)
            .WithMesh("cubeModel", "deferred_unlit")
            .WithPBRMaterial(0.0f, 0.55f, 1.0f)
            .Build();
        if (auto* renderer = scene.registry.try_get<MeshRendererComponent>(road))
            renderer->color = glm::vec4(0.08f, 0.09f, 0.1f, 1.0f);
    };
    makeRoad("RoadNorthSouth", glm::vec3(-20.0f, 0.08f, 0.0f), glm::vec3(5.0f, 0.1f, 45.0f));
    makeRoad("RoadEastWest", glm::vec3(0.0f, 0.09f, -20.0f), glm::vec3(45.0f, 0.1f, 5.0f));

    auto groundPhys = EntityBuilder(scene, res, "scenario")
        .WithName("GroundPhys")
        .WithTransform(glm::vec3(0.0f, -0.5f, 0.0f), glm::vec3(0.0f), glm::vec3(1.0f))
        .Build();
    auto& groundShape = EntityManager::AddComponent<RigidShapeComponent>(scene, groundPhys);
    groundShape.type = ShapeType::Box;
    groundShape.size = glm::vec3(50.0f, 1.0f, 50.0f);
    auto& groundRB = EntityManager::AddComponent<RigidBodyComponent>(scene, groundPhys);
    groundRB.mass = 0.0f;
    groundRB.isStatic = true;

    // Dynamically generate obstacles based on count and size parameters
    for (int i = 0; i < m_S5ObstacleCount; ++i)
    {
        float ox = static_cast<float>(rand() % 36 - 18);
        float oz = static_cast<float>(rand() % 36 - 18);
        if (glm::length(glm::vec2(ox + 20.0f, oz - 20.0f)) < 6.0f ||
            glm::length(glm::vec2(ox - 20.0f, oz + 20.0f)) < 6.0f)
        {
            ox += 10.0f;
            oz -= 10.0f;
        }

        float obstacleHeight = 2.0f + static_cast<float>(rand() % 50) / 10.0f;
        glm::vec3 obstacleScale(m_S5ObstacleSize, obstacleHeight, m_S5ObstacleSize);

        auto obstacle = EntityBuilder(scene, res, "scenario")
            .WithName("Obstacle_" + std::to_string(i))
            .WithTag("obstacle")
            .WithTransform(glm::vec3(ox, obstacleHeight * 0.5f, oz), glm::vec3(0.0f), obstacleScale)
            .WithMesh("cubeModel", "deferred_lit")
            .WithPBRMaterial(0.3f, 0.3f, 1.0f)
            .Build();
        if (auto* renderer = scene.registry.try_get<MeshRendererComponent>(obstacle))
            renderer->color = glm::vec4(0.9f, 0.08f, 0.05f, 1.0f);

        auto obstacleCollider = EntityBuilder(scene, res, "scenario")
            .WithName("ObstacleCollider_" + std::to_string(i))
            .WithTransform(glm::vec3(ox, obstacleHeight * 0.5f, oz), glm::vec3(0.0f), glm::vec3(1.0f))
            .Build();

        auto& shape = EntityManager::AddComponent<RigidShapeComponent>(scene, obstacleCollider);
        shape.type = ShapeType::Box;
        shape.size = obstacleScale;
        shape.friction = 0.8f;
        auto& rb = EntityManager::AddComponent<RigidBodyComponent>(scene, obstacleCollider);
        rb.mass = 0.0f;
        rb.isStatic = true;
    }

    auto navMesh = scene.registry.create();
    scene.registry.emplace<InfoComponent>(navMesh).sceneName = "scenario";
    auto& navComp = scene.registry.emplace<NavMeshComponent>(navMesh);
    navComp.needsRebuild = true;
    navComp.isDynamic = true;

    m_NavFollower = EntityBuilder(scene, res, "scenario")
        .WithName("Follower")
        .WithTransform(glm::vec3(-20.0f, 0.5f, 20.0f), glm::vec3(0.0f), glm::vec3(1.5f))
        .WithMesh("capsuleModel", "deferred_lit")
        .WithPBRMaterial(0.1f, 0.5f, 1.0f)
        .WithPathFollower(m_S5FollowerSpeed, 15.0f, 30.0f, 60.0f)
        .Build();
    if (auto* renderer = scene.registry.try_get<MeshRendererComponent>(m_NavFollower))
        renderer->color = glm::vec4(0.1f, 0.9f, 0.25f, 1.0f);

    auto& pf = scene.registry.get<PathFollowerComponent>(m_NavFollower);
    pf.lockXPitch = m_S5LockXPitch;
    pf.lockYYaw = m_S5LockYYaw;
    pf.lockZRoll = m_S5LockZRoll;
    pf.pathfindingOptions.criteria = static_cast<PathfindingCriteria>(m_S5PathfindingCriteria);
    pf.pathfindingOptions.preferredTags = {"road"};
    pf.pathfindingOptions.tagWeightBonus = 30.0f;

    if (m_S5PathfindingCriteria == 0)
    {
        m_NavWaypoints = {
            glm::vec3(-20.0f, 0.5f, 20.0f),
            glm::vec3(20.0f, 0.5f, -20.0f)
        };
    }
    else if (m_S5PathfindingCriteria == 1)
    {
        m_NavWaypoints = {
            glm::vec3(-20.0f, 0.5f, 20.0f),
            glm::vec3(-8.0f, 0.5f, 10.0f),
            glm::vec3(0.0f, 0.5f, 0.0f),
            glm::vec3(10.0f, 0.5f, -8.0f),
            glm::vec3(20.0f, 0.5f, -20.0f)
        };
    }
    else
    {
        m_NavWaypoints = {
            glm::vec3(-20.0f, 0.5f, 20.0f),
            glm::vec3(-20.0f, 0.5f, -20.0f),
            glm::vec3(20.0f, 0.5f, -20.0f)
        };
    }
    m_CurrentWaypointIndex = 1;

    navSystem.SetShowDebug(m_ShowDebugLines);
    auto& transformSys = GetSystem<TransformSystem>();
    transformSys.m_IsLinearTransformsDirty = true;
    transformSys.Update(scene, 0.0f);
    auto& physicsSystem = GetSystem<PhysicsSystem>();
    physicsSystem.Update(scene, 0.0f);
    navSystem.Update(scene, 0.0f);
    auto navMeshView = scene.registry.view<NavMeshComponent>();
    for (auto entity : navMeshView)
    {
        auto& navMesh = navMeshView.get<NavMeshComponent>(entity);
        for (auto& tri : navMesh.triangles)
        {
            bool onRoad = (std::abs(tri.center.x + 20.0f) <= 2.9f && tri.center.z >= -22.5f && tri.center.z <= 22.5f) ||
                          (std::abs(tri.center.z + 20.0f) <= 2.9f && tri.center.x >= -22.5f && tri.center.x <= 22.5f);
            tri.tag = onRoad ? "road" : "walkable";
        }
        for (auto& node : navMesh.nodes)
        {
            bool onRoad = (std::abs(node.position.x + 20.0f) <= 2.9f && node.position.z >= -22.5f &&
                           node.position.z <= 22.5f) ||
                          (std::abs(node.position.z + 20.0f) <= 2.9f && node.position.x >= -22.5f &&
                           node.position.x <= 22.5f);
            node.tag = onRoad ? "road" : "walkable";
        }
    }
    navSystem.MoveTo(scene, m_NavFollower, m_NavWaypoints[m_CurrentWaypointIndex]);
}

void SampleState::LoadScene6()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    EntityBuilder(scene, res, "scenario")
        .WithName("DirLight")
        .WithTransform(glm::vec3(0.0f, 40.0f, 0.0f), glm::vec3(-45.0f, -45.0f, 0.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.7f, -1.0f, -0.7f)), glm::vec3(1.0f), 1.2f)
        .Build();

    for (int i = 0; i < 100; ++i)
    {
        float angle = (static_cast<float>(i) * 3.6f) * 3.14159f / 180.0f;
        float radius = 10.0f + static_cast<float>(i % 5) * 3.0f;
        glm::vec3 pos(cos(angle) * radius, 0.5f + static_cast<float>(i % 3) * 2.0f, sin(angle) * radius);

        std::string name = "ScriptedEntity_" + std::to_string(i);
        auto entity = EntityBuilder(scene, res, "scenario")
            .WithName(name)
            .WithTransform(pos, glm::vec3(0.0f), glm::vec3(1.0f))
            .WithMesh("cubeModel", "deferred_unlit")
            .WithPBRMaterial(0.1f, 0.5f, 1.0f)
            .Build();

        if (auto* renderer = scene.registry.try_get<MeshRendererComponent>(entity))
        {
            float hue = static_cast<float>(i) * 0.0618f;
            renderer->color = glm::vec4(
                0.35f + 0.65f * (0.5f + 0.5f * sin(hue * 6.28318f)),
                0.35f + 0.65f * (0.5f + 0.5f * sin(hue * 6.28318f + 2.09439f)),
                0.35f + 0.65f * (0.5f + 0.5f * sin(hue * 6.28318f + 4.18879f)),
                1.0f);
        }

        std::string scriptName;
        int scriptType = i % 6;
        if (scriptType == 0) scriptName = "OrbitScript";
        else if (scriptType == 1) scriptName = "PulseScaleScript";
        else if (scriptType == 2) scriptName = "ColorShiftScript";
        else if (scriptType == 3) scriptName = "RandomMoveScript";
        else if (scriptType == 4) scriptName = "RotateScript";
        else scriptName = "BouncingScript";

        auto& script = scene.registry.emplace<ScriptComponent>(entity);
        script.className = scriptName;
        script.InstantiateScript = [scriptName]() {
            auto registry = ServiceLocator::Instance().Resolve<IScriptRegistry>();
            return registry ? registry->Create(scriptName) : nullptr;
        };
        script.DestroyScript = [](ScriptComponent* nsc) { nsc->instance.reset(); };
    }
}

void SampleState::LoadScene7()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    EntityBuilder(scene, res, "scenario")
        .WithName("DirLight")
        .WithTransform(glm::vec3(0.0f, 40.0f, 0.0f), glm::vec3(-45.0f, -45.0f, 0.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.7f, -1.0f, -0.7f)), glm::vec3(1.0f), 1.2f)
        .Build();

    float angleStep = 360.0f / static_cast<float>(m_S7EmitterCount > 0 ? m_S7EmitterCount : 1);

    for (int i = 0; i < m_S7EmitterCount; ++i)
    {
        float angle = (static_cast<float>(i) * angleStep) * 3.14159f / 180.0f;
        float radius = 15.0f;
        glm::vec3 pos(cos(angle) * radius, 0.1f, sin(angle) * radius);

        auto emitterEntity = EntityBuilder(scene, res, "scenario")
            .WithName("Emitter_" + std::to_string(i))
            .WithTransform(pos, glm::vec3(0.0f))
            .Build();

        auto& pe = scene.registry.emplace<ParticleEmitterComponent>(emitterEntity);
        pe.isActive = true;
        pe.emitter.SpawnRate = m_S7SpawnRate;
        pe.emitter.LifeTime = m_S7LifeTime;
        pe.emitter.StartSize = m_S7StartSize;
        pe.emitter.EndSize = m_S7EndSize;

        glm::vec3 dir = glm::normalize(glm::vec3(pos.x, 10.0f, pos.z));
        pe.emitter.MinVelocity = dir * m_S7MinSpeed - glm::vec3(0.5f, 0.0f, 0.5f);
        pe.emitter.MaxVelocity = dir * m_S7MaxSpeed + glm::vec3(0.5f, m_S7VerticalSpeed, 0.5f);

        float r = 0.5f + 0.5f * sin(angle);
        float g = 0.5f + 0.5f * sin(angle + 2.0f);
        float b = 0.5f + 0.5f * sin(angle + 4.0f);
        pe.emitter.StartColor = glm::vec4(r, g, b, 1.0f);
        pe.emitter.EndColor = glm::vec4(r, g, b, 0.0f);

        pe.emitter.Initialize(1000);
    }
}

void SampleState::LoadScene8()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    auto floor = EntityBuilder(scene, res, "scenario")
        .WithName("Floor")
        .WithTransform(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(50.0f, 1.0f, 50.0f))
        .WithMesh("planeModel", "deferred_lit")
        .WithPBRMaterial(0.0f, 0.8f, 1.0f)
        .Build();
    auto& floorShape = EntityManager::AddComponent<RigidShapeComponent>(scene, floor);
    floorShape.type = ShapeType::Box;
    floorShape.size = glm::vec3(50.0f, 1.0f, 50.0f);
    floorShape.friction = 0.8f;
    auto& floorRB = EntityManager::AddComponent<RigidBodyComponent>(scene, floor);
    floorRB.mass = 0.0f;
    floorRB.isStatic = true;

    EntityBuilder(scene, res, "scenario")
        .WithName("DirLight")
        .WithTransform(glm::vec3(20.0f, 40.0f, 20.0f), glm::vec3(-45.0f, -45.0f, 0.0f), glm::vec3(1.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.7f, -1.0f, -0.7f)), glm::vec3(1.0f), 1.2f)
        .Build();

    // Controllable Player entity
    auto player = EntityBuilder(scene, res, "scenario")
        .WithName("PlayerCube")
        .WithTransform(glm::vec3(0.0f, 0.75f, 0.0f), glm::vec3(0.0f), glm::vec3(1.5f))
        .WithMesh("cubeModel", "deferred_lit")
        .WithPBRMaterial(0.1f, 0.4f, 1.0f)
        .Build();

    std::string scriptName = "PlayerControlScript";
    auto& script = scene.registry.emplace<ScriptComponent>(player);
    script.className = scriptName;
    script.InstantiateScript = [scriptName]() {
        auto registry = ServiceLocator::Instance().Resolve<IScriptRegistry>();
        return registry ? registry->Create(scriptName) : nullptr;
    };
    script.DestroyScript = [](ScriptComponent* nsc) { nsc->instance.reset(); };

    auto& playerShape = EntityManager::AddComponent<RigidShapeComponent>(scene, player);
    playerShape.type = ShapeType::Box;
    playerShape.size = glm::vec3(1.5f);
    playerShape.friction = 0.7f;
    auto& playerRB = EntityManager::AddComponent<RigidBodyComponent>(scene, player);
    playerRB.mass = 1.0f;
    playerRB.isStatic = false;
    playerRB.isKinematic = true;
    playerRB.linearFactor = glm::vec3(1.0f, 1.0f, 1.0f);
    playerRB.angularFactor = glm::vec3(0.0f, 1.0f, 0.0f);

    // Spawn some targets / visual obstacles to walk around
    for (int i = 0; i < 5; ++i)
    {
        float angle = static_cast<float>(i) * 72.0f * 3.14159f / 180.0f;
        float radius = 10.0f;
        glm::vec3 pos(cos(angle) * radius, 1.0f, sin(angle) * radius);

        auto target = EntityBuilder(scene, res, "scenario")
            .WithName("Target_" + std::to_string(i))
            .WithTransform(pos, glm::vec3(0.0f), glm::vec3(2.0f))
            .WithMesh("sphereModel", "deferred_lit")
            .WithPBRMaterial(0.9f, 0.1f, 1.0f)
            .Build();

        auto& shape = EntityManager::AddComponent<RigidShapeComponent>(scene, target);
        shape.type = ShapeType::Sphere;
        shape.radius = 1.0f;
        shape.friction = 0.5f;
        shape.restitution = 0.25f;
        auto& rb = EntityManager::AddComponent<RigidBodyComponent>(scene, target);
        rb.mass = 1.0f;
        rb.isStatic = false;
        rb.linearDamping = 0.25f;
        rb.angularDamping = 0.25f;
    }

    auto& transformSys = GetSystem<TransformSystem>();
    transformSys.m_IsLinearTransformsDirty = true;
    transformSys.Update(scene, 0.0f);
    GetSystem<PhysicsSystem>().Update(scene, 0.0f);
}

void SampleState::LoadScene9()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    EntityBuilder(scene, res, "scenario")
        .WithName("Floor")
        .WithTransform(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(60.0f, 1.0f, 60.0f))
        .WithMesh("planeModel", "deferred_lit")
        .WithPBRMaterial(0.0f, 0.9f, 1.0f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("DirLight")
        .WithTransform(glm::vec3(20.0f, 40.0f, 20.0f), glm::vec3(-45.0f, -45.0f, 0.0f), glm::vec3(1.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.7f, -1.0f, -0.7f)), glm::vec3(0.2f, 0.2f, 0.2f), 0.5f)
        .Build();

    struct GlowingObject {
        glm::vec3 pos;
        glm::vec3 emissionColor;
        float scale;
    };

    std::vector<GlowingObject> glowers = {
        { glm::vec3(-10.0f, 2.0f, 0.0f), glm::vec3(10.0f, 0.0f, 0.0f), 2.0f },
        { glm::vec3(0.0f, 2.0f, -10.0f), glm::vec3(0.0f, 10.0f, 0.0f), 2.0f },
        { glm::vec3(10.0f, 2.0f, 0.0f), glm::vec3(0.0f, 0.0f, 10.0f), 2.0f },
        { glm::vec3(0.0f, 3.0f, 10.0f), glm::vec3(10.0f, 10.0f, 0.0f), 3.0f },
        { glm::vec3(0.0f, 5.0f, 0.0f), glm::vec3(20.0f, 20.0f, 20.0f), 1.5f }
    };

    int index = 0;
    for (const auto& g : glowers)
    {
        auto ent = EntityBuilder(scene, res, "scenario")
            .WithName("Glower_" + std::to_string(index++))
            .WithTransform(g.pos, glm::vec3(0.0f), glm::vec3(g.scale))
            .WithMesh("sphereModel", "deferred_lit")
            .WithPBRMaterial(0.0f, 0.1f, 1.0f)
            .Build();

        auto& mat = scene.registry.get<AxisMaterialComponent>(ent);
        mat.desc.emission = g.emissionColor;
        mat.gpu.dirty = true;

        EntityBuilder(scene, res, "scenario")
            .WithName("GlowLight_" + std::to_string(index))
            .WithTransform(g.pos)
            .WithPointLight(glm::normalize(g.emissionColor), glm::length(g.emissionColor) * 0.5f, 15.0f)
            .Build();
    }

    m_PPBloomEnabled = true;
    m_PPBloomThreshold = 0.8f;
    m_PPBloomIntensity = 2.0f;
    m_PPBloomRadius = 0.005f;
    m_PPHdrEnabled = true;
    m_PPExposure = 1.0f;
    m_PPGamma = 2.2f;
    m_PPTonemappingMode = 1;

    auto ppEntity = EntityBuilder(scene, res, "scenario")
        .WithName("Scenario9PostProcess")
        .Build();
    auto& pp = scene.registry.emplace<PostProcessComponent>(ppEntity);
    pp.enabled = true;

    auto* sysMgr = Resolve<SystemManager>();
    if (sysMgr)
    {
        auto* ppSys = dynamic_cast<PostProcessSystem*>(sysMgr->GetSystem("PostProcessSystem"));
        if (ppSys)
        {
            auto& pipeline = ppSys->GetPipeline();
            pipeline.SetHDREnabled(m_PPHdrEnabled);
            pipeline.SetBloomEnabled(m_PPBloomEnabled);
            pipeline.SetBloomThreshold(m_PPBloomThreshold);
            pipeline.SetBloomIntensity(m_PPBloomIntensity);
            pipeline.SetBloomRadius(m_PPBloomRadius);
            pipeline.SetExposure(m_PPExposure);
            pipeline.SetGamma(m_PPGamma);
            pipeline.SetTonemappingMode(m_PPTonemappingMode);
        }
    }
}

void SampleState::LoadScene10()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();
    if (auto* physics = Resolve<IPhysicsWorld>())
    {
        physics->SetGravity(m_S10Gravity);
        physics->SetSolverIterations(24);
    }
    if (auto* collisionMatrix = Resolve<CollisionMatrix>())
    {
        collisionMatrix->IgnoreTagCollision("chain_link", "chain_link");
    }

    // 1. Static Floor & Lights
    EntityBuilder(scene, res, "scenario")
        .WithName("Floor")
        .WithTransform(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(80.0f, 1.0f, 80.0f))
        .WithMesh("planeModel", "deferred_lit")
        .WithPBRMaterial(0.2f, 0.8f, 1.0f)
        .Build();

    auto floorPhys = EntityBuilder(scene, res, "scenario")
        .WithName("FloorPhysics")
        .WithTransform(glm::vec3(0.0f, -0.5f, 0.0f), glm::vec3(0.0f), glm::vec3(1.0f))
        .Build();
    
    auto& floorShape = EntityManager::AddComponent<RigidShapeComponent>(scene, floorPhys);
    floorShape.type = ShapeType::Box;
    floorShape.size = glm::vec3(80.0f, 1.0f, 80.0f);
    floorShape.restitution = 0.5f;
    floorShape.friction = 0.5f;

    auto& floorRB = EntityManager::AddComponent<RigidBodyComponent>(scene, floorPhys);
    floorRB.mass = 0.0f;
    floorRB.isStatic = true;

    EntityBuilder(scene, res, "scenario")
        .WithName("DirLight")
        .WithTransform(glm::vec3(20.0f, 40.0f, 20.0f), glm::vec3(-45.0f, -45.0f, 0.0f), glm::vec3(1.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.7f, -1.0f, -0.7f)), glm::vec3(1.0f), 1.5f)
        .Build();

    // 2. Hanging anchor (Static body, high up)
    glm::vec3 anchorPos(0.0f, 22.0f, 0.0f);
    auto anchor = EntityBuilder(scene, res, "scenario")
        .WithName("Anchor")
        .WithTag("chain_link")
        .WithTransform(anchorPos, glm::vec3(0.0f), glm::vec3(1.5f))
        .WithMesh("cubeModel", "deferred_lit")
        .WithPBRMaterial(0.8f, 0.2f, 1.0f)
        .Build();

    auto& anchorShape = EntityManager::AddComponent<RigidShapeComponent>(scene, anchor);
    anchorShape.type = ShapeType::Box;
    anchorShape.size = glm::vec3(1.5f);
    anchorShape.restitution = 0.5f;
    anchorShape.friction = 0.5f;

    auto& anchorRB = EntityManager::AddComponent<RigidBodyComponent>(scene, anchor);
    anchorRB.mass = 0.0f;
    anchorRB.isStatic = true;
    anchorRB.linearDamping = 0.2f;
    anchorRB.angularDamping = 0.8f;

    // 3. Dynamically build chain components
    m_S10ChainEntities.clear();
    m_S10ChainEntities.push_back(anchor);

    float linkOffset = 1.1f;
    for (int i = 0; i < m_S10ChainLength; ++i)
    {
        glm::vec3 currentPos = anchorPos - glm::vec3(0.0f, linkOffset * (i + 1), 0.0f);
        
        auto link = EntityBuilder(scene, res, "scenario")
            .WithName("ChainLink_" + std::to_string(i))
            .WithTag("chain_link")
            .WithTransform(currentPos, glm::vec3(0.0f), glm::vec3(0.8f, 1.0f, 0.8f))
            .WithMesh("cubeModel", "deferred_lit")
            .WithPBRMaterial(0.1f, 0.5f, 1.0f)
            .Build();

        auto& shape = EntityManager::AddComponent<RigidShapeComponent>(scene, link);
        shape.type = ShapeType::Box;
        shape.size = glm::vec3(0.8f, 1.0f, 0.8f);
        shape.restitution = 0.15f;
        shape.friction = 0.65f;

        auto& rb = EntityManager::AddComponent<RigidBodyComponent>(scene, link);
        rb.mass = 1.5f;
        rb.isStatic = false;
        rb.linearDamping = 0.3f;
        rb.angularDamping = 0.9f;

        m_S10ChainEntities.push_back(link);
    }

    // Force PhysicsSystem to initialize bullet body structures immediately
    auto& physicsSys = GetSystem<PhysicsSystem>();
    physicsSys.Update(scene, 0.0f);

    // 4. Create constraints between consecutive chain links
    auto physics_ptr = Resolve<IPhysicsWorld>();
    if (physics_ptr)
    {
        for (size_t i = 1; i < m_S10ChainEntities.size(); ++i)
        {
            entt::entity prevEntity = m_S10ChainEntities[i - 1];
            entt::entity link = m_S10ChainEntities[i];

            auto& prevRBComp = scene.registry.get<RigidBodyComponent>(prevEntity);
            auto& linkRBComp = scene.registry.get<RigidBodyComponent>(link);

            if (prevRBComp.body && linkRBComp.body)
            {
                // Anchor is first element (index 0)
                glm::vec3 pivotA = (i == 1) ? glm::vec3(0.0f, -0.75f, 0.0f) : glm::vec3(0.0f, -0.52f, 0.0f);
                glm::vec3 pivotB = glm::vec3(0.0f, 0.52f, 0.0f);

                auto constraint = physics_ptr->CreateHingeConstraint(
                    prevRBComp.body,
                    linkRBComp.body,
                    pivotA, pivotB,
                    glm::vec3(1.0f, 0.0f, 0.0f),
                    glm::vec3(1.0f, 0.0f, 0.0f)
                );

                if (constraint)
                {
                    physics_ptr->AddConstraint(constraint);
                    prevRBComp.constraints.push_back(constraint);
                }
            }
        }

        // Apply a starting side kick so it swings immediately
        auto lastLink = m_S10ChainEntities.back();
        auto* rbLast = scene.registry.get<RigidBodyComponent>(lastLink).body.get();
        if (rbLast)
        {
            rbLast->Activate(true);
            rbLast->ApplyCentralForce(glm::vec3(35.0f, 0.0f, 0.0f));
        }
    }
}

void SampleState::LoadScene11()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    // 1. Static Floor & Lights
    EntityBuilder(scene, res, "scenario")
        .WithName("Floor")
        .WithTransform(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(80.0f, 1.0f, 80.0f))
        .WithMesh("planeModel", "deferred_lit")
        .WithPBRMaterial(0.0f, 0.8f, 1.0f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("DecalDirLight")
        .WithTransform(glm::vec3(20.0f, 40.0f, 20.0f), glm::vec3(-45.0f, -45.0f, 0.0f), glm::vec3(1.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.7f, -1.0f, -0.7f)), m_S11LightColor, m_S11LightIntensity)
        .Build();

    // 2. Spawn a large central wall to project decals onto
    EntityBuilder(scene, res, "scenario")
        .WithName("DecalWall")
        .WithTransform(glm::vec3(0.0f, 10.0f, -10.0f), glm::vec3(0.0f), glm::vec3(50.0f, 20.0f, 2.0f))
        .WithMesh("cubeModel", "deferred_lit")
        .WithPBRMaterial(0.1f, 0.9f, 1.0f)
        .Build();

    // 3. Use tint-only decals. The decal shader falls back to a white source when no texture is assigned.
    uint32_t decalTexId = 0;

    // 4. Spawn Decal Components in a grid on the wall
    for (int i = 0; i < m_S11DecalCount; ++i)
    {
        float x = -20.0f + static_cast<float>(i % 10) * 4.5f;
        float y = 3.0f + static_cast<float>(i / 10) * 4.0f;
        float z = -9.0f; 

        float size = m_S11DecalSize;
        auto decalEnt = EntityBuilder(scene, res, "scenario")
            .WithName("Decal_" + std::to_string(i))
            .WithTransform(glm::vec3(x, y, z), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(size, size, size))
            .Build();

        auto& decal = scene.registry.emplace<DecalComponent>(decalEnt);
        decal.albedoMap = decalTexId;
        decal.opacity = m_S11Opacity;
        decal.roughness = 0.5f;
        decal.metallic = 0.0f;
        
        if (m_S11RainbowMode)
        {
            float hue = static_cast<float>(i) / static_cast<float>(m_S11DecalCount);
            float r = 0.5f + 0.5f * sin(hue * 6.28318f);
            float g = 0.5f + 0.5f * sin(hue * 6.28318f + 2.09439f);
            float b = 0.5f + 0.5f * sin(hue * 6.28318f + 4.18879f);
            decal.tintColor = glm::vec4(r, g, b, 1.0f);
        }
        else
        {
            decal.tintColor = glm::vec4(m_S11Color, 1.0f);
        }
    }
}

void SampleState::LoadScene12()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    EntityBuilder(scene, res, "scenario")
        .WithName("Ground")
        .WithTransform(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(80.0f, 1.0f, 80.0f))
        .WithMesh("planeModel", "deferred_lit")
        .WithPBRMaterial(0.0f, 0.8f, 1.0f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("DirLight")
        .WithTransform(glm::vec3(20.0f, 40.0f, 20.0f), glm::vec3(-45.0f, -45.0f, 0.0f), glm::vec3(1.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.7f, -1.0f, -0.7f)), glm::vec3(1.0f), 1.5f)
        .Build();

    m_S12Status = "Scene initialized. Ready to Save or Load.";
}

void SampleState::LoadScene13()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    EntityBuilder(scene, res, "scenario")
        .WithName("Ground")
        .WithTransform(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(80.0f, 1.0f, 80.0f))
        .WithMesh("planeModel", "deferred_lit")
        .WithPBRMaterial(0.0f, 0.8f, 1.0f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("DirLight")
        .WithTransform(glm::vec3(20.0f, 40.0f, 20.0f), glm::vec3(-45.0f, -45.0f, 0.0f), glm::vec3(1.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.7f, -1.0f, -0.7f)), glm::vec3(1.0f), 1.5f)
        .Build();

    if (auto* io = Resolve<IOHandler>())
    {
        InputSerializer serializer;
        serializer.Deserialize("sample/resource/binding/binding.axs", io->GetInputManager());
    }

    auto player = EntityBuilder(scene, res, "scenario")
        .WithName("BindingPlayer")
        .WithTransform(glm::vec3(0.0f, 2.0f, 0.0f), glm::vec3(0.0f), glm::vec3(2.0f))
        .WithMesh("capsuleModel", "deferred_lit")
        .WithPBRMaterial(0.0f, 0.5f, 1.0f)
        .Build();

    auto& script = scene.registry.emplace<ScriptComponent>(player);
    script.className = "PlayerControlScript";
    script.InstantiateScript = []() {
        auto playerScript = std::make_unique<PlayerControlScript>();
        playerScript->allowMouseColor = false;
        return playerScript;
    };
    script.DestroyScript = [](ScriptComponent* nsc) { nsc->instance.reset(); };

    struct InputPad
    {
        const char* action;
        glm::vec3 position;
        glm::vec3 scale;
    };

    const InputPad pads[] = {
        {"PlayerForward", glm::vec3(0.0f, 0.12f, -8.0f), glm::vec3(3.0f, 0.15f, 3.0f)},
        {"PlayerLeft", glm::vec3(-4.0f, 0.12f, -4.0f), glm::vec3(3.0f, 0.15f, 3.0f)},
        {"PlayerBackward", glm::vec3(0.0f, 0.12f, -4.0f), glm::vec3(3.0f, 0.15f, 3.0f)},
        {"PlayerRight", glm::vec3(4.0f, 0.12f, -4.0f), glm::vec3(3.0f, 0.15f, 3.0f)},
        {"PlayerJump", glm::vec3(0.0f, 0.12f, 4.0f), glm::vec3(9.0f, 0.15f, 2.5f)},
    };

    for (const auto& pad : pads)
    {
        auto padEntity = EntityBuilder(scene, res, "scenario")
            .WithName(std::string("InputPad_") + pad.action)
            .WithTransform(pad.position, glm::vec3(0.0f), pad.scale)
            .WithMesh("cubeModel", "deferred_unlit")
            .WithPBRMaterial(0.0f, 0.5f, 1.0f)
            .Build();

        if (auto* renderer = scene.registry.try_get<MeshRendererComponent>(padEntity))
            renderer->color = glm::vec4(0.18f, 0.2f, 0.24f, 1.0f);
    }

    m_S13Status = "Loaded binding.axs. Press mapped controls to light pads and move the capsule.";
}

void SampleState::LoadScene14()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    EntityBuilder(scene, res, "scenario")
        .WithName("Ground")
        .WithTransform(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(80.0f, 1.0f, 80.0f))
        .WithMesh("planeModel", "deferred_lit")
        .WithPBRMaterial(0.1f, 0.1f, 0.1f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("DirLight")
        .WithTransform(glm::vec3(20.0f, 40.0f, 20.0f), glm::vec3(-45.0f, -45.0f, 0.0f), glm::vec3(1.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.7f, -1.0f, -0.7f)), glm::vec3(1.0f), 1.5f)
        .Build();

    auto viCube = EntityBuilder(scene, res, "scenario")
        .WithName("viCube")
        .WithTransform(glm::vec3(-10.0f, 3.0f, 0.0f), glm::vec3(0.0f, 45.0f, 0.0f), glm::vec3(4.0f))
        .WithMesh("cubeModel", "deferred_lit")
        .WithPBRMaterial(1.0f, 0.1f, 0.1f)
        .Build();
    auto* rVi = scene.registry.try_get<MeshRendererComponent>(viCube);
    if (rVi) rVi->color = glm::vec4(1.0f, 0.1f, 0.1f, 1.0f);

    auto enCube = EntityBuilder(scene, res, "scenario")
        .WithName("enCube")
        .WithTransform(glm::vec3(10.0f, 3.0f, 0.0f), glm::vec3(0.0f, 45.0f, 0.0f), glm::vec3(4.0f))
        .WithMesh("cubeModel", "deferred_lit")
        .WithPBRMaterial(0.1f, 0.1f, 1.0f)
        .Build();
    auto* rEn = scene.registry.try_get<MeshRendererComponent>(enCube);
    if (rEn) rEn->color = glm::vec4(0.1f, 0.1f, 1.0f, 1.0f);

    auto& l10n = GetSystem<LocalizationSystem>();
    l10n.LoadLanguage("sample/resource/l10n/vi.axs", "vi");
    l10n.LoadLanguage("sample/resource/l10n/en.axs", "en");
    l10n.SetLanguage("en");
}

void SampleState::LoadScene15()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    EntityBuilder(scene, res, "scenario")
        .WithName("Ground")
        .WithTransform(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(80.0f, 1.0f, 80.0f))
        .WithMesh("planeModel", "deferred_lit")
        .WithPBRMaterial(0.0f, 0.8f, 1.0f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("DirLight")
        .WithTransform(glm::vec3(20.0f, 40.0f, 20.0f), glm::vec3(-45.0f, -45.0f, 0.0f), glm::vec3(1.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.7f, -1.0f, -0.7f)), glm::vec3(1.0f), 1.5f)
        .Build();

    int columns = static_cast<int>(std::ceil(std::sqrt(static_cast<float>(m_S15EntityCount))));
    float spacing = (m_S15EntitySize * 1.6f > 1.5f) ? (m_S15EntitySize * 1.6f) : 1.5f;
    for (int i = 0; i < m_S15EntityCount; ++i)
    {
        int xIdx = i % columns;
        int zIdx = i / columns;
        glm::vec3 pos((xIdx - columns * 0.5f) * spacing, m_S15EntitySize * 0.5f, (zIdx - columns * 0.5f) * spacing);
        EntityBuilder(scene, res, "scenario")
            .WithName("DataEntity_" + std::to_string(i))
            .WithTransform(pos, glm::vec3(0.0f, i * 13.0f, 0.0f), glm::vec3(m_S15EntitySize))
            .WithMesh((i % 2 == 0) ? "cubeModel" : "sphereModel", "deferred_lit")
            .WithPBRMaterial(0.2f, 0.5f, 1.0f)
            .Build();
    }

    m_S15Status = "Ready. Save/load entity count and size, then reload to apply.";
}

void SampleState::LoadScene16()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    EntityBuilder(scene, res, "scenario")
        .WithName("Ground")
        .WithTransform(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(80.0f, 1.0f, 80.0f))
        .WithMesh("planeModel", "deferred_lit")
        .WithPBRMaterial(0.0f, 0.8f, 1.0f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("DirLight")
        .WithTransform(glm::vec3(20.0f, 40.0f, 20.0f), glm::vec3(-45.0f, -45.0f, 0.0f), glm::vec3(1.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.7f, -1.0f, -0.7f)), glm::vec3(1.0f), 1.5f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("NetworkOrb")
        .WithTransform(glm::vec3(0.0f, 5.0f, 0.0f), glm::vec3(0.0f), glm::vec3(3.0f))
        .WithMesh("sphereModel", "deferred_lit")
        .WithPBRMaterial(0.1f, 0.9f, 0.1f)
        .Build();

    m_S16Messages.clear();
    m_S16Status = "Network scenario initialized. Click Start to bind/connect.";
}

void SampleState::LoadScene17()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    EntityBuilder(scene, res, "scenario")
        .WithName("AudioPlatform")
        .WithTransform(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(60.0f, 1.0f, 60.0f))
        .WithMesh("planeModel", "deferred_lit")
        .WithPBRMaterial(0.2f, 0.8f, 0.2f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("DirLight")
        .WithTransform(glm::vec3(20.0f, 40.0f, 20.0f), glm::vec3(-45.0f, -45.0f, 0.0f), glm::vec3(1.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.7f, -1.0f, -0.7f)), glm::vec3(1.0f), 1.5f)
        .Build();

    auto audioSource = EntityBuilder(scene, res, "scenario")
        .WithName("AudioSource3D")
        .WithTransform(glm::vec3(0.0f, 3.0f, 0.0f), glm::vec3(0.0f), glm::vec3(2.0f))
        .WithMesh("sphereModel", "deferred_lit")
        .WithPBRMaterial(1.0f, 0.5f, 0.0f)
        .Build();

    auto& audio3D = scene.registry.emplace<AudioSourceComponent>(audioSource);
    audio3D.filePath = SamplePath("sample/resource/audio/sample.wav");
    audio3D.playOnAwake = true;
    audio3D.loop = true;
    audio3D.is3D = true;
    audio3D.volume = m_S17Volume3D;
    audio3D.pitch = m_S17Pitch;
    audio3D.speed = 1.0f;
    audio3D.minDistance = m_S17MinDistance;
    audio3D.maxDistance = m_S17MaxDistance;

    auto audio2D = EntityBuilder(scene, res, "scenario")
        .WithName("Audio2DLoop")
        .Build();
    auto& audio2DComp = scene.registry.emplace<AudioSourceComponent>(audio2D);
    audio2DComp.filePath = SamplePath("sample/resource/audio/sample.wav");
    audio2DComp.loop = true;
    audio2DComp.volume = m_S17Volume2D;
    audio2DComp.pitch = m_S17Pitch;

    m_S17Play2D = false;
    m_S17Play3D = true;
    m_S17Audio2D = nullptr;
    m_S17Audio3D = nullptr;
}

void SampleState::LoadScene18()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    EntityBuilder(scene, res, "scenario")
        .WithName("Ground")
        .WithTransform(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(80.0f, 1.0f, 80.0f))
        .WithMesh("planeModel", "deferred_lit")
        .WithPBRMaterial(0.0f, 0.8f, 1.0f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("DirLight")
        .WithTransform(glm::vec3(20.0f, 40.0f, 20.0f), glm::vec3(-45.0f, -45.0f, 0.0f), glm::vec3(1.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.7f, -1.0f, -0.7f)), glm::vec3(1.0f), 1.5f)
        .Build();

    auto tvScreen = EntityBuilder(scene, res, "scenario")
        .WithName("TVScreen")
        .WithTransform(glm::vec3(0.0f, 10.0f, 0.0f), glm::vec3(0.0f), glm::vec3(16.0f, 9.0f, 0.5f))
        .WithMesh("cubeModel", "videomapShader")
        .WithPBRMaterial(0.0f, 0.4f, 1.0f)
        .Build();
    if (auto* renderer = scene.registry.try_get<MeshRendererComponent>(tvScreen))
    {
        renderer->renderMode = RenderMode::ForceForward;
        renderer->color = glm::vec4(1.0f);
    }

    auto& video = scene.registry.emplace<VideoPlayerComponent>(tvScreen);
    video.filePath = SamplePath("sample/resource/video/sample.mp4");
    video.isLooping = true;
    video.playOnAwake = true;
    video.isPlaying = true;
    video.volume = m_S18Volume;
    video.maxDecodes = 3;

    auto uiVideo = EntityBuilder(scene, res, "scenario")
        .WithName("VideoPreviewUI")
        .WithUITransform(glm::vec2(420.0f, 30.0f), glm::vec2(320.0f, 180.0f), 10)
        .WithUIRenderer("video_preview_ui", glm::vec4(1.0f))
        .Build();
    auto& uiVideoPlayer = scene.registry.emplace<VideoPlayerComponent>(uiVideo);
    uiVideoPlayer.filePath = SamplePath("sample/resource/video/sample.mp4");
    uiVideoPlayer.isLooping = true;
    uiVideoPlayer.playOnAwake = true;
    uiVideoPlayer.isPlaying = true;
    uiVideoPlayer.volume = m_S18Volume;
    uiVideoPlayer.maxDecodes = 3;
}

void SampleState::LoadScene19()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    EntityBuilder(scene, res, "scenario")
        .WithName("Ground")
        .WithTransform(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(80.0f, 1.0f, 80.0f))
        .WithMesh("planeModel", "deferred_lit")
        .WithPBRMaterial(0.0f, 0.8f, 1.0f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("DirLight")
        .WithTransform(glm::vec3(20.0f, 40.0f, 20.0f), glm::vec3(-45.0f, -45.0f, 0.0f), glm::vec3(1.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.7f, -1.0f, -0.7f)), glm::vec3(1.0f), 1.5f)
        .Build();

    // Models must be loaded before clips because AnimationManager binds clips to the model skeleton.
    res.LoadModel("defeatedModel", "sample/resource/object/defeated.fbx");
    res.LoadModel("spinModel", "sample/resource/object/spin.fbx");
    res.LoadAnimation("defeated", "sample/resource/object/defeated.fbx", "defeatedModel");
    res.LoadAnimation("spin", "sample/resource/object/spin.fbx", "spinModel");

    auto defeatedEntity = EntityBuilder(scene, res, "scenario")
        .WithName("DefeatedFbxCharacter")
        .WithTransform(glm::vec3(-5.0f, 3.0f, 0.0f), glm::vec3(0.0f), glm::vec3(4.0f))
        .WithMesh("defeatedModel", "deferred_lit")
        .WithAnimation("defeated")
        .Build();

    auto spinEntity = EntityBuilder(scene, res, "scenario")
        .WithName("SpinFbxCharacter")
        .WithTransform(glm::vec3(5.0f, 3.0f, 0.0f), glm::vec3(0.0f), glm::vec3(4.0f))
        .WithMesh("spinModel", "deferred_lit")
        .WithAnimation("spin")
        .Build();

    auto& defeatedAnim = scene.registry.get<AnimationComponent>(defeatedEntity);
    defeatedAnim.animations.push_back("spin");
    if (defeatedAnim.animator)
    {
        auto spinAnim = res.GetAnimation("spin");
        if (spinAnim)
        {
            defeatedAnim.animator->AddAnimation("spin", spinAnim);
        }
    }

    auto& spinAnimComp = scene.registry.get<AnimationComponent>(spinEntity);
    spinAnimComp.animations.push_back("defeated");
    if (spinAnimComp.animator)
    {
        auto defeatedAnimClip = res.GetAnimation("defeated");
        if (defeatedAnimClip)
        {
            spinAnimComp.animator->AddAnimation("defeated", defeatedAnimClip);
        }
    }
}

void SampleState::LoadScene20()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    EntityBuilder(scene, res, "scenario")
        .WithName("DirLight")
        .WithTransform(glm::vec3(20.0f, 40.0f, 20.0f), glm::vec3(-45.0f, -45.0f, 0.0f), glm::vec3(1.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.7f, -1.0f, -0.7f)), glm::vec3(1.0f), 1.5f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("ReflectionFillLight")
        .WithTransform(glm::vec3(0.0f, 12.0f, 12.0f), glm::vec3(0.0f), glm::vec3(0.8f))
        .WithMesh("sphereModel", "deferred_lit")
        .WithPBRMaterial(0.0f, 0.3f, 1.0f)
        .WithPointLight(glm::vec3(1.0f, 0.92f, 0.75f), 12.0f, 45.0f)
        .Build();

    auto floor = EntityBuilder(scene, res, "scenario")
        .WithName("ReflectiveFloor")
        .WithTransform(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(80.0f, 1.0f, 80.0f))
        .WithMesh("planeModel", "deferred_lit")
        .WithPBRMaterial(0.02f, 0.35f, 1.0f)
        .Build();
    if (auto* floorRenderer = scene.registry.try_get<MeshRendererComponent>(floor))
        floorRenderer->color = glm::vec4(0.55f, 0.55f, 0.52f, 1.0f);

    auto planarMirror = EntityBuilder(scene, res, "scenario")
        .WithName("PlanarMirror")
        .WithTransform(glm::vec3(0.0f, 0.00f, -50.0f), glm::vec3(90.0f, 0.0f, 0.0f), glm::vec3(24.0f, 1.0f, 24.0f))
        .WithMesh("planeModel", "deferred_reflect")
        .WithPBRMaterial(0.0f, 0.04f, 1.0f)
        .Build();
    if (auto* mirrorRenderer = scene.registry.try_get<MeshRendererComponent>(planarMirror))
    {
        mirrorRenderer->color = glm::vec4(0.78f, 0.84f, 0.9f, 1.0f);
        mirrorRenderer->receiveShadow = false;
    }
    auto& planar = scene.registry.emplace<PlanarReflectionComponent>(planarMirror);
    planar.resolution = 1024;
    planar.resolution_y = 1024;
    planar.isDirty = true;
    auto& planarReflective = scene.registry.emplace<ReflectiveComponent>(planarMirror);
    planarReflective.reflectivity = 0.85f;
    planarReflective.fresnelBias = 0.08f;
    planarReflective.fresnelPower = 3.0f;
    planarReflective.enabled = true;

    auto wallRed = EntityBuilder(scene, res, "scenario")
        .WithName("WallRed")
        .WithTransform(glm::vec3(-20.0f, 5.0f, 0.0f), glm::vec3(0.0f), glm::vec3(2.0f, 10.0f, 20.0f))
        .WithMesh("cubeModel", "deferred_lit")
        .WithPBRMaterial(0.0f, 0.65f, 1.0f)
        .Build();
    auto* rRed = scene.registry.try_get<MeshRendererComponent>(wallRed);
    if (rRed) rRed->color = glm::vec4(1.0f, 0.1f, 0.1f, 1.0f);

    auto wallBlue = EntityBuilder(scene, res, "scenario")
        .WithName("WallBlue")
        .WithTransform(glm::vec3(20.0f, 5.0f, 0.0f), glm::vec3(0.0f), glm::vec3(2.0f, 10.0f, 20.0f))
        .WithMesh("cubeModel", "deferred_lit")
        .WithPBRMaterial(0.0f, 0.65f, 1.0f)
        .Build();
    auto* rBlue = scene.registry.try_get<MeshRendererComponent>(wallBlue);
    if (rBlue) rBlue->color = glm::vec4(0.1f, 0.1f, 1.0f, 1.0f);

    // Separate probe entity from reflective sphere to avoid self-reference issues
    auto probeEntity = EntityBuilder(scene, res, "scenario")
        .WithName("CenterProbe")
        .WithTransform(glm::vec3(0.0f, 6.0f, 0.0f), glm::vec3(0.0f), glm::vec3(1.0f))
        .Build();

    auto& probeComp = scene.registry.emplace<ReflectionProbeComponent>(probeEntity);
    probeComp.type = ReflectionProbeType::Dynamic;
    probeComp.resolution = m_S20ProbeResolution;
    probeComp.boxProjection = true;
    probeComp.boxMin = glm::vec3(-24.0f, -6.0f, -24.0f);
    probeComp.boxMax = glm::vec3(24.0f, 16.0f, 24.0f);
    probeComp.blendDistance = 6.0f;
    probeComp.isDirty = true;

    auto sphere = EntityBuilder(scene, res, "scenario")
        .WithName("ReflectiveSphere")
        .WithTransform(glm::vec3(0.0f, 6.0f, 0.0f), glm::vec3(0.0f), glm::vec3(4.0f))
        .WithMesh("sphereModel", "deferred_reflect")
        .WithPBRMaterial(0.05f, 0.95f, 1.0f)
        .Build();

    auto& refComp = scene.registry.emplace<ReflectiveComponent>(sphere);
    refComp.reflectivity = m_S20Reflectivity;
    refComp.fresnelPower = m_S20FresnelPower;
    refComp.fresnelBias = m_S20FresnelBias;
    refComp.enabled = true;
    refComp.targetProbe = "CenterProbe";
}

void SampleState::LoadScene21()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    EntityBuilder(scene, res, "scenario")
        .WithName("DirLight")
        .WithTransform(glm::vec3(20.0f, 40.0f, 20.0f), glm::vec3(-45.0f, -45.0f, 0.0f), glm::vec3(1.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.7f, -1.0f, -0.7f)), glm::vec3(1.0f), 1.4f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("TransparentGround")
        .WithTransform(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(70.0f, 1.0f, 70.0f))
        .WithMesh("planeModel", "deferred_lit")
        .WithPBRMaterial(0.0f, 0.75f, 1.0f)
        .Build();

    auto mover = EntityBuilder(scene, res, "scenario")
        .WithName("OpaqueMover")
        .WithTransform(glm::vec3(0.0f, 2.0f, -1.5f), glm::vec3(0.0f), glm::vec3(2.5f))
        .WithMesh("sphereModel", "deferred_lit")
        .WithPBRMaterial(0.0f, 0.35f, 1.0f)
        .Build();
    if (auto* renderer = scene.registry.try_get<MeshRendererComponent>(mover))
        renderer->color = glm::vec4(1.0f, 0.72f, 0.1f, 1.0f);

    for (int i = 0; i < 5; ++i)
    {
        float x = -12.0f + i * 6.0f;
        auto glass = EntityBuilder(scene, res, "scenario")
            .WithName("Glass_" + std::to_string(i))
            .WithTransform(glm::vec3(x, 4.0f, 0.0f), glm::vec3(0.0f, i * 14.0f, 0.0f), glm::vec3(3.0f, 7.0f, 0.35f))
            .WithMesh("cubeModel", "forward_transparent")
            .WithPBRMaterial(0.0f, m_S21GlassRoughness, 1.0f)
            .Build();

        if (auto* renderer = scene.registry.try_get<MeshRendererComponent>(glass))
        {
            renderer->renderMode = RenderMode::ForceForward;
            renderer->color = glm::vec4(0.2f + 0.12f * i, 0.75f, 1.0f, 1.0f);
            renderer->castShadow = false;
        }
        if (auto* mat = scene.registry.try_get<AxisMaterialComponent>(glass))
        {
            mat->desc.opacity = m_S21GlassOpacity;
            mat->desc.blendSrc = BlendFactor::SrcAlpha;
            mat->desc.blendDst = BlendFactor::OneMinusSrcAlpha;
            mat->gpu.dirty = true;
        }
    }
}

void SampleState::LoadScene22()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    EntityBuilder(scene, res, "scenario")
        .WithName("DirLight")
        .WithTransform(glm::vec3(20.0f, 40.0f, 20.0f), glm::vec3(-45.0f, -45.0f, 0.0f), glm::vec3(1.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.7f, -1.0f, -0.7f)), glm::vec3(1.0f), 1.2f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("WarmKeyLight")
        .WithTransform(glm::vec3(-14.0f, 8.0f, 10.0f), glm::vec3(0.0f), glm::vec3(0.8f))
        .WithMesh("sphereModel", "deferred_lit")
        .WithPBRMaterial(0.0f, 0.3f, 1.0f)
        .WithPointLight(glm::vec3(1.0f, 0.72f, 0.45f), 18.0f, 35.0f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("PBRFloor")
        .WithTransform(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(80.0f, 1.0f, 80.0f))
        .WithMesh("planeModel", "deferred_lit")
        .WithPBRMaterial(0.0f, 0.8f, 1.0f)
        .Build();

    for (int row = 0; row < 5; ++row)
    {
        for (int col = 0; col < 5; ++col)
        {
            float metallic = row / 4.0f;
            float roughness = 0.05f + col * 0.225f;
            auto sphere = EntityBuilder(scene, res, "scenario")
                .WithName("PBR_" + std::to_string(row) + "_" + std::to_string(col))
                .WithTransform(glm::vec3((col - 2) * 5.0f, 2.0f, (row - 2) * 5.0f), glm::vec3(0.0f), glm::vec3(2.0f))
                .WithMesh("sphereModel", "deferred_lit")
                .WithPBRMaterial(metallic, roughness, 1.0f)
                .Build();
            if (auto* renderer = scene.registry.try_get<MeshRendererComponent>(sphere))
                renderer->color = glm::vec4(0.9f, 0.82f, 0.62f, 1.0f);
        }
    }
}

void SampleState::LoadScene23()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    EntityBuilder(scene, res, "scenario")
        .WithName("LODDirLight")
        .WithTransform(glm::vec3(20.0f, 35.0f, 25.0f), glm::vec3(-45.0f, -35.0f, 0.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.6f, -1.0f, -0.5f)), glm::vec3(1.0f), 1.4f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("LODFloor")
        .WithTransform(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(90.0f, 1.0f, 120.0f))
        .WithMesh("planeModel", "deferred_lit_shadow")
        .WithPBRMaterial(0.0f, 0.85f, 1.0f)
        .Build();

    auto midModel = res.GetModel("cubeModel");
    auto farModel = res.GetModel("capsuleModel");
    for (int i = 0; i < 12; ++i)
    {
        float x = -22.0f + (i % 4) * 14.5f;
        float z = 28.0f - (i / 4) * 28.0f;
        auto entity = EntityBuilder(scene, res, "scenario")
            .WithName("LOD_" + std::to_string(i))
            .WithTransform(glm::vec3(x, 3.0f, z), glm::vec3(0.0f, i * 15.0f, 0.0f), glm::vec3(2.8f))
            .WithMesh("sphereModel", "deferred_lit_shadow")
            .WithPBRMaterial(0.1f, 0.45f, 1.0f)
            .Build();

        auto& lod = scene.registry.emplace<LODComponent>(entity);
        lod.lodDistancesSq = {900.0f, 2500.0f};
        lod.lodModels = {midModel, farModel};
        if (auto* renderer = scene.registry.try_get<MeshRendererComponent>(entity))
            renderer->color = glm::vec4(0.25f + 0.05f * i, 0.8f, 0.4f, 1.0f);
    }

    // Solid reference entity (no LOD) so user can compare LOD swaps against a stable mesh
    auto solidRef = EntityBuilder(scene, res, "scenario")
        .WithName("LOD_SolidReference")
        .WithTransform(glm::vec3(0.0f, 10.0f, 0.0f), glm::vec3(0.0f), glm::vec3(3.5f))
        .WithMesh("sphereModel", "deferred_lit_shadow")
        .WithPBRMaterial(0.8f, 0.2f, 1.0f)
        .Build();
    if (auto* renderer = scene.registry.try_get<MeshRendererComponent>(solidRef))
        renderer->color = glm::vec4(1.0f, 0.35f, 0.1f, 1.0f);
}

void SampleState::LoadScene24()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();
    m_S24LayerMask = 0x7;

    EntityBuilder(scene, res, "scenario")
        .WithName("LayerDirLight")
        .WithTransform(glm::vec3(20.0f, 35.0f, 20.0f), glm::vec3(-45.0f, -45.0f, 0.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.6f, -1.0f, -0.6f)), glm::vec3(1.0f), 1.3f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("LayerFloor")
        .WithLayer(0x1)
        .WithTransform(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(80.0f, 1.0f, 80.0f))
        .WithMesh("planeModel", "deferred_lit")
        .WithPBRMaterial(0.0f, 0.8f, 1.0f)
        .Build();

    for (int i = 0; i < 8; ++i)
    {
        auto cube = EntityBuilder(scene, res, "scenario")
            .WithName("LayerRedCube_" + std::to_string(i))
            .WithLayer(0x2)
            .WithTransform(glm::vec3(-18.0f + i * 5.0f, 2.0f, -8.0f), glm::vec3(0.0f, i * 20.0f, 0.0f), glm::vec3(2.0f))
            .WithMesh("cubeModel", "deferred_lit")
            .WithPBRMaterial(0.0f, 0.55f, 1.0f)
            .Build();
        scene.registry.get<MeshRendererComponent>(cube).color = glm::vec4(1.0f, 0.15f, 0.1f, 1.0f);

        auto sphere = EntityBuilder(scene, res, "scenario")
            .WithName("LayerBlueSphere_" + std::to_string(i))
            .WithLayer(0x4)
            .WithTransform(glm::vec3(-18.0f + i * 5.0f, 2.0f, 8.0f), glm::vec3(0.0f), glm::vec3(2.0f))
            .WithMesh("sphereModel", "deferred_lit")
            .WithPBRMaterial(0.0f, 0.35f, 1.0f)
            .Build();
        scene.registry.get<MeshRendererComponent>(sphere).color = glm::vec4(0.1f, 0.35f, 1.0f, 1.0f);
    }
}

void SampleState::LoadScene25()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();
    m_S25ReverseOrder = false;

    EntityBuilder(scene, res, "scenario")
        .WithName("OrderLight")
        .WithTransform(glm::vec3(0.0f, 18.0f, 18.0f), glm::vec3(-45.0f, 0.0f, 0.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(0.0f, -1.0f, -0.4f)), glm::vec3(1.0f), 1.1f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("OrderFloor")
        .WithTransform(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(60.0f, 1.0f, 60.0f))
        .WithMesh("planeModel", "deferred_lit")
        .WithPBRMaterial(0.0f, 0.8f, 1.0f)
        .Build();

    struct PanelDef
    {
        const char* name;
        glm::vec3 pos;
        glm::vec4 color;
        int order;
    };
    PanelDef panels[] = {
        {"Order_Red", glm::vec3(-1.6f, 5.0f, 0.0f), glm::vec4(1.0f, 0.1f, 0.1f, 0.55f), 10},
        {"Order_Green", glm::vec3(0.0f, 5.0f, 0.0f), glm::vec4(0.1f, 1.0f, 0.25f, 0.55f), 20},
        {"Order_Blue", glm::vec3(1.6f, 5.0f, 0.0f), glm::vec4(0.15f, 0.35f, 1.0f, 0.55f), 30},
    };

    for (const auto& panel : panels)
    {
        auto entity = EntityBuilder(scene, res, "scenario")
            .WithName(panel.name)
            .WithTransform(panel.pos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(5.0f, 7.0f, 0.25f))
            .WithMesh("cubeModel", "forward_transparent")
            .WithPBRMaterial(0.0f, 0.2f, 1.0f)
            .Build();
        auto& renderer = scene.registry.get<MeshRendererComponent>(entity);
        renderer.renderMode = RenderMode::ForceForward;
        renderer.castShadow = false;
        renderer.order = panel.order;
        renderer.color = panel.color;

        auto& mat = scene.registry.get<AxisMaterialComponent>(entity);
        mat.desc.opacity = panel.color.a;
        mat.desc.blendSrc = BlendFactor::SrcAlpha;
        mat.desc.blendDst = BlendFactor::OneMinusSrcAlpha;
        mat.gpu.dirty = true;
    }

    // struct SolidDef
    // {
    //     const char* name;
    //     glm::vec3 pos;
    //     glm::vec4 color;
    //     int order;
    // };
    // SolidDef solids[] = {
    //     {"Order_Red", glm::vec3(-1.6f, 5.0f, 5.0f), glm::vec4(1.0f, 0.1f, 0.1f, 0.55f), 10},
    //     {"Order_Green", glm::vec3(0.0f, 5.0f, 5.0f), glm::vec4(0.1f, 1.0f, 0.25f, 0.55f), 20},
    //     {"Order_Blue", glm::vec3(1.6f, 5.0f, 5.0f), glm::vec4(0.15f, 0.35f, 1.0f, 0.55f), 30},
    // };

    // for (const auto& solid : solids)
    // {
    //     auto entity = EntityBuilder(scene, res, "scenario")
    //         .WithName(solid.name)
    //         .WithTransform(solid.pos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(5.0f, 7.0f, 0.25f))
    //         .WithMesh("cubeModel", "deferred_lit")
    //         .WithPBRMaterial(0.0f, 0.2f, 1.0f)
    //         .Build();
    //     auto& renderer = scene.registry.get<MeshRendererComponent>(entity);
    //     renderer.renderMode = RenderMode::ForceForward;
    //     renderer.castShadow = false;
    //     renderer.order = solid.order;
    //     renderer.color = solid.color;
    // }
}

void SampleState::LoadScene26()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    EntityBuilder(scene, res, "scenario")
        .WithName("BatchLight")
        .WithTransform(glm::vec3(25.0f, 45.0f, 25.0f), glm::vec3(-45.0f, -45.0f, 0.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.6f, -1.0f, -0.6f)), glm::vec3(1.0f), 1.2f)
        .Build();

    EntityBuilder(scene, res, "scenario")
        .WithName("BatchFloor")
        .WithTransform(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(180.0f, 1.0f, 180.0f))
        .WithMesh("planeModel", "deferred_lit")
        .WithPBRMaterial(0.0f, 0.85f, 1.0f)
        .Build();

    int side = static_cast<int>(std::ceil(std::sqrt(static_cast<float>(m_S26InstanceCount))));
    float spacing = 2.7f;
    float offset = -side * spacing * 0.5f;
    for (int i = 0; i < m_S26InstanceCount; ++i)
    {
        int x = i % side;
        int z = i / side;
        auto entity = EntityBuilder(scene, res, "scenario")
            .WithName("BatchCube_" + std::to_string(i))
            .WithTransform(glm::vec3(offset + x * spacing, 1.3f, offset + z * spacing), glm::vec3(0.0f), glm::vec3(1.0f))
            .WithMesh("cubeModel", "deferred_lit")
            .WithPBRMaterial(0.05f, 0.55f, 1.0f)
            .Build();

        if (m_S26UniqueTint)
        {
            float hue = static_cast<float>(i % 97) / 97.0f;
            scene.registry.get<MeshRendererComponent>(entity).color =
                glm::vec4(0.35f + 0.55f * hue, 0.55f, 1.0f - 0.5f * hue, 1.0f);
        }
    }
}

void SampleState::LoadScene27()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();

    EntityBuilder(scene, res, "scenario")
        .WithName("ShadowReceiverFloor")
        .WithTransform(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(85.0f, 1.0f, 85.0f))
        .WithMesh("planeModel", "deferred_lit_shadow")
        .WithPBRMaterial(0.0f, 0.8f, 1.0f)
        .Build();

    auto casterA = EntityBuilder(scene, res, "scenario")
        .WithName("ShadowCasterCube")
        .WithTransform(glm::vec3(-6.0f, 5.0f, 0.0f), glm::vec3(0.0f, 25.0f, 0.0f), glm::vec3(4.0f, 8.0f, 4.0f))
        .WithMesh("cubeModel", "deferred_lit_shadow")
        .WithPBRMaterial(0.1f, 0.5f, 1.0f)
        .Build();
    {
        auto& r = scene.registry.get<MeshRendererComponent>(casterA);
        r.receiveShadow = false;
        r.castShadow = true;
    }

    auto casterB = EntityBuilder(scene, res, "scenario")
        .WithName("ShadowCasterSphere")
        .WithTransform(glm::vec3(7.0f, 5.0f, -3.0f), glm::vec3(0.0f), glm::vec3(4.0f))
        .WithMesh("sphereModel", "deferred_lit_shadow")
        .WithPBRMaterial(0.0f, 0.35f, 1.0f)
        .Build();
    {
        auto& r = scene.registry.get<MeshRendererComponent>(casterB);
        r.receiveShadow = false;
        r.castShadow = true;
    }

    auto light = EntityBuilder(scene, res, "scenario")
        .WithName("DeferredShadowDirLight")
        .WithTransform(glm::vec3(25.0f, 40.0f, 20.0f), glm::vec3(-50.0f, -40.0f, 0.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.55f, -1.0f, -0.35f)), glm::vec3(1.0f), 1.4f)
        .Build();
    if (auto* dir = scene.registry.try_get<DirectionalLightComponent>(light))
        dir->isCastShadow = true;
}
