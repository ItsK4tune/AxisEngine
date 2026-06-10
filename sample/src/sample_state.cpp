#include "sample_state.h"
#include <core/logic/event_manager.h>
#include <ecs/logic/camera_system.h>
#include <ecs/logic/post_process_system.h>
#include <ecs/unit/post_process_component.h>
#include <platform/logic/input_serializer.h>
#include <core/logic/data_node_serializer.h>
#include <network/network_system.h>
#include <physics/logic/collision_matrix.h>
#include <physics/logic/physics_query_service.h>
#include <physics/unit/ray.h>
#include <audio/logic/audio_service.h>
#include <core/logic/backend_registry.h>
#include <core/logic/config_manager.h>
#include <audio/interface/i_sound.h>
#include <ecs/unit/media_components.h>
#include <ecs/unit/network_components.h>
#include <ecs/unit/reflection_components.h>
#include <ecs/interface/i_render_state_service.h>
#include <ecs/unit/light_probe_components.h>
#include <resource/type/resource_events.h>
#include <scene/logic/scene_serializer.h>
#ifdef ENABLE_EDITOR
#include <editor/editor_system.h>
#include <imgui.h>
#endif
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <glm/gtx/quaternion.hpp>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dxgi1_4.h>
#include <pdh.h>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "pdh.lib")

#ifdef GetCurrentTime
#undef GetCurrentTime
#endif
#endif

namespace
{
constexpr const char* kScenario25BaseSceneName = "scenario_base";
constexpr const char* kScenario25DynamicSceneName = "scenario";
constexpr const char* kScenario31Audio2DName = "Audio2DLoop";
constexpr const char* kScenario31Audio3DName = "AudioSource3D";

struct PerfStats
{
    float cpu = 0.0f;
    float gpu = 0.0f;
    float ram = 0.0f;
    float vram = 0.0f;
};

PerfStats QueryPerfStats()
{
#if defined(_WIN32)
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
            stats.cpu = glm::clamp((1.0f - static_cast<float>(idleDelta) / static_cast<float>(sysDelta)) * 100.0f, 0.0f,
                                   100.0f);
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
#else
    return {};
#endif
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

constexpr const char* kScenario25ScenePath = "sample/scene/sample.axs";
constexpr const char* kScenario25SceneLegacyPath = "sample/scene/sample.axis";
constexpr const char* kScenario27DataPath = "sample/resource/data/data.axs";
constexpr const char* kScenario27DataLegacyPath = "sample/resource/data/data.axis";

glm::quat RotationFromNegativeY(const glm::vec3& direction)
{
    glm::vec3 dir = glm::normalize(direction);
    if (!std::isfinite(dir.x) || !std::isfinite(dir.y) || !std::isfinite(dir.z) || glm::length2(dir) < 0.0001f)
        return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    return glm::rotation(glm::vec3(0.0f, -1.0f, 0.0f), dir);
}

bool RayPlaneIntersection(const glm::vec3& origin, const glm::vec3& dir, float planeY, glm::vec3& outPoint)
{
    if (std::abs(dir.y) < 0.0001f)
        return false;
    float t = (planeY - origin.y) / dir.y;
    if (t < 0.0f)
        return false;
    outPoint = origin + dir * t;
    return true;
}

void ConfigureScenario23PathOptions(PathFollowerComponent& follower, int criteria)
{
    follower.pathfindingOptions.criteria = static_cast<PathfindingCriteria>(criteria);
    follower.pathfindingOptions.preferredTags =
        criteria == 2 ? std::vector<std::string>{"road"} : std::vector<std::string>{"walkable", "road"};
    follower.pathfindingOptions.tagWeightBonus = criteria == 2 ? 80.0f : 1.0f;
    follower.pathfindingOptions.altitudePenaltyWeight = criteria == 1 ? 14.0f : 5.0f;
}

float Scenario23NavHeight(float x, float z)
{
    float ridge = 4.0f * std::exp(-std::abs(x) * 0.075f);
    float roll = 1.2f * std::sin(z * 0.18f) + 0.8f * std::sin((x + z) * 0.11f);
    return 0.5f + (std::max)(0.0f, ridge + roll);
}

float Scenario23WaypointY(int criteria, float x, float z)
{
    return (criteria == 0 || criteria == 1 || criteria == 4) ? Scenario23NavHeight(x, z) : 0.5f;
}

struct Scenario21ShapeSpec
{
    const char* mesh;
    ShapeType shape;
    glm::vec3 visualScale;
    glm::vec3 boxSize;
    float radius;
    float height;
};

Scenario21ShapeSpec GetScenario21ShapeSpec(int shapeIndex, bool payload)
{
    switch (shapeIndex)
    {
        case 1:
            return {"sphereModel", ShapeType::Sphere,    payload ? glm::vec3(1.65f) : glm::vec3(0.85f), glm::vec3(1.0f),
                    0.5f,          payload ? 1.0f : 0.8f};
        case 2:
            return {"capsuleModel",
                    ShapeType::Capsule,
                    payload ? glm::vec3(1.35f, 1.9f, 1.35f) : glm::vec3(0.65f, 1.15f, 0.65f),
                    glm::vec3(1.0f),
                    0.5f,
                    payload ? 1.05f : 0.85f};
        default:
            return {"cubeModel",     ShapeType::Box,        payload ? glm::vec3(1.6f) : glm::vec3(0.75f, 0.95f, 0.75f),
                    glm::vec3(0.5f), payload ? 1.0f : 0.5f, payload ? 1.0f : 0.8f};
    }
}

void ApplyShapeSpec(RigidShapeComponent& shape, const Scenario21ShapeSpec& spec)
{
    shape.type = spec.shape;
    shape.size = spec.boxSize;
    shape.radius = spec.radius;
    shape.height = spec.height;
}

void StopScenario31AudioHandles(std::shared_ptr<ISound>& audio2D, std::shared_ptr<ISound>& audio3D, bool& play2D,
                                bool& play3D)
{
    if (audio2D)
    {
        audio2D->Stop();
        audio2D = nullptr;
    }
    if (audio3D)
    {
        audio3D->Stop();
        audio3D = nullptr;
    }
    play2D = false;
    play3D = false;
}

void StopAudioSourceComponent(AudioSourceComponent& audio)
{
    if (audio.sound)
    {
        audio.sound->Stop();
        audio.sound = nullptr;
    }
    audio.shouldPlay = false;
    audio.playOnAwake = false;
}

void ApplyScenario31AudioComponent(AudioSourceComponent& audio, bool play, bool is3D, float volume, float pitch,
                                   float minDistance, float maxDistance)
{
    audio.loop = true;
    audio.is3D = is3D;
    audio.volume = volume;
    audio.pitch = pitch;
    audio.playOnAwake = play;

    if (is3D)
    {
        audio.minDistance = minDistance;
        audio.maxDistance = maxDistance;
    }

    if (!play)
    {
        StopAudioSourceComponent(audio);
        return;
    }

    if (audio.sound && audio.sound->IsFinished())
        audio.sound = nullptr;

    if (!audio.sound)
    {
        audio.shouldPlay = true;
    }
    else
    {
        audio.shouldPlay = false;
    }

    if (audio.sound)
    {
        audio.sound->SetVolume(audio.volume);
        audio.sound->SetPitch(audio.pitch * audio.speed);
        if (is3D)
        {
            audio.sound->SetMinDistance(audio.minDistance);
            audio.sound->SetMaxDistance(audio.maxDistance);
        }
    }
}

void SetUITextByName(Scene& scene, const std::string& name, const std::string& text)
{
    auto view = scene.View<UITextComponent, InfoComponent>();
    for (auto entity : view)
    {
        auto& info = view.get<InfoComponent>(entity);
        if (info.name != name)
            continue;

        view.get<UITextComponent>(entity).text = text;
        return;
    }
}

void UpdateScenario29LocalizedUI(Scene& scene, LocalizationSystem& l10n)
{
    const int entityCount = static_cast<int>(scene.View<InfoComponent>().size());
    int rigidBodyCount = 0;
    auto rbView = scene.View<RigidBodyComponent>();
    for (auto entity : rbView)
    {
        if (rbView.get<RigidBodyComponent>(entity).body)
            ++rigidBodyCount;
    }

    SetUITextByName(scene, "L10nPanelTitle", l10n.Get("app.title"));
    SetUITextByName(scene, "L10nCurrentLanguageText",
                    l10n.GetFormat("l10n.preview.current_language", l10n.GetLanguage()));
    SetUITextByName(scene, "L10nScenarioLabelText", l10n.Get("menu.select_scenario"));
    SetUITextByName(scene, "L10nEntityCountText",
                    l10n.GetFormat("scenario.active_entities", std::to_string(entityCount)));
    SetUITextByName(scene, "L10nReloadText",
                    l10n.Get("l10n.preview.description") + "\n" +
                        l10n.GetFormat("scenario.active_rigid_bodies", std::to_string(rigidBodyCount)));
}

struct Scenario30SpawnCommand
{
    int id = 0;
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 color = glm::vec3(1.0f);
};

std::string BuildScenario30SpawnPayload(const Scenario30SpawnCommand& cmd)
{
    char buffer[192];
    std::snprintf(buffer, sizeof(buffer), "AXIS_S30_SPAWN|%d|%.3f|%.3f|%.3f|%.3f|%.3f|%.3f", cmd.id, cmd.position.x,
                  cmd.position.y, cmd.position.z, cmd.color.x, cmd.color.y, cmd.color.z);
    return std::string(buffer);
}

bool ParseScenario30SpawnPayload(const std::string& msg, Scenario30SpawnCommand& out)
{
    Scenario30SpawnCommand parsed;
    int matched =
        std::sscanf(msg.c_str(), "AXIS_S30_SPAWN|%d|%f|%f|%f|%f|%f|%f", &parsed.id, &parsed.position.x,
                    &parsed.position.y, &parsed.position.z, &parsed.color.x, &parsed.color.y, &parsed.color.z);
    if (matched != 7 || parsed.id <= 0)
        return false;

    out = parsed;
    return true;
}

entt::entity FindEntityByName(Scene& scene, const std::string& name)
{
    auto view = scene.View<InfoComponent>();
    for (auto entity : view)
    {
        if (view.get<InfoComponent>(entity).name == name)
            return entity;
    }
    return entt::null;
}

bool PointInsideNamedBox(Scene& scene, const char* name, const glm::vec3& point, const glm::vec3& padding)
{
    auto entity = FindEntityByName(scene, name);
    if (entity == entt::null || !scene.IsValid(entity))
        return false;

    auto* boxPos = scene.TryGetComponent<PositionComponent>(entity);
    auto* boxScale = scene.TryGetComponent<ScaleComponent>(entity);
    if (!boxPos || !boxScale)
        return false;

    const glm::vec3 halfExtents = boxScale->value + padding;
    const glm::vec3 delta = glm::abs(point - boxPos->value);
    return delta.x <= halfExtents.x && delta.y <= halfExtents.y && delta.z <= halfExtents.z;
}

entt::entity SpawnScenario30NetworkEntity(Scene& scene, ResourceManager& res, const Scenario30SpawnCommand& cmd)
{
    const std::string name = "S29ServerSpawn_" + std::to_string(cmd.id);
    if (auto existing = FindEntityByName(scene, name); existing != entt::null)
        return existing;

    auto entity = EntityBuilder(scene, res, "scenario")
                      .WithName(name)
                      .WithTransform(cmd.position, glm::vec3(0.0f, static_cast<float>((cmd.id * 37) % 360), 0.0f),
                                     glm::vec3(1.6f))
                      .WithPBRMesh("sphereModel", "deferred_lit", 0.05f, 0.35f, 1.0f)
                      .WithNetwork(static_cast<uint32_t>(cmd.id), 0, false)
                      .Build();

    if (auto* renderer = scene.TryGetComponent<MeshRendererComponent>(entity))
        renderer->color = glm::vec4(cmd.color, 1.0f);

    return entity;
}
}  // namespace

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
    EnableSystem("StreamingSystem", true);

    SetCursorMode(CursorMode::Normal);

#ifdef ENABLE_EDITOR
    EnableSystem("EditorSystem", false);

    auto* sysMgr = Resolve<SystemManager>();
    if (sysMgr)
    {
        auto* editorSys = dynamic_cast<EditorSystem*>(sysMgr->GetSystem("EditorSystem"));
        if (editorSys)
        {
            m_EditorImGuiLayer = &editorSys->GetImGuiLayer();
        }
    }
#endif

    // Register actions for Scenario 8 input handling
    ResetDefaultPlayerBindings();

    srand(static_cast<unsigned int>(time(nullptr)));

    // Load initial scenario (Scene 1)
    LoadScenario(1);
}

void SampleState::ResetDefaultPlayerBindings()
{
    auto* io = Resolve<IOHandler>();
    if (!io)
        return;

    auto& input = io->GetInputManager();
    input.FlushBindings();
    input.BindAction("PlayerForward", InputType::Key, (int)Key::W);
    input.BindAction("PlayerBackward", InputType::Key, (int)Key::S);
    input.BindAction("PlayerLeft", InputType::Key, (int)Key::A);
    input.BindAction("PlayerRight", InputType::Key, (int)Key::D);
    input.BindAction("PlayerAction", InputType::MouseButton, (int)Mouse::Left);
    input.BindAction("PlayerJump", InputType::Key, (int)Key::Space);
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

    // Pathfinding update (Scenario 23)
    if (m_CurrentScenario == 23 && m_NavFollower != entt::null && !m_NavWaypoints.empty())
    {
        auto& navSystem = GetSystem<NavigationSystem>();
        auto* follower = GetScene().TryGetComponent<PathFollowerComponent>(m_NavFollower);
        if (follower)
        {
            follower->moveSpeed = m_S23FollowerSpeed;
            follower->lockXPitch = m_S23LockXPitch;
            follower->lockYYaw = m_S23LockYYaw;
            follower->lockZRoll = m_S23LockZRoll;
            follower->lockMoveX = m_S23LockMoveX;
            follower->lockMoveY = m_S23LockMoveY;
            follower->lockMoveZ = m_S23LockMoveZ;
            ConfigureScenario23PathOptions(*follower, m_S23PathfindingCriteria);

            if (m_S23LastPathfindingCriteria != m_S23PathfindingCriteria || m_S23RepathRequested)
            {
                navSystem.StopMoving(GetScene(), m_NavFollower);
                navSystem.MoveTo(GetScene(), m_NavFollower, follower->targetPosition);
                m_S23LastPathfindingCriteria = m_S23PathfindingCriteria;
                m_S23RepathRequested = false;
            }

            if (!follower->pathPending && !follower->isMoving)
            {
                m_CurrentWaypointIndex = (m_CurrentWaypointIndex + 1) % m_NavWaypoints.size();
                navSystem.MoveTo(GetScene(), m_NavFollower, m_NavWaypoints[m_CurrentWaypointIndex]);
            }
        }
    }

    // Apply continuous wind force (Scenario 21)
    if (m_CurrentScenario == 21 && !m_S21ChainEntities.empty() && m_S21WindForce > 0.0f)
    {
        auto& scene = GetScene();
        auto lastEntity = m_S21ChainEntities.back();
        auto* rb = scene.TryGetComponent<RigidBodyComponent>(lastEntity);
        if (rb && rb->body)
        {
            rb->body->Activate(true);
            static float windAccumTime = 0.0f;
            windAccumTime += dt;
            float windForceX = m_S21WindForce * sin(windAccumTime * 3.0f);
            rb->body->ApplyCentralForce(glm::vec3(windForceX, 0.0f, 0.0f));
        }
    }

    // Post-processing parameters update (Scenario 10)
    if (m_CurrentScenario == 10)
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

                auto view = GetScene().View<PostProcessComponent, InfoComponent>();
                for (auto entity : view)
                {
                    auto& info = view.get<InfoComponent>(entity);
                    if (info.name != "Scenario10PostProcess")
                        continue;

                    auto& pp = view.get<PostProcessComponent>(entity);
                    pp.effects.clear();
                    if (m_PPVignetteEnabled)
                        pp.effects.push_back({"vignette", 80, 0, 0, 0, 0, true, false});
                    if (m_PPGlitchEnabled)
                        pp.effects.push_back({"glitch", 90, 0, 0, 0, 0, true, false});
                    if (m_PPFilmGrainEnabled)
                        pp.effects.push_back({"film_grain", 100, 0, 0, 0, 0, true, false});
                    if (m_PPGrayEnabled)
                        pp.effects.push_back({"grayscale", 120, 0, 0, 0, 0, true, false});
                    if (m_PPDitherEnabled)
                        pp.effects.push_back({"dither", 130, 0, 0, 0, 0, true, false});
                    if (m_PPPartialEffectEnabled)
                    {
                        const int w = (std::max)(0, m_PPPartialW);
                        const int h = (std::max)(0, m_PPPartialH);
                        if (w > 0 && h > 0)
                        {
                            std::string shader = "grayscale";
                            switch (m_PPPartialEffectType)
                            {
                                case 0:
                                    shader = "grayscale";
                                    break;
                                case 1:
                                    shader = "dither";
                                    break;
                                case 2:
                                    shader = "glitch";
                                    break;
                                case 3:
                                    shader = "vignette";
                                    break;
                                default:
                                    break;
                            }
                            pp.effects.push_back({shader, 140, m_PPPartialX, m_PPPartialY, w, h, true, false});
                        }
                    }
                    break;
                }
            }
        }
    }

    if (m_CurrentScenario == 24)
    {
        auto* io = Resolve<IOHandler>();
        auto* phys = Resolve<IPhysicsWorld>();
        if (io && phys)
        {
            const auto& mouse = io->GetMouse();
            const bool mouseDown = GetAction("PlayerAction");
            const bool mousePressed = GetActionDown("PlayerAction");
#ifdef ENABLE_EDITOR
            const bool mouseCaptured = ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse;
#else
            const bool mouseCaptured = false;
#endif
            auto& scene = GetScene();

            if (m_S24Dragging && (!mouseDown || mouseCaptured || !scene.IsValid(m_S24GrabbedEntity)))
            {
                m_S24Dragging = false;
                m_S24GrabbedEntity = entt::null;
            }

            if (!m_S24Dragging && mousePressed && !mouseCaptured)
            {
                PhysicsQueryService query;
                auto hit = query.RaycastFromScreen(glm::vec2(mouse.GetLastX(), mouse.GetLastY()));
                if (hit.hasHit && scene.IsValid(hit.entity))
                {
                    auto* rb = scene.TryGetComponent<RigidBodyComponent>(hit.entity);
                    auto* info = scene.TryGetComponent<InfoComponent>(hit.entity);
                    if (rb && !rb->isStatic && (!info || info->name != "S26_CharacterController"))
                    {
                        auto* pos = scene.TryGetComponent<PositionComponent>(hit.entity);
                        if (pos)
                        {
                            m_S24GrabbedEntity = hit.entity;
                            m_S24Dragging = true;
                            m_S24GrabOffset = pos->value - hit.hitPoint;
                            m_S24GrabPlaneY = (glm::max)(2.0f, pos->value.y + 2.0f);
                            m_ShowDebugLines = false;
                        }
                    }
                }
            }

            if (m_S24Dragging && scene.IsValid(m_S24GrabbedEntity))
            {
                auto camEntity = scene.GetActiveCamera();
                if (camEntity != entt::null && scene.HasAllComponents<CameraComponent, PositionComponent>(camEntity))
                {
                    auto& cam = scene.GetComponent<CameraComponent>(camEntity);
                    glm::vec2 viewportSize(static_cast<float>(io->GetMonitorManager().GetWidth()),
                                           static_cast<float>(io->GetMonitorManager().GetHeight()));
                    Ray ray = RaycastUtils::CalculateRay(glm::vec2(mouse.GetLastX(), mouse.GetLastY()), viewportSize,
                                                         cam.viewMatrix, cam.projectionMatrix);
                    glm::vec3 targetPoint;
                    if (RayPlaneIntersection(ray.origin, ray.direction, m_S24GrabPlaneY, targetPoint))
                    {
                        auto* pos = scene.TryGetComponent<PositionComponent>(m_S24GrabbedEntity);
                        auto* rot = scene.TryGetComponent<RotationComponent>(m_S24GrabbedEntity);
                        auto* world = scene.TryGetComponent<WorldTransformComponent>(m_S24GrabbedEntity);
                        auto* rb = scene.TryGetComponent<RigidBodyComponent>(m_S24GrabbedEntity);
                        if (pos)
                        {
                            pos->value = targetPoint + m_S24GrabOffset;
                            if (rot)
                                rot->value = glm::quat(glm::vec3(0.0f));
                            if (world)
                                world->isDirty = true;
                            if (rb && rb->body)
                            {
                                rb->body->Activate(true);
                                rb->body->SetWorldTransform(pos->value,
                                                            rot ? rot->value : glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
                                rb->body->SetLinearVelocity(glm::vec3(0.0f));
                                rb->body->SetAngularVelocity(glm::vec3(0.0f));
                            }
                        }
                    }
                }
            }
        }
    }

    if (m_CurrentScenario == 16)
    {
        if (m_S16CardEntity != entt::null && GetScene().IsValid(m_S16CardEntity))
        {
            if (auto* transform = GetScene().TryGetComponent<UITransformComponent>(m_S16CardEntity))
                transform->rotation = m_S16RotateCard;
            if (m_S16TextureEntity != entt::null && GetScene().IsValid(m_S16TextureEntity))
            {
                if (auto* info = GetScene().TryGetComponent<InfoComponent>(m_S16TextureEntity))
                    info->isActive = m_S16ShowTexture;
                if (auto* transform = GetScene().TryGetComponent<UITransformComponent>(m_S16TextureEntity))
                {
                    transform->flipX = m_S16FlipTextureX;
                    transform->flipY = m_S16FlipTextureY;
                }
            }
        }
        if (m_S16RootPanel != entt::null && GetScene().IsValid(m_S16RootPanel))
        {
            auto& scene = GetScene();
            if (auto* renderer = scene.TryGetComponent<UIRendererComponent>(m_S16RootPanel))
            {
                renderer->color.a = m_S16PanelAlpha;
            }
            if (auto* flex = scene.TryGetComponent<UIFlexLayoutComponent>(m_S16RootPanel))
            {
                flex->direction = (m_S16LayoutMode == 2) ? FlexDirection::Column : FlexDirection::Row;
                flex->spacing = (m_S16LayoutMode == 1) ? 18.0f : 10.0f;
            }
        }
    }

    if (m_CurrentScenario == 14 && m_S14SpawnPhysicsBalls)
    {
        m_S14SpawnTimer += dt;
        if (m_S14SpawnTimer >= 0.35f)
        {
            m_S14SpawnTimer = 0.0f;
            auto& scene = GetScene();
            auto& res = Get<ResourceManager>();
            auto view = scene.View<RigidBodyComponent, InfoComponent>();
            int spawnCount = 0;
            entt::entity oldestEntity = entt::null;
            int minBallId = 2147483647;
            for (auto entity : view)
            {
                auto& info = view.get<InfoComponent>(entity);
                if (info.name.rfind("S14Ball_", 0) == 0)
                {
                    spawnCount++;
                    try
                    {
                        int id = std::stoi(info.name.substr(8));
                        if (id < minBallId)
                        {
                            minBallId = id;
                            oldestEntity = entity;
                        }
                    }
                    catch (...)
                    {
                        if (oldestEntity == entt::null || (uint32_t)entity < (uint32_t)oldestEntity)
                        {
                            oldestEntity = entity;
                        }
                    }
                }
            }

            if (spawnCount >= 40 && oldestEntity != entt::null)
            {
                scene.Destroy(oldestEntity);
            }

            float rx = (static_cast<float>(rand() % 100) / 100.0f - 0.5f) * (m_S14TerrainWidth * 0.7f);
            float rz = (static_cast<float>(rand() % 100) / 100.0f - 0.5f) * (m_S14TerrainLength * 0.7f);
            float ry = m_S14TerrainHeight + 15.0f + (static_cast<float>(rand() % 100) / 10.0f);

            static int ballId = 0;
            std::string ballName = "S14Ball_" + std::to_string(++ballId);
            bool isSphere = (rand() % 2 == 0);
            auto ball = EntityBuilder(scene, res, "scenario")
                            .WithName(ballName)
                            .WithTransform(glm::vec3(rx, ry, rz), glm::vec3(rand() % 360, rand() % 360, rand() % 360),
                                           glm::vec3(1.5f))
                            .WithPBRMesh(isSphere ? "sphereModel" : "cubeModel", "deferred_lit", 0.05f, 0.4f, 1.0f)
                            .Build();

            if (auto* renderer = scene.TryGetComponent<MeshRendererComponent>(ball))
            {
                float rColor = 0.3f + static_cast<float>(rand() % 70) / 100.0f;
                float gColor = 0.3f + static_cast<float>(rand() % 70) / 100.0f;
                float bColor = 0.3f + static_cast<float>(rand() % 70) / 100.0f;
                renderer->color = glm::vec4(rColor, gColor, bColor, 1.0f);
            }

            auto& shape = scene.AddComponent<RigidShapeComponent>(ball);
            shape.type = isSphere ? ShapeType::Sphere : ShapeType::Box;
            shape.size = glm::vec3(1.0f);
            shape.radius = 1.0f;
            shape.height = 2.0f;
            shape.restitution = 0.65f;
            shape.friction = 0.35f;

            auto& rb = scene.AddComponent<RigidBodyComponent>(ball);
            rb.mass = 1.0f;
            rb.isStatic = false;
            rb.linearDamping = 0.05f;
            rb.angularDamping = 0.05f;
        }
    }

    if (m_CurrentScenario == 3)
    {
        m_S3MotionTime += dt;
        auto& scene = GetScene();

        auto dirView = scene.View<DirectionalLightComponent, InfoComponent>();
        for (auto entity : dirView)
        {
            auto& info = dirView.get<InfoComponent>(entity);
            if (info.name != "DirLight")
                continue;
            auto& light = dirView.get<DirectionalLightComponent>(entity);
            light.color = m_S3DirectionalColor;
            light.intensity = m_S3DirectionalIntensity;
            if (m_S3LightMotionMode != 0)
            {
                float a = m_S3MotionTime * m_S3DirectionalSweepSpeed;
                light.direction = glm::normalize(glm::vec3(cos(a) * 0.65f, -1.0f, sin(a) * 0.65f));
                if (auto* rot = scene.TryGetComponent<RotationComponent>(entity))
                    rot->value = RotationFromNegativeY(light.direction);
                if (auto* world = scene.TryGetComponent<WorldTransformComponent>(entity))
                    world->isDirty = true;
            }
            if (auto* mat = scene.TryGetComponent<MaterialComponent>(entity))
            {
                mat->desc.emission = m_S3DirectionalColor * (m_S3DirectionalIntensity * 3.0f);
                mat->gpu.dirty = true;
            }
            break;
        }

        auto view = GetScene().View<PositionComponent, PointLightComponent, InfoComponent>();
        for (auto entity : view)
        {
            auto& info = view.get<InfoComponent>(entity);
            if (info.name != "PointLight")
                continue;

            auto& pos = view.get<PositionComponent>(entity);
            float a = m_S3MotionTime * m_S3PointMotionSpeed;
            if (m_S3LightMotionMode == 1)
                pos.value =
                    glm::vec3(cos(a) * m_S3PointOrbitRadius, m_S3PointMotionHeight, sin(a) * m_S3PointOrbitRadius);
            else if (m_S3LightMotionMode == 2)
                pos.value.y = m_S3PointMotionHeight + sin(a) * 8.0f;
            else if (m_S3LightMotionMode == 3)
                pos.value = glm::vec3(sin(a) * m_S3PointOrbitRadius, m_S3PointMotionHeight + sin(a * 1.7f) * 6.0f,
                                      sin(a * 2.0f) * m_S3PointOrbitRadius * 0.5f);

            if (auto* world = GetScene().TryGetComponent<WorldTransformComponent>(entity))
                world->isDirty = true;

            auto& light = view.get<PointLightComponent>(entity);
            light.color = m_S3PointColor;
            light.intensity = m_S3PointIntensity;
            if (auto* mat = GetScene().TryGetComponent<MaterialComponent>(entity))
            {
                mat->desc.emission = m_S3PointColor * (m_S3PointIntensity * 2.0f);
                mat->gpu.dirty = true;
            }
            break;
        }

        auto spotView = scene.View<PositionComponent, RotationComponent, SpotLightComponent, InfoComponent>();
        for (auto entity : spotView)
        {
            auto& info = spotView.get<InfoComponent>(entity);
            if (info.name != "SpotLight")
                continue;
            auto& light = spotView.get<SpotLightComponent>(entity);
            light.color = m_S3SpotColor;
            light.intensity = m_S3SpotIntensity;
            light.cutOff = glm::cos(glm::radians(22.5f));
            light.outerCutOff = glm::cos(glm::radians(32.5f));
            light.linear = 0.045f;
            light.quadratic = 0.0075f;

            auto& pos = spotView.get<PositionComponent>(entity);
            auto& rot = spotView.get<RotationComponent>(entity);
            float a = m_S3MotionTime * m_S3SpotMotionSpeed + 1.5708f;
            if (m_S3LightMotionMode == 1)
                pos.value = glm::vec3(cos(a) * m_S3SpotOrbitRadius, m_S3SpotMotionHeight, sin(a) * m_S3SpotOrbitRadius);
            else if (m_S3LightMotionMode == 2)
                pos.value.y = m_S3SpotMotionHeight + sin(a) * 10.0f;
            else if (m_S3LightMotionMode == 3)
                pos.value = glm::vec3(sin(a * 1.3f) * m_S3SpotOrbitRadius, m_S3SpotMotionHeight + sin(a) * 5.0f,
                                      sin(a * 2.0f) * m_S3SpotOrbitRadius * 0.55f);

            glm::vec3 target = glm::vec3(0.0f, 1.0f, 0.0f);
            light.direction = glm::normalize(target - pos.value);
            rot.value = RotationFromNegativeY(light.direction);
            if (auto* world = scene.TryGetComponent<WorldTransformComponent>(entity))
                world->isDirty = true;
            if (auto* mat = scene.TryGetComponent<MaterialComponent>(entity))
            {
                mat->desc.emission = m_S3SpotColor * (m_S3SpotIntensity * 1.25f);
                mat->gpu.dirty = true;
            }
            break;
        }
    }

    if (m_CurrentScenario == 2)
    {
        auto& scene = GetScene();
        auto dirView = scene.View<DirectionalLightComponent>();
        for (auto entity : dirView)
        {
            auto& light = dirView.get<DirectionalLightComponent>(entity);
            light.color = m_S2DirectionalColor;
            light.intensity = m_S2DirectionalIntensity;
        }
        auto pointView = scene.View<PointLightComponent>();
        for (auto entity : pointView)
        {
            auto& light = pointView.get<PointLightComponent>(entity);
            light.color = m_S2PointColor;
            light.intensity = m_S2PointIntensity;
            if (auto* mat = scene.TryGetComponent<MaterialComponent>(entity))
            {
                mat->desc.emission = m_S2PointColor * (m_S2PointIntensity * 4.0f);
                mat->gpu.dirty = true;
            }
        }
        auto spotView = scene.View<SpotLightComponent>();
        for (auto entity : spotView)
        {
            auto& light = spotView.get<SpotLightComponent>(entity);
            light.color = m_S2SpotColor;
            light.intensity = m_S2SpotIntensity;
            if (auto* mat = scene.TryGetComponent<MaterialComponent>(entity))
            {
                mat->desc.emission = m_S2SpotColor * (m_S2SpotIntensity * 3.0f);
                mat->gpu.dirty = true;
            }
        }

        glm::vec3 averageColor = (m_S2DirectionalColor + m_S2PointColor + m_S2SpotColor) / 3.0f;
        float visibleLight = glm::clamp(
            (m_S2DirectionalIntensity / 0.2f + m_S2PointIntensity / 10.0f + m_S2SpotIntensity / 12.0f) / 3.0f, 0.0f,
            1.0f);
        visibleLight = 0.18f + visibleLight * 0.82f;
        auto renderView = scene.View<MeshRendererComponent, InfoComponent>();
        for (auto entity : renderView)
        {
            auto& info = renderView.get<InfoComponent>(entity);
            if (info.name == "Floor" || info.name == "Cylinder")
            {
                renderView.get<MeshRendererComponent>(entity).color = glm::vec4(averageColor * visibleLight, 1.0f);
            }
        }
    }

    if (m_CurrentScenario == 13)
    {
        auto& scene = GetScene();
        auto view = scene.View<DirectionalLightComponent, InfoComponent>();
        for (auto entity : view)
        {
            auto& info = view.get<InfoComponent>(entity);
            if (info.name != "DecalDirLight")
                continue;
            auto& light = view.get<DirectionalLightComponent>(entity);
            light.color = m_S13LightColor;
            light.intensity = m_S13LightIntensity;
            light.isCastShadow = m_S13LightingMode == 2;
            break;
        }

        if (m_S13PointLightEntity != entt::null && scene.IsValid(m_S13PointLightEntity))
        {
            if (auto* light = scene.TryGetComponent<PointLightComponent>(m_S13PointLightEntity))
            {
                light->active = m_S13UsePointLight;
                light->color = m_S13PointLightColor;
                light->intensity = m_S13PointLightIntensity;
                light->radius = m_S13PointLightRadius;
                light->isCastShadow = m_S13LightingMode == 2;
            }
        }

        if (m_S13PointLightMarkerEntity != entt::null && scene.IsValid(m_S13PointLightMarkerEntity))
        {
            if (auto* info = scene.TryGetComponent<InfoComponent>(m_S13PointLightMarkerEntity))
                info->isActive = m_S13UsePointLight;
            if (auto* renderer = scene.TryGetComponent<MeshRendererComponent>(m_S13PointLightMarkerEntity))
                renderer->color = glm::vec4(m_S13PointLightColor, 1.0f);
            if (auto* mat = scene.TryGetComponent<MaterialComponent>(m_S13PointLightMarkerEntity))
            {
                mat->desc.emission = m_S13PointLightColor * (m_S13PointLightIntensity * 0.8f);
                mat->gpu.dirty = true;
            }
        }

        if (m_S13ShadowCasterEntity != entt::null && scene.IsValid(m_S13ShadowCasterEntity))
        {
            if (auto* info = scene.TryGetComponent<InfoComponent>(m_S13ShadowCasterEntity))
                info->isActive = m_S13ShowShadowCaster;
            if (auto* renderer = scene.TryGetComponent<MeshRendererComponent>(m_S13ShadowCasterEntity))
                renderer->castShadow = m_S13ShowShadowCaster;
        }

        auto decalView = scene.View<DecalComponent>();
        for (auto entity : decalView) decalView.get<DecalComponent>(entity).lightingMode = m_S13LightingMode;
    }

    if (m_CurrentScenario == 28)
    {
        auto* io = Resolve<IOHandler>();
        if (io)
        {
            auto& input = io->GetInputManager();
            auto view =
                GetScene().View<MeshRendererComponent, ScaleComponent, WorldTransformComponent, InfoComponent>();
            for (auto entity : view)
            {
                auto& info = view.get<InfoComponent>(entity);
                if (info.name.rfind("InputPad_", 0) != 0)
                    continue;

                const std::string action = info.name.substr(9);
                const bool pressed = input.GetAction(action);
                auto& renderer = view.get<MeshRendererComponent>(entity);
                auto& scale = view.get<ScaleComponent>(entity);
                renderer.color = pressed ? glm::vec4(0.0f, 0.85f, 0.25f, 1.0f) : glm::vec4(0.18f, 0.2f, 0.24f, 1.0f);
                scale.value.y = pressed ? 0.35f : 0.15f;
                view.get<WorldTransformComponent>(entity).isDirty = true;
            }
        }
    }

    if (m_CurrentScenario == 29)
    {
        auto& l10n = GetSystem<LocalizationSystem>();
        UpdateScenario29LocalizedUI(GetScene(), l10n);
    }

    // Scenario 30 Network Update
    if (m_CurrentScenario == 30)
    {
        auto* sysMgr = Resolve<SystemManager>();
        if (sysMgr)
        {
            auto* netSystem = dynamic_cast<NetworkSystem*>(sysMgr->GetSystem("NetworkSystem"));
            if (netSystem && netSystem->IsRunning())
            {
                if (!netSystem->IsServer())
                {
                    m_S30SendTimer += dt;
                    if (m_S30SendTimer >= 1.5f)
                    {
                        m_S30SendTimer = 0.0f;
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

    // Scenario 31 Audio Orbit Update
    if (m_CurrentScenario == 31)
    {
        m_S31OrbitAngle += m_S31Speed * dt;
        glm::vec3 soundPos(sin(m_S31OrbitAngle) * 15.0f, 3.0f, cos(m_S31OrbitAngle) * 15.0f);

        auto view = GetScene().View<PositionComponent, InfoComponent>();
        for (auto entity : view)
        {
            auto& info = view.get<InfoComponent>(entity);
            if (info.name == kScenario31Audio3DName)
            {
                view.get<PositionComponent>(entity).value = soundPos;
                if (auto* world = GetScene().TryGetComponent<WorldTransformComponent>(entity))
                    world->isDirty = true;
                break;
            }
        }

        ApplyScenario31AudioSettings();
        if (auto* audio3D = FindScenario31AudioSource(kScenario31Audio3DName); audio3D && audio3D->sound)
            audio3D->sound->SetPosition(soundPos);
    }

    if (m_CurrentScenario == 6 && m_S6AnimateObjects)
    {
        m_S31OrbitAngle += dt * 0.8f;
        auto view = GetScene().View<PositionComponent, WorldTransformComponent, InfoComponent>();
        for (auto entity : view)
        {
            auto& info = view.get<InfoComponent>(entity);
            if (info.name == "OpaqueMover")
            {
                auto& pos = view.get<PositionComponent>(entity);
                pos.value.x = sin(m_S31OrbitAngle) * 7.5f;
                view.get<WorldTransformComponent>(entity).isDirty = true;
            }
        }
    }

    if (m_CurrentScenario == 4)
    {
        auto& scene = GetScene();
        m_S4AnimTime += m_S4AnimateCasters ? dt : 0.0f;

        const auto applyTransform = [&](entt::entity entity, const glm::vec3& pos, const glm::vec3& euler,
                                        const glm::vec3& scale) {
            if (entity == entt::null || !scene.IsValid(entity))
                return;
            if (auto* p = scene.TryGetComponent<PositionComponent>(entity))
                p->value = pos;
            if (auto* r = scene.TryGetComponent<RotationComponent>(entity))
                r->value = glm::quat(glm::radians(euler));
            if (auto* s = scene.TryGetComponent<ScaleComponent>(entity))
                s->value = scale;
            if (auto* w = scene.TryGetComponent<WorldTransformComponent>(entity))
                w->isDirty = true;
        };

        float spin = m_S4AnimateCasters ? std::sin(m_S4AnimTime * 1.4f) * 28.0f : 0.0f;
        applyTransform(m_S4ReceiverEntity, glm::vec3(0.0f), glm::vec3(0.0f),
                       glm::vec3(m_S4ReceiverSize, 1.0f, m_S4ReceiverSize));
        applyTransform(m_S4DeferredCubeEntity, glm::vec3(-16.0f * m_S4CasterSpread, m_S4CasterHeight, -1.0f),
                       glm::vec3(0.0f, 25.0f + spin, 0.0f), glm::vec3(4.0f, 8.0f, 4.0f) * m_S4CasterScale);
        applyTransform(m_S4DeferredSphereEntity, glm::vec3(-9.0f * m_S4CasterSpread, m_S4CasterHeight, 4.0f),
                       glm::vec3(0.0f, -spin, 0.0f), glm::vec3(4.0f) * m_S4CasterScale);
        applyTransform(m_S4ForwardCubeEntity, glm::vec3(10.0f * m_S4CasterSpread, m_S4CasterHeight + 1.0f, -1.0f),
                       glm::vec3(0.0f, -18.0f - spin, 0.0f), glm::vec3(3.5f, 7.0f, 3.5f) * m_S4CasterScale);
        applyTransform(m_S4ForwardSphereEntity, glm::vec3(18.0f * m_S4CasterSpread, m_S4CasterHeight - 1.0f, 4.0f),
                       glm::vec3(0.0f, 35.0f + spin, 0.0f), glm::vec3(2.8f, 5.2f, 2.8f) * m_S4CasterScale);

        if (m_S4LightEntity != entt::null && scene.IsValid(m_S4LightEntity))
        {
            glm::vec3 direction = glm::normalize(
                glm::vec3(std::cos(glm::radians(m_S4LightYaw)) * std::cos(glm::radians(m_S4LightPitch)),
                          std::sin(glm::radians(m_S4LightPitch)),
                          std::sin(glm::radians(m_S4LightYaw)) * std::cos(glm::radians(m_S4LightPitch))));
            if (auto* light = scene.TryGetComponent<DirectionalLightComponent>(m_S4LightEntity))
            {
                light->direction = direction;
                light->intensity = m_S4LightIntensity;
                light->isCastShadow = true;
            }
            if (auto* rot = scene.TryGetComponent<RotationComponent>(m_S4LightEntity))
                rot->value = glm::quat(glm::radians(glm::vec3(m_S4LightPitch, m_S4LightYaw, 0.0f)));
            if (auto* world = scene.TryGetComponent<WorldTransformComponent>(m_S4LightEntity))
                world->isDirty = true;
        }
    }

    if (m_CurrentScenario == 22)
    {
        auto& scene = GetScene();
        if (m_S22AutoSweep)
        {
            m_S22SweepTime += dt;
            m_S22Yaw = -90.0f + std::sin(m_S22SweepTime * 0.75f) * 34.0f;
        }

        if (m_S22ShotFlash > 0.0f)
            m_S22ShotFlash = (std::max)(0.0f, m_S22ShotFlash - dt);

#ifdef ENABLE_EDITOR
        const bool mouseCaptured = ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse;
#else
        const bool mouseCaptured = false;
#endif
        const bool clickFire = GetActionDown("PlayerAction") && !mouseCaptured;
        const bool shouldFire = m_S22FireRequested || clickFire;
        m_S22FireRequested = false;

        if (m_S22EmitterEntity != entt::null && scene.IsValid(m_S22EmitterEntity))
        {
            if (auto* pos = scene.TryGetComponent<PositionComponent>(m_S22EmitterEntity))
                m_S22RayOrigin = pos->value;
        }

        glm::vec3 dir = RaycastUtils::AngleToDirection(m_S22Yaw, m_S22Pitch);
        PhysicsQueryService query;
        RayHit hit = query.Raycast(m_S22RayOrigin, dir, m_S22Distance);
        m_S22RayHit = hit.hasHit;
        m_S22RayEnd = hit.hasHit ? hit.hitPoint : (m_S22RayOrigin + dir * m_S22Distance);
        m_S22LastHitEntity = hit.hasHit ? hit.entity : entt::null;

        if (hit.hasHit && scene.IsValid(hit.entity))
        {
            if (auto* info = scene.TryGetComponent<InfoComponent>(hit.entity))
                m_S22LastHit = info->name + " @ " + std::to_string(static_cast<int>(hit.distance)) + "m";
            else
                m_S22LastHit = "Entity " + std::to_string(static_cast<uint32_t>(hit.entity));
        }
        else
        {
            m_S22LastHit = "No hit";
        }

        if (shouldFire)
        {
            m_S22ShotFlash = 0.18f;
            if (hit.hasHit && scene.IsValid(hit.entity))
            {
                auto* info = scene.TryGetComponent<InfoComponent>(hit.entity);
                const bool isTarget = info && info->name.rfind("S22Target_", 0) == 0;
                if (isTarget)
                {
                    ++m_S22HitCount;
                    if (auto* renderer = scene.TryGetComponent<MeshRendererComponent>(hit.entity))
                        renderer->color = glm::vec4(1.0f, 0.12f, 0.06f, 1.0f);
                    if (auto* mat = scene.TryGetComponent<MaterialComponent>(hit.entity))
                    {
                        mat->desc.emission = glm::vec3(1.2f, 0.08f, 0.04f);
                        mat->gpu.dirty = true;
                    }
                    if (auto* rb = scene.TryGetComponent<RigidBodyComponent>(hit.entity); rb && rb->body)
                    {
                        rb->body->Activate(true);
                        rb->body->ApplyCentralImpulse(dir * m_S22ImpactImpulse);
                    }
                }
            }
        }
    }

    if (m_CurrentScenario == 12)
    {
        auto view = GetScene().View<LightProbeComponent>();
        for (auto entity : view)
        {
            auto& probe = view.get<LightProbeComponent>(entity);
            probe.intensity = m_S12ProbeIntensity;
            probe.radius = m_S12ProbeRadius;
        }
    }
}

void SampleState::OnRender()
{
    OnRenderDebug();
#ifdef ENABLE_EDITOR
    if (m_EditorImGuiLayer && !m_EditorSystemEnabled)
    {
        m_EditorImGuiLayer->BeginFrame();
        DrawGUI();
        m_EditorImGuiLayer->EndFrame();
    }
#endif
}

void SampleState::OnRenderDebug()
{
    if (m_ShowDebugLines)
    {
        // Draw obstacles in RED (Scenario 23)
        if (m_CurrentScenario == 23)
        {
            auto physics_ptr = Resolve<IPhysicsWorld>();
            if (physics_ptr)
            {
                auto& scene = GetScene();
                auto view = scene.View<PositionComponent, ScaleComponent, InfoComponent>();
                for (auto entity : view)
                {
                    auto& info = view.get<InfoComponent>(entity);
                    if (info.tag == "obstacle")
                    {
                        auto& pos = view.get<PositionComponent>(entity);
                        auto& scale = view.get<ScaleComponent>(entity);
                        glm::vec3 h = scale.value * 0.5f;
                        glm::vec3 p = pos.value;
                        glm::vec3 v[8] = {p + glm::vec3(-h.x, -h.y, -h.z), p + glm::vec3(h.x, -h.y, -h.z),
                                          p + glm::vec3(h.x, h.y, -h.z),   p + glm::vec3(-h.x, h.y, -h.z),
                                          p + glm::vec3(-h.x, -h.y, h.z),  p + glm::vec3(h.x, -h.y, h.z),
                                          p + glm::vec3(h.x, h.y, h.z),    p + glm::vec3(-h.x, h.y, h.z)};
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

                if (m_NavFollower != entt::null)
                {
                    if (auto* follower = scene.TryGetComponent<PathFollowerComponent>(m_NavFollower))
                    {
                        const auto& plannedPath =
                            !follower->debugPlannedPath.empty() ? follower->debugPlannedPath : follower->currentPath;
                        for (size_t i = 1; i < plannedPath.size(); ++i)
                        {
                            physics_ptr->DrawLine(plannedPath[i - 1] + glm::vec3(0.0f, 0.55f, 0.0f),
                                                  plannedPath[i] + glm::vec3(0.0f, 0.55f, 0.0f),
                                                  glm::vec3(0.0f, 1.0f, 0.15f));
                        }
                        for (size_t i = 1; i < follower->debugTraveledPath.size(); ++i)
                        {
                            physics_ptr->DrawLine(follower->debugTraveledPath[i - 1] + glm::vec3(0.0f, 0.85f, 0.0f),
                                                  follower->debugTraveledPath[i] + glm::vec3(0.0f, 0.85f, 0.0f),
                                                  glm::vec3(1.0f, 0.8f, 0.0f));
                        }
                    }
                }
            }
        }

        if (m_CurrentScenario == 22)
        {
            if (auto physics_ptr = Resolve<IPhysicsWorld>())
            {
                glm::vec3 color = m_S22ShotFlash > 0.0f
                                      ? glm::vec3(1.0f, 0.95f, 0.05f)
                                      : (m_S22RayHit ? glm::vec3(1.0f, 0.08f, 0.04f) : glm::vec3(0.1f, 0.7f, 1.0f));
                physics_ptr->DrawLine(m_S22RayOrigin, m_S22RayEnd, color);
                physics_ptr->DrawLine(m_S22RayEnd + glm::vec3(-0.25f, 0.0f, 0.0f),
                                      m_S22RayEnd + glm::vec3(0.25f, 0.0f, 0.0f), color);
                physics_ptr->DrawLine(m_S22RayEnd + glm::vec3(0.0f, -0.25f, 0.0f),
                                      m_S22RayEnd + glm::vec3(0.0f, 0.25f, 0.0f), color);
                physics_ptr->DrawLine(m_S22RayEnd + glm::vec3(0.0f, 0.0f, -0.25f),
                                      m_S22RayEnd + glm::vec3(0.0f, 0.0f, 0.25f), color);
            }
        }

        // Render physics debug lines (skip for scenario 7 - particle stress test)
        if (m_CurrentScenario != 9)
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
    StopScenario31Audio();

    // Clean up scene entities
    auto* sceneMgr = Resolve<SceneManager>();
    if (sceneMgr)
    {
        sceneMgr->UnloadSceneByName(kScenario25DynamicSceneName);
        sceneMgr->UnloadSceneByName(kScenario25BaseSceneName);
    }

    // No need to shutdown the shared ImGuiLayer
}

void SampleState::LoadScenario(int index)
{
    if (m_CurrentScenario == 31)
        StopScenario31Audio();

    auto* sceneMgr = Resolve<SceneManager>();
    if (sceneMgr)
    {
        sceneMgr->UnloadSceneByName(kScenario25DynamicSceneName);
        sceneMgr->UnloadSceneByName(kScenario25BaseSceneName);
    }

    m_CurrentScenario = index;
    m_ShowDebugLines = false;
    m_NavFollower = entt::null;
    m_S24GrabbedEntity = entt::null;
    m_S24Dragging = false;
    m_S3MotionTime = 0.0f;
    m_S3DirLightEntity = entt::null;
    m_S3PointLightEntity = entt::null;
    m_S3SpotLightEntity = entt::null;
    m_S13DirLightEntity = entt::null;
    m_S13PointLightEntity = entt::null;
    m_S13PointLightMarkerEntity = entt::null;
    m_S13ShadowCasterEntity = entt::null;
    m_S4ReceiverEntity = entt::null;
    m_S4DeferredCubeEntity = entt::null;
    m_S4DeferredSphereEntity = entt::null;
    m_S4ForwardCubeEntity = entt::null;
    m_S4ForwardSphereEntity = entt::null;
    m_S4LightEntity = entt::null;
    m_S4AnimTime = 0.0f;
    m_S16CardEntity = entt::null;
    m_S16TextureEntity = entt::null;
    m_S16RootPanel = entt::null;
    m_S22EmitterEntity = entt::null;
    m_S22LastHitEntity = entt::null;
    m_S22Targets.clear();
    m_S22RayHit = false;
    m_S22ShotFlash = 0.0f;
    m_S11ReflectionSpheres.clear();
    m_S11ReflectionProbes.clear();
    m_S11PlanarMirror = entt::null;
    m_S11ActiveCase = 0;
    m_S26ControllerEntity = entt::null;
    m_S23LastPathfindingCriteria = -1;
    m_S23RepathRequested = true;
    if (index != 6 && index != 12)
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
            pipeline.ResetTemporalHistory();
        }
    }
    m_PPVignetteEnabled = false;
    m_PPGlitchEnabled = false;
    m_PPFilmGrainEnabled = false;
    m_PPGrayEnabled = false;
    m_PPDitherEnabled = false;
    m_PPPartialEffectEnabled = false;
    m_PPPartialEffectType = 2;
    m_PPPartialX = 0;
    m_PPPartialY = 0;
    m_PPPartialW = 0;
    m_PPPartialH = 0;
    m_S21ChainEntities.clear();
    m_S25RandomEntities.clear();
    if (auto* physics = Resolve<IPhysicsWorld>())
        physics->SetSolverIterations(10);
    if (auto* collisionMatrix = Resolve<CollisionMatrix>())
        collisionMatrix->Reset();
    if (auto* navSystem = Resolve<NavigationSystem>())
        navSystem->SetShowDebug(m_ShowDebugLines);
    if (auto* renderState = Resolve<IRenderStateService>())
    {
        renderState->SetOcclusionCulling(false);
        renderState->SetDebugNoTexture(false);
        renderState->SetWireframe(false);
    }

    // Reset camera for all scenarios
    SetupCamera();

    switch (index)
    {
        case 1:
            LoadScene1();
            break;
        case 2:
            LoadScene2();
            break;
        case 3:
            LoadScene3();
            break;
        case 4:
            LoadScene4();
            break;
        case 5:
            LoadScene5();
            break;
        case 6:
            LoadScene6();
            break;
        case 7:
            LoadScene7();
            break;
        case 8:
            LoadScene8();
            break;
        case 9:
            LoadScene9();
            break;
        case 10:
            LoadScene10();
            break;
        case 11:
            LoadScene11();
            break;
        case 12:
            LoadScene12();
            break;
        case 13:
            LoadScene13();
            break;
        case 14:
            LoadScene14();
            break;
        case 15:
            LoadScene15();
            break;
        case 16:
            LoadScene16();
            break;
        case 17:
            LoadScene17();
            break;
        case 18:
            LoadScene18();
            break;
        case 19:
            LoadScene19();
            break;
        case 20:
            LoadScene20();
            break;
        case 21:
            LoadScene21();
            break;
        case 22:
            LoadScene22();
            break;
        case 23:
            LoadScene23();
            break;
        case 24:
            LoadScene24();
            break;
        case 25:
            LoadScene25();
            break;
        case 26:
            LoadScene26();
            break;
        case 27:
            LoadScene27();
            break;
        case 28:
            LoadScene28();
            break;
        case 29:
            LoadScene29();
            break;
        case 30:
            LoadScene30();
            break;
        case 31:
            LoadScene31();
            break;
        default:
            break;
    }

    // Force immediate transform update so worldMatrix is correct before next render
    auto& transformSys = GetSystem<TransformSystem>();
    transformSys.m_IsLinearTransformsDirty = true;
    transformSys.Update(GetScene(), 0.0f);
    GetSystem<CameraSystem>().Update(GetScene(), 0.0f);
    if (auto* navSystem = Resolve<NavigationSystem>())
        navSystem->SetShowDebug(m_ShowDebugLines);
}

void SampleState::StopScenario31Audio()
{
    StopScenario31AudioHandles(m_S31Audio2D, m_S31Audio3D, m_S31Play2D, m_S31Play3D);
    if (auto* audio2D = FindScenario31AudioSource(kScenario31Audio2DName))
        StopAudioSourceComponent(*audio2D);
    if (auto* audio3D = FindScenario31AudioSource(kScenario31Audio3DName))
        StopAudioSourceComponent(*audio3D);
}

AudioSourceComponent* SampleState::FindScenario31AudioSource(const char* name)
{
    if (!name)
        return nullptr;

    auto& scene = GetScene();
    auto entity = scene.FindByName(name);
    if (entity == entt::null || !scene.IsValid(entity))
        return nullptr;
    return scene.TryGetComponent<AudioSourceComponent>(entity);
}

void SampleState::ApplyScenario31AudioSettings()
{
    m_S31Volume2D = glm::clamp(m_S31Volume2D, 0.0f, kScenario31MaxVolume);
    m_S31Volume3D = glm::clamp(m_S31Volume3D, 0.0f, kScenario31MaxVolume);

    if (auto* audio2D = FindScenario31AudioSource(kScenario31Audio2DName))
    {
        ApplyScenario31AudioComponent(*audio2D, m_S31Play2D, false, m_S31Volume2D, m_S31Pitch, m_S31MinDistance,
                                      m_S31MaxDistance);
    }

    if (auto* audio3D = FindScenario31AudioSource(kScenario31Audio3DName))
    {
        ApplyScenario31AudioComponent(*audio3D, m_S31Play3D, true, m_S31Volume3D, m_S31Pitch, m_S31MinDistance,
                                      m_S31MaxDistance);
    }
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
    auto& script = scene.AddComponent<ScriptComponent>(camera);
    script.className = "DefaultCameraController";
    script.InstantiateScript = []() { return std::make_unique<DefaultCameraController>(); };
    script.DestroyScript = [](ScriptComponent* nsc) {
        nsc->instance.reset();
        nsc->scriptableInstance = nullptr;
        nsc->inputScriptableInstance = nullptr;
    };
}

void SampleState::DrawGUI()
{
#ifdef ENABLE_EDITOR
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(380, 780), ImGuiCond_Always);

    ImGui::Begin("AxisEngine Benchmark Dashboard", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    ImGui::TextColored(ImVec4(0.0f, 0.8f, 1.0f, 1.0f), "Performance Metrics");
    ImGui::Separator();
    PerfStats perf = QueryPerfStats();
    ImGui::Text("FPS: %.1f", m_CurrentFps);
    ImGui::Text("Frame Time: %.2f ms", 1000.0f / (m_CurrentFps > 0.0f ? m_CurrentFps : 60.0f));
    ImGui::Text("Active Entities: %d", (int)GetScene().View<InfoComponent>().size());
    ImGui::Text("CPU: %.0f%%  GPU: %.0f%%", perf.cpu, perf.gpu);
    ImGui::Text("RAM: %.0f%%  VRAM: %.0f%%", perf.ram, perf.vram);

    // Physics rigid bodies count
    int rbCount = 0;
    auto rbView = GetScene().View<RigidBodyComponent>();
    for (auto entity : rbView)
    {
        if (rbView.get<RigidBodyComponent>(entity).body)
            rbCount++;
    }
    ImGui::Text("Active Rigid Bodies: %d", rbCount);

    // Active particle count
    int pCount = 0;
    auto pView = GetScene().View<ParticleEmitterComponent>();
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

    auto AddScenarioButton = [this](int index, const char* label, const char* desc) {
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

    AddScenarioButton(1, "Scenario 1: Render Entity Load",
                      "Dense deferred-unlit renderer load with optional unique tint to break batches");
    AddScenarioButton(2, "Scenario 2: Lighting Load Test",
                      "1 smooth capsule, plane, and 999 dynamic lighting entities");
    AddScenarioButton(3, "Scenario 3: Shadow Mapping Test", "1,000 cubes, plane, and 3 light types casting shadows");
    AddScenarioButton(4, "Scenario 4: Shadow Receiver",
                      "Interactive deferred/forward casters on a deferred shadow receiver");
    AddScenarioButton(5, "Scenario 5: PBR Material Matrix",
                      "Compare metallic/roughness material response under fixed lights");
    AddScenarioButton(6, "Scenario 6: Transparent Objects",
                      "Test transparent forward pass sorting, opacity, and lighting");
    AddScenarioButton(7, "Scenario 7: Render Order", "Show ordering with transparent panels and opaque markers");
    AddScenarioButton(8, "Scenario 8: Layer Filter", "Toggle camera culling mask bits to show/hide render layers");
    AddScenarioButton(9, "Scenario 9: LOD Selection", "Show distance based model swaps using LODComponent");
    AddScenarioButton(10, "Scenario 10: Post-Process & Tonemap",
                      "Test HDR Bloom parameters, Exposure, Gamma, and Tonemapping");
    AddScenarioButton(11, "Scenario 11: SSR & Env Probes",
                      "Test environment probes and planar reflections reflection mapping");
    AddScenarioButton(12, "Scenario 12: Light Probe",
                      "Ambient spherical-harmonic light probes with editable intensity/radius");
    AddScenarioButton(13, "Scenario 13: Decal Stress Test", "PBR Decals: project materials dynamically on surfaces");
    AddScenarioButton(14, "Scenario 14: Terrain Creation Showcase",
                      "Generate heightmaps and splatmaps procedurally and drop physics bodies");
    AddScenarioButton(15, "Scenario 15: Particle Stress Test",
                      "50 emitters with high-density colorful particle vortex");
    AddScenarioButton(16, "Scenario 16: UI & Responsive Showcase",
                      "Merged: UI Showcase transform/rotate/flip and Responsive layout tests");
    AddScenarioButton(17, "Scenario 17: Interactive UI", "Show UI onHover, onClick, onHold and button callbacks");
    AddScenarioButton(18, "Scenario 18: Video Mesh & UI Render",
                      "Test Video Decoder applying frames to texture and UI using sample.mp4");
    AddScenarioButton(19, "Scenario 19: Skeletal Anim & Blend",
                      "Test animations blend/crossfade using defeated.fbx / spin.fbx");
    AddScenarioButton(20, "Scenario 20: Physics Stress Test", "Dynamic Bullet rigid bodies falling and colliding");
    AddScenarioButton(21, "Scenario 21: Physics Constraint Chain",
                      "Pendulum test with Bullet joints and selectable primitive colliders");
    AddScenarioButton(22, "Scenario 22: Hitray & Hitscan",
                      "Raycast preview plus instant hitscan impulse against targets");
    AddScenarioButton(23, "Scenario 23: Navigation Test", "Dynamic NavMesh generation and differentiated path methods");
    AddScenarioButton(24, "Scenario 24: Scriptable Stability Test", "100 entities with 6 complex C++ script behaviors");
    AddScenarioButton(25, "Scenario 25: Scene Save & Load", "Test scene saving/loading using sample.axs");
    AddScenarioButton(26, "Scenario 26: Character Controller + Collision Zone",
                      "CharacterControllerComponent, trigger counters, fixed constraint, and obstacle course");
    AddScenarioButton(27, "Scenario 27: DataNote YAML Test",
                      "Test serialization of entity count and size using data.axs");
    AddScenarioButton(28, "Scenario 28: Input Binding Save/Load", "Test load/save key bindings from binding.axs");
    AddScenarioButton(29, "Scenario 29: Localization (l10n)", "Test load/apply localizations from vi.axs/en.axs");
    AddScenarioButton(30, "Scenario 30: Network Messaging", "Test local network client/server packet transmission");
    AddScenarioButton(31, "Scenario 31: Audio 2D & 3D Test", "Test irrKlang audio play/orbit using sample.wav");

    ImGui::EndChild();

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.0f, 0.8f, 1.0f, 1.0f), "Scenario Parameters / Options");
    ImGui::Separator();

    if (m_CurrentScenario == 1)
    {
        ImGui::SliderInt("Entities count", &m_S1EntityCount, 1000, 50000);
        ImGui::Combo("Mesh Type", &m_S1MeshType, "Sphere\0Cube\0Cylinder\0Capsule\0");
        ImGui::Checkbox("Unique Tint (break batches)", &m_S1UniqueTint);
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Click 'Reload Scenario' to apply changes.");
    }
    else if (m_CurrentScenario == 3)
    {
        ImGui::Combo("Light Motion", &m_S3LightMotionMode, "Static\0Circle\0Vertical Bob\0Figure Eight\0");
        ImGui::SliderFloat("Directional Sweep Speed", &m_S3DirectionalSweepSpeed, 0.0f, 2.0f);
        ImGui::SliderFloat("Point Motion Speed", &m_S3PointMotionSpeed, 0.1f, 5.0f);
        ImGui::SliderFloat("Point Orbit Radius", &m_S3PointOrbitRadius, 4.0f, 35.0f);
        ImGui::SliderFloat("Point Base Height", &m_S3PointMotionHeight, 3.0f, 30.0f);
        ImGui::SliderFloat("Spot Motion Speed", &m_S3SpotMotionSpeed, 0.1f, 5.0f);
        ImGui::SliderFloat("Spot Orbit Radius", &m_S3SpotOrbitRadius, 4.0f, 40.0f);
        ImGui::SliderFloat("Spot Base Height", &m_S3SpotMotionHeight, 5.0f, 40.0f);
        ImGui::Separator();
        ImGui::ColorEdit3("Directional Color", &m_S3DirectionalColor.x);
        ImGui::SliderFloat("Directional Brightness", &m_S3DirectionalIntensity, 0.0f, 5.0f);
        ImGui::ColorEdit3("Point Color", &m_S3PointColor.x);
        ImGui::SliderFloat("Point Brightness", &m_S3PointIntensity, 0.0f, 20.0f);
        ImGui::ColorEdit3("Spot Color", &m_S3SpotColor.x);
        ImGui::SliderFloat("Spot Brightness", &m_S3SpotIntensity, 0.0f, 25.0f);
    }
    else if (m_CurrentScenario == 2)
    {
        ImGui::ColorEdit3("Directional Color", &m_S2DirectionalColor.x);
        ImGui::SliderFloat("Directional Intensity", &m_S2DirectionalIntensity, 0.0f, 0.2f);
        ImGui::ColorEdit3("Point Color", &m_S2PointColor.x);
        ImGui::SliderFloat("Point Intensity", &m_S2PointIntensity, 0.0f, 10.0f);
        ImGui::ColorEdit3("Spot Color", &m_S2SpotColor.x);
        ImGui::SliderFloat("Spot Intensity", &m_S2SpotIntensity, 0.0f, 12.0f);
    }
    else if (m_CurrentScenario == 20)
    {
        ImGui::SliderInt("Rigid Bodies count", &m_S20EntityCount, 100, 3000);
        ImGui::Combo("Shape Type", &m_S20ShapeType, "Box\0Sphere\0Capsule\0");
        ImGui::SliderFloat("Mass", &m_S20Mass, 0.1f, 10.0f);
        ImGui::SliderFloat("Bounciness", &m_S20Restitution, 0.0f, 1.0f);
        ImGui::SliderFloat("Friction", &m_S20Friction, 0.0f, 1.0f);
        ImGui::DragFloat3("Gravity", &m_S20Gravity.x, 0.1f, -50.0f, 50.0f);
        ImGui::SliderFloat("Spawn Height", &m_S20SpawnHeight, 3.0f, 30.0f);
        ImGui::SliderFloat("Grid Spacing", &m_S20GridSpacing, 1.0f, 5.0f);
        ImGui::SliderFloat("Linear Damping", &m_S20LinearDamping, 0.0f, 1.0f);
        ImGui::SliderFloat("Angular Damping", &m_S20AngularDamping, 0.0f, 1.0f);
        ImGui::SliderFloat("Initial Impulse", &m_S20InitialImpulse, 0.0f, 30.0f);
        ImGui::Text("Mass changes collision and impulse response; free-fall speed stays gravity-driven.");
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Click 'Reload Scenario' to apply changes.");
        ImGui::Spacing();
        if (ImGui::Checkbox("Show Physics Debug Lines", &m_ShowDebugLines))
        {
            auto& navSystem = GetSystem<NavigationSystem>();
            navSystem.SetShowDebug(m_ShowDebugLines);
        }
    }
    else if (m_CurrentScenario == 23)
    {
        ImGui::SliderInt("Obstacles count", &m_S23ObstacleCount, 0, 50);
        ImGui::SliderFloat("Obstacle Size", &m_S23ObstacleSize, 1.0f, 10.0f);
        ImGui::SliderFloat("Follower Speed", &m_S23FollowerSpeed, 1.0f, 25.0f);
        if (ImGui::Combo("Pathfinding Method", &m_S23PathfindingCriteria,
                         "Shortest (XYZ)\0Smoothest (XYZ turns)\0Stay on road (XY)\0Straight line\0High ground\0"))
        {
            m_S23RepathRequested = true;
        }
        ImGui::Text("Green: planned path. Yellow: actual traveled path.");
        ImGui::Spacing();
        ImGui::Text("Axis / Rotation Locks:");
        ImGui::Checkbox("Lock X (Pitch)", &m_S23LockXPitch);
        ImGui::Checkbox("Lock Y (Yaw)", &m_S23LockYYaw);
        ImGui::Checkbox("Lock Z (Roll)", &m_S23LockZRoll);
        ImGui::Separator();
        ImGui::Text("Movement Axis Locks:");
        ImGui::Checkbox("Lock Move X", &m_S23LockMoveX);
        ImGui::Checkbox("Lock Move Y", &m_S23LockMoveY);
        ImGui::Checkbox("Lock Move Z", &m_S23LockMoveZ);
        ImGui::Spacing();
        ImGui::DragFloat3("New Waypoint", &m_S23NewWaypoint.x, 0.5f, -24.0f, 24.0f);
        if (ImGui::Button("Add Waypoint", ImVec2(170, 24)))
        {
            m_NavWaypoints.push_back(
                glm::vec3(m_S23NewWaypoint.x,
                          Scenario23WaypointY(m_S23PathfindingCriteria, m_S23NewWaypoint.x, m_S23NewWaypoint.z),
                          m_S23NewWaypoint.z));
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
            m_NavWaypoints = {glm::vec3(-22.0f, Scenario23WaypointY(m_S23PathfindingCriteria, -22.0f, 22.0f), 22.0f),
                              glm::vec3(22.0f, Scenario23WaypointY(m_S23PathfindingCriteria, 22.0f, -22.0f), -22.0f)};
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
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f),
                           "Method replans immediately; reload only regenerates obstacles.");
        ImGui::Spacing();
        if (ImGui::Checkbox("Show Debug Lines (Path/NavMesh)", &m_ShowDebugLines))
        {
            auto& navSystem = GetSystem<NavigationSystem>();
            navSystem.SetShowDebug(m_ShowDebugLines);
        }
    }
    else if (m_CurrentScenario == 24)
    {
        ImGui::SliderInt("Scripted Entity Count", &m_S24EntityCount, 1, 1000);
        ImGui::Combo("Mesh Type", &m_S24MeshType, "Cube\0Sphere\0Capsule\0Cylinder\0");
        ImGui::Combo("Shader Mode", &m_S24ShaderMode, "Deferred Unlit\0Deferred Lit\0Forward Lit\0");
        ImGui::SliderFloat("Base Radius", &m_S24BaseRadius, 2.0f, 40.0f);
        ImGui::SliderFloat("Radius Step", &m_S24RadiusStep, 0.0f, 10.0f);
        ImGui::SliderFloat("Vertical Step", &m_S24VerticalStep, 0.0f, 8.0f);
        ImGui::SliderFloat("Entity Scale", &m_S24EntityScale, 0.2f, 4.0f);
        ImGui::Separator();
        ImGui::Text("Enabled Script Behaviors:");
        ImGui::Checkbox("Orbit", &m_S24EnableOrbit);
        ImGui::Checkbox("Pulse Scale", &m_S24EnablePulse);
        ImGui::Checkbox("Color Shift", &m_S24EnableColor);
        ImGui::Checkbox("Random Move", &m_S24EnableRandomMove);
        ImGui::Checkbox("Rotate", &m_S24EnableRotate);
        ImGui::Checkbox("Bounce", &m_S24EnableBounce);
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Click 'Reload Scenario' to apply changes.");
    }
    else if (m_CurrentScenario == 15)
    {
        ImGui::SliderInt("Emitters count", &m_S15EmitterCount, 1, 200);
        ImGui::SliderFloat("Spawn Rate", &m_S15SpawnRate, 10.0f, 1000.0f);
        ImGui::SliderFloat("Life Time (s)", &m_S15LifeTime, 0.5f, 5.0f);
        ImGui::SliderFloat("Start Size", &m_S15StartSize, 0.05f, 2.0f);
        ImGui::SliderFloat("End Size", &m_S15EndSize, 0.0f, 2.0f);
        ImGui::SliderFloat("Min Speed", &m_S15MinSpeed, 0.0f, 20.0f);
        ImGui::SliderFloat("Max Speed", &m_S15MaxSpeed, 0.0f, 30.0f);
        ImGui::SliderFloat("Vertical Speed", &m_S15VerticalSpeed, -10.0f, 20.0f);
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Click 'Reload Scenario' to apply changes.");
    }
    else if (m_CurrentScenario == 10)
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
        ImGui::Checkbox("Gray Effect", &m_PPGrayEnabled);
        ImGui::Checkbox("Dither Effect", &m_PPDitherEnabled);
        ImGui::Checkbox("Partial Rect Effect", &m_PPPartialEffectEnabled);
        ImGui::Combo("Partial Effect Type", &m_PPPartialEffectType, "Gray\0Dither\0Glitch\0Vignette\0");
        ImGui::Text("Partial XYWH is in screen pixels and only affects this region.");
        ImGui::DragInt("Partial X", &m_PPPartialX, 1.0f, 0, 4096);
        ImGui::DragInt("Partial Y", &m_PPPartialY, 1.0f, 0, 4096);
        ImGui::DragInt("Partial W", &m_PPPartialW, 1.0f, 0, 4096);
        ImGui::DragInt("Partial H", &m_PPPartialH, 1.0f, 0, 4096);
        if (ImGui::Button("Set Partial Demo Region", ImVec2(180, 24)))
        {
            m_PPPartialX = 160;
            m_PPPartialY = 120;
            m_PPPartialW = 640;
            m_PPPartialH = 360;
        }
    }
    else if (m_CurrentScenario == 21)
    {
        ImGui::SliderInt("Chain Length", &m_S21ChainLength, 2, 20);
        ImGui::SliderFloat("Wind Force (Oscillating)", &m_S21WindForce, 0.0f, 200.0f);
        ImGui::DragFloat3("Gravity", &m_S21Gravity.x, 0.1f, -50.0f, 50.0f);
        ImGui::SliderFloat("Link Mass", &m_S21LinkMass, 0.25f, 5.0f);
        ImGui::SliderFloat("Payload Mass", &m_S21PayloadMass, 0.5f, 20.0f);
        ImGui::SliderFloat("Link Damping", &m_S21LinkDamping, 0.0f, 1.0f);
        ImGui::SliderFloat("Anchor Height", &m_S21AnchorHeight, 10.0f, 40.0f);
        ImGui::SliderFloat("Kick Force", &m_S21KickForce, 0.0f, 100.0f);
        ImGui::Combo("Link Shape", &m_S21LinkShape, "Box\0Sphere\0Capsule\0");
        ImGui::Combo("Payload Shape", &m_S21PayloadShape, "Box\0Sphere\0Capsule\0");
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Click 'Reload Scenario' to apply changes.");
        ImGui::Spacing();
        if (ImGui::Checkbox("Show Constraints Debug", &m_ShowDebugLines))
        {
            auto& navSystem = GetSystem<NavigationSystem>();
            navSystem.SetShowDebug(m_ShowDebugLines);
        }
    }
    else if (m_CurrentScenario == 13)
    {
        ImGui::SliderInt("Decal Count", &m_S13DecalCount, 1, 100);
        ImGui::SliderFloat("Decal Size", &m_S13DecalSize, 0.5f, 10.0f);
        ImGui::SliderFloat("Decal Opacity", &m_S13Opacity, 0.0f, 1.0f);
        ImGui::ColorEdit3("Decal Color", &m_S13Color.x);
        ImGui::Combo("Decal Lighting", &m_S13LightingMode, "Unlit\0Lit\0Lit + Shadow\0");
        ImGui::Checkbox("Shadow Caster", &m_S13ShowShadowCaster);
        ImGui::Separator();
        ImGui::ColorEdit3("Light Color", &m_S13LightColor.x);
        ImGui::SliderFloat("Light Intensity", &m_S13LightIntensity, 0.0f, 8.0f);
        ImGui::Checkbox("Point Light Source", &m_S13UsePointLight);
        ImGui::ColorEdit3("Point Light Color", &m_S13PointLightColor.x);
        ImGui::SliderFloat("Point Light Intensity", &m_S13PointLightIntensity, 0.0f, 12.0f);
        ImGui::SliderFloat("Point Light Radius", &m_S13PointLightRadius, 2.0f, 80.0f);
        ImGui::Separator();
        ImGui::Checkbox("Rainbow Color Mode", &m_S13RainbowMode);
        auto decalView = GetScene().View<DecalComponent>();
        int decalIndex = 0;
        for (auto entity : decalView)
        {
            auto& decal = decalView.get<DecalComponent>(entity);
            decal.opacity = m_S13Opacity;
            decal.lightingMode = m_S13LightingMode;
            if (m_S13RainbowMode)
            {
                int decalDenom = m_S13DecalCount > 1 ? m_S13DecalCount : 1;
                float hue = static_cast<float>(decalIndex) / static_cast<float>(decalDenom);
                decal.tintColor =
                    glm::vec4(0.5f + 0.5f * sin(hue * 6.28318f), 0.5f + 0.5f * sin(hue * 6.28318f + 2.09439f),
                              0.5f + 0.5f * sin(hue * 6.28318f + 4.18879f), 1.0f);
            }
            else
            {
                decal.tintColor = glm::vec4(m_S13Color, 1.0f);
            }
            ++decalIndex;
        }
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Count/size require Reload Scenario.");
    }
    else if (m_CurrentScenario == 25)
    {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "Scene Save/Load (sample.axs)");
        ImGui::Text("Status: %s", m_S25Status.c_str());
        ImGui::Spacing();
        if (ImGui::Button("Add Random Entity", ImVec2(360, 24)))
        {
            auto& scene = GetScene();
            auto& res = Get<ResourceManager>();
            float x = static_cast<float>(rand() % 400 - 200) / 10.0f;
            float z = static_cast<float>(rand() % 400 - 200) / 10.0f;
            float size = 0.8f + static_cast<float>(rand() % 30) / 10.0f;
            auto entity =
                EntityBuilder(scene, res, "scenario")
                    .WithName("S25Random_" + std::to_string(++m_S25RandomEntityCount))
                    .WithTransform(glm::vec3(x, size * 0.5f, z), glm::vec3(0.0f, rand() % 360, 0.0f), glm::vec3(size))
                    .WithPBRMesh((m_S25RandomEntityCount % 2) ? "cubeModel" : "sphereModel", "deferred_lit", 0.1f, 0.5f,
                                 1.0f)
                    .Build();
            m_S25RandomEntities.push_back(entity);
            if (auto* renderer = scene.TryGetComponent<MeshRendererComponent>(entity))
            {
                renderer->color =
                    glm::vec4(static_cast<float>(rand() % 100) / 100.0f, static_cast<float>(rand() % 100) / 100.0f,
                              static_cast<float>(rand() % 100) / 100.0f, 1.0f);
            }
            auto& transformSys = GetSystem<TransformSystem>();
            transformSys.m_IsLinearTransformsDirty = true;
            transformSys.Update(scene, 0.0f);
            m_S25Status = "Added random entity. Save, reload scenario, then Load Scene to verify.";
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
                if (physicsSystem)
                    phys = &physicsSystem->GetPhysicsWorld();
            }
            if (sceneMgr)
            {
                sceneMgr->UnloadSceneByName(kScenario25DynamicSceneName);
                SetupCamera();
                m_S25RandomEntities.clear();
                m_S25RandomEntityCount = 0;
                const std::string path = std::filesystem::exists(SamplePath(kScenario25ScenePath))
                                             ? SamplePath(kScenario25ScenePath)
                                             : SamplePath(kScenario25SceneLegacyPath);
                SceneLoadResult res =
                    SceneSerializer::Deserialize(path, GetScene(), Get<ResourceManager>(), phys, audio);
                if (!res.entities.empty())
                {
                    for (auto entity : res.entities)
                    {
                        if (auto* info = GetScene().TryGetComponent<InfoComponent>(entity))
                        {
                            info->sceneName = "scenario";
                        }
                        sceneMgr->AddEntity(entity, "scenario");
                    }
                    m_S25RandomEntityCount = static_cast<int>(res.entities.size());
                    m_S25Status =
                        "Scene loaded successfully. Spawns: " + std::to_string(res.entities.size()) + " entities.";
                    auto& transformSys = GetSystem<TransformSystem>();
                    transformSys.m_IsLinearTransformsDirty = true;
                    transformSys.Update(GetScene(), 0.0f);

                    bool hasLight = false;
                    auto lightView = GetScene().View<DirectionalLightComponent>();
                    for (auto entity : lightView)
                    {
                        (void)entity;
                        hasLight = true;
                        break;
                    }
                    if (!hasLight)
                    {
                        auto& scene = GetScene();
                        auto& resMgr = Get<ResourceManager>();
                        auto light = EntityBuilder(scene, resMgr, "scenario")
                                         .WithName("LoadedSceneFallbackLight")
                                         .WithTransform(glm::vec3(20.0f, 40.0f, 20.0f), glm::vec3(-45.0f, -45.0f, 0.0f))
                                         .WithDirectionalLight(glm::normalize(glm::vec3(-0.7f, -1.0f, -0.7f)),
                                                               glm::vec3(1.0f), 1.5f)
                                         .Build();
                        sceneMgr->AddEntity(light, "scenario");
                    }
                }
                else
                {
                    m_S25Status = "Failed to deserialize scene from sample.axs.";
                }
            }
        }
        if (ImGui::Button("Save Current Scene (Serialize)", ImVec2(360, 24)))
        {
            bool ok = SceneSerializer::Serialize(kScenario25ScenePath, GetScene(), Get<ResourceManager>(), "scenario");
            if (ok)
            {
                m_S25Status = "Serialized current scene to sample.axs!";
            }
            else
            {
                m_S25Status = "Failed to serialize current scene.";
            }
        }
        if (ImGui::Button("Flush Scene (Keep Base Plane/Light)", ImVec2(360, 24)))
        {
            auto* sceneMgr = Resolve<SceneManager>();
            if (sceneMgr)
                sceneMgr->UnloadSceneByName(kScenario25DynamicSceneName);
            m_S25RandomEntities.clear();
            SetupCamera();
            m_S25RandomEntityCount = 0;
            m_S25Status = "Dynamic scene flushed. Floor/light kept.";
        }
    }
    else if (m_CurrentScenario == 28)
    {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "Input Binding Save/Load");
        ImGui::Text("Status: %s", m_S28Status.c_str());
        ImGui::Spacing();

        // --- Step 1: Persist / Load ---
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "Step 1 - Persist bindings:");
        if (ImGui::Button("Load binding.axs", ImVec2(175, 24)))
        {
            auto* io = Resolve<IOHandler>();
            if (io)
            {
                InputSerializer serializer;
                const std::string bindingPath = "sample/resource/binding/binding.axs";
                m_S28Status = serializer.Deserialize(bindingPath, io->GetInputManager())
                                  ? "Loaded sample/resource/binding/binding.axs."
                                  : "Failed to load binding.axs.";
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Save binding.axs", ImVec2(175, 24)))
        {
            auto* io = Resolve<IOHandler>();
            if (io)
            {
                InputSerializer serializer;
                const std::string bindingPath = "sample/resource/binding/binding.axs";
                m_S28Status = serializer.Serialize(bindingPath, io->GetInputManager())
                                  ? "Saved bindings to binding.axs."
                                  : "Failed to save.";
            }
        }
        if (ImGui::Button("Flush Bindings (Clear All)", ImVec2(360, 24)))
        {
            auto* io = Resolve<IOHandler>();
            if (io)
            {
                auto& input = io->GetInputManager();
                input.FlushBindings();
                m_S28Status = "Bindings cleared. Load a preset or save a new one.";
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
                input.BindAction("PlayerForward", InputType::Key, (int)Key::Up);
                input.BindAction("PlayerBackward", InputType::Key, (int)Key::Down);
                input.BindAction("PlayerLeft", InputType::Key, (int)Key::Left);
                input.BindAction("PlayerRight", InputType::Key, (int)Key::Right);
                input.BindAction("PlayerJump", InputType::Key, (int)Key::Space);
                input.BindAction("PlayerAction", InputType::MouseButton, (int)Mouse::Left);
                m_S28Status = "Arrow-key preset applied. Save to persist.";
            }
        }

        ImGui::Separator();
        // --- Step 3: Custom binding ---
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "Step 3 - Add custom binding:");
        static char actionBuf[64] = "PlayerCustom";
        ImGui::InputText("Action Name", actionBuf, sizeof(actionBuf));
        ImGui::InputInt("Key Code", &m_S28NewKey);
        if (ImGui::Button("Add Binding", ImVec2(360, 24)))
        {
            auto* io = Resolve<IOHandler>();
            if (io)
            {
                io->GetInputManager().BindAction(actionBuf, InputType::Key, m_S28NewKey);
                m_S28Status = std::string("Bound '") + actionBuf + "' to key " + std::to_string(m_S28NewKey) +
                              ". Save to persist.";
            }
        }

        ImGui::Separator();
        // --- Live state ---
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "Step 4 - Live action state:");
        auto* io = Resolve<IOHandler>();
        if (io)
        {
            auto& input = io->GetInputManager();
            const char* liveActions[] = {"PlayerForward", "PlayerBackward", "PlayerLeft",
                                         "PlayerRight",   "PlayerJump",     "PlayerAction"};
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
                    if (b.type == InputType::Key)
                        bindText += "Key:" + std::to_string(b.code) + " ";
                    else if (b.type == InputType::MouseButton)
                        bindText += "Mouse:" + std::to_string(b.code) + " ";
                    else if (b.type == InputType::GamepadButton)
                        bindText += "Gamepad:" + std::to_string(b.code) + " ";
                }
                ImGui::BulletText("%-20s %s", action.c_str(), bindText.c_str());
            }
        }
    }
    else if (m_CurrentScenario == 29)
    {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "Localization (l10n)");
        ImGui::Spacing();

        auto& l10n = GetSystem<LocalizationSystem>();
        ImGui::Text("Current Language: %s", l10n.GetLanguage().c_str());

        if (ImGui::Button("Switch to Vietnamese (vi)", ImVec2(360, 24)))
        {
            l10n.LoadLanguage("sample/resource/l10n/vi.axs", "vi");
            l10n.SetLanguage("vi");
            UpdateScenario29LocalizedUI(GetScene(), l10n);
        }
        if (ImGui::Button("Switch to English (en)", ImVec2(360, 24)))
        {
            l10n.LoadLanguage("sample/resource/l10n/en.axs", "en");
            l10n.SetLanguage("en");
            UpdateScenario29LocalizedUI(GetScene(), l10n);
        }

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.0f, 0.8f, 1.0f, 1.0f), "Localized Outputs:");
        ImGui::Text("App Title: %s", l10n.Get("app.title").c_str());
        ImGui::Text("Select Scenario Label: %s", l10n.Get("menu.select_scenario").c_str());

        int entCount = (int)GetScene().GetEntityCount();
        ImGui::Text("Localized format check: %s",
                    l10n.GetFormat("scenario.active_entities", std::to_string(entCount)).c_str());
    }
    else if (m_CurrentScenario == 27)
    {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "DataNote YAML Save/Load");
        ImGui::Text("Status: %s", m_S27Status.c_str());
        ImGui::Spacing();
        ImGui::SliderInt("Entity Count", &m_S27EntityCount, 1, 250);
        ImGui::SliderFloat("Entity Size", &m_S27EntitySize, 0.2f, 10.0f);

        if (ImGui::Button("Save data.axs", ImVec2(360, 24)))
        {
            std::unordered_map<std::string, DataNode> dataMap;
            DataNode dataNote;
            dataNote.attributes["EntityCount"] = std::to_string(m_S27EntityCount);
            dataNote.attributes["EntitySize"] = std::to_string(m_S27EntitySize);
            dataNote.value = "DataNote";
            dataMap["DataNote"] = dataNote;

            DataNodeSerializer serializer;
            if (serializer.Serialize(kScenario27DataPath, dataMap))
            {
                m_S27Status = "Serialized data to data.axs successfully.";
            }
            else
            {
                m_S27Status = "Failed to serialize data.";
            }
        }
        if (ImGui::Button("Load data.axs", ImVec2(360, 24)))
        {
            std::unordered_map<std::string, DataNode> dataMap;
            DataNodeSerializer serializer;
            const char* path =
                std::filesystem::exists(kScenario27DataPath) ? kScenario27DataPath : kScenario27DataLegacyPath;
            if (serializer.Deserialize(path, dataMap))
            {
                auto it = dataMap.find("DataNote");
                if (it == dataMap.end())
                    it = dataMap.find("PlayerStats");
                if (it != dataMap.end())
                {
                    auto& stats = it->second;
                    if (stats.attributes.find("EntityCount") != stats.attributes.end())
                        m_S27EntityCount = std::stoi(stats.attributes["EntityCount"]);
                    if (stats.attributes.find("EntitySize") != stats.attributes.end())
                        m_S27EntitySize = std::stof(stats.attributes["EntitySize"]);
                }
                m_S27Status = "Deserialized data successfully. Reload Scenario to apply entity count/size.";
            }
            else
            {
                m_S27Status = "Failed to deserialize data.axs.";
            }
        }
    }
    else if (m_CurrentScenario == 30)
    {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "Network Messaging (ENet)");
        ImGui::Text("Status: %s", m_S30Status.c_str());
        ImGui::Checkbox("Host as Server", &m_S30IsServer);
        ImGui::InputText("Host IP", m_S30Host, sizeof(m_S30Host));
        ImGui::InputInt("Port", &m_S30Port);
        m_S30Port = glm::clamp(m_S30Port, 1024, 65535);

        auto* sysMgr = Resolve<SystemManager>();
        if (sysMgr)
        {
            auto* netSystem = dynamic_cast<NetworkSystem*>(sysMgr->GetSystem("NetworkSystem"));
            if (netSystem)
            {
                ImGui::Text("Running: %s | Mode: %s | Connected peers: %zu", netSystem->IsRunning() ? "yes" : "no",
                            netSystem->IsServer() ? "server" : (netSystem->IsClient() ? "client" : "idle"),
                            netSystem->GetConnectedPeerCount());
                if (netSystem->IsClient())
                    ImGui::Text("Client peer state: %s", netSystem->GetClientPeerState());
                if (ImGui::Button("Start Connection", ImVec2(170, 24)))
                {
                    if (netSystem->IsRunning())
                        netSystem->Stop();
                    NetworkConfig config;
                    config.port = static_cast<uint16_t>(m_S30Port);
                    config.maxClients = 32;
                    config.useIPv6 = false;
                    m_S30Messages.clear();

                    if (m_S30IsServer)
                    {
                        config.host.clear();
                        netSystem->SetOnConnect([this](ENetPeer*) {
                            m_S30Status = "Server: client connected.";
                            m_S30Messages.push_back("Client connected.");
                        });
                        netSystem->SetOnDisconnect([this](ENetPeer*) {
                            m_S30Status = "Server: client disconnected.";
                            m_S30Messages.push_back("Client disconnected.");
                        });
                        netSystem->SetOnMessage(
                            [this, netSystem](ENetPeer* peer, const uint8_t* data, size_t size, uint8_t) {
                                std::string msg((const char*)data, size);
                                if (!msg.empty() && msg.back() == '\0')
                                    msg.pop_back();
                                m_S30Messages.push_back("Server recvd: " + msg);
                                std::string echo = "Echo: " + msg;
                                netSystem->SendPacket(peer, echo.c_str(), echo.size() + 1);
                            });
                        m_S30Status = netSystem->StartServer(config)
                                          ? "Server started on port " + std::to_string(m_S30Port) + "."
                                          : "Failed to start server.";
                    }
                    else
                    {
                        config.host = m_S30Host;
                        netSystem->SetOnConnect([this](ENetPeer*) {
                            m_S30Status =
                                "Client connected to " + std::string(m_S30Host) + ":" + std::to_string(m_S30Port) + ".";
                            m_S30Messages.push_back("Connected to server.");
                        });
                        netSystem->SetOnDisconnect([this](ENetPeer*) {
                            m_S30Status = "Client disconnected.";
                            m_S30Messages.push_back("Disconnected from server.");
                        });
                        netSystem->SetOnMessage([this](ENetPeer*, const uint8_t* data, size_t size, uint8_t) {
                            std::string msg((const char*)data, size);
                            if (!msg.empty() && msg.back() == '\0')
                                msg.pop_back();
                            Scenario30SpawnCommand spawnCmd;
                            if (ParseScenario30SpawnPayload(msg, spawnCmd))
                            {
                                SpawnScenario30NetworkEntity(GetScene(), Get<ResourceManager>(), spawnCmd);
                                m_S30Messages.push_back("Client spawned server entity #" + std::to_string(spawnCmd.id));
                                return;
                            }
                            m_S30Messages.push_back("Client recvd: " + msg);
                        });
                        if (netSystem->StartClient(config))
                        {
                            m_S30Status = "Client connecting to " + std::string(m_S30Host) + ":" +
                                          std::to_string(m_S30Port) + "...";
                        }
                        else
                        {
                            m_S30Status = "Failed to initiate client connection.";
                        }
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Stop Connection", ImVec2(170, 24)))
                {
                    netSystem->Stop();
                    m_S30Status = "Stopped.";
                }
            }
        }

        ImGui::Separator();
        ImGui::InputText("Message", m_S30MessageText, sizeof(m_S30MessageText));
        if (ImGui::Button("Send Message", ImVec2(360, 24)))
        {
            if (auto* sysMgr2 = Resolve<SystemManager>())
            {
                if (auto* netSystem = dynamic_cast<NetworkSystem*>(sysMgr2->GetSystem("NetworkSystem")))
                {
                    std::string msg = m_S30MessageText;
                    if (netSystem->IsServer())
                    {
                        if (netSystem->GetConnectedPeerCount() > 0)
                        {
                            netSystem->BroadcastPacket(msg.c_str(), msg.size() + 1);
                            m_S30Messages.push_back("Server sent: " + msg);
                        }
                        else
                        {
                            m_S30Messages.push_back("Server has no connected clients.");
                        }
                    }
                    else if (auto* peer = netSystem->GetServerPeer())
                    {
                        netSystem->SendPacket(peer, msg.c_str(), msg.size() + 1);
                        m_S30Messages.push_back("Client sent: " + msg);
                    }
                    else
                    {
                        m_S30Messages.push_back(std::string("No connected peer. State: ") +
                                                netSystem->GetClientPeerState());
                    }
                }
            }
        }
        if (ImGui::Button("Server Spawn Entity For Clients", ImVec2(360, 24)))
        {
            if (auto* sysMgr2 = Resolve<SystemManager>())
            {
                if (auto* netSystem = dynamic_cast<NetworkSystem*>(sysMgr2->GetSystem("NetworkSystem")))
                {
                    if (!netSystem->IsRunning() || !netSystem->IsServer())
                    {
                        m_S30Messages.push_back("Spawn requires a running server.");
                    }
                    else
                    {
                        Scenario30SpawnCommand cmd;
                        cmd.id = ++m_S30SpawnCounter;
                        cmd.position = glm::vec3(static_cast<float>(rand() % 260 - 130) / 10.0f,
                                                 2.0f + static_cast<float>(rand() % 40) / 10.0f,
                                                 static_cast<float>(rand() % 260 - 130) / 10.0f);
                        cmd.color = glm::vec3(0.35f + static_cast<float>(rand() % 65) / 100.0f,
                                              0.35f + static_cast<float>(rand() % 65) / 100.0f,
                                              0.35f + static_cast<float>(rand() % 65) / 100.0f);
                        SpawnScenario30NetworkEntity(GetScene(), Get<ResourceManager>(), cmd);

                        std::string payload = BuildScenario30SpawnPayload(cmd);
                        size_t peerCount = netSystem->GetConnectedPeerCount();
                        if (peerCount > 0)
                            netSystem->BroadcastPacket(payload.c_str(), payload.size() + 1);
                        m_S30Messages.push_back("Server spawned entity #" + std::to_string(cmd.id) +
                                                " and broadcast to " + std::to_string(peerCount) + " clients.");
                    }
                }
            }
        }
        ImGui::Separator();
        ImGui::Text("Network Logs:");
        ImGui::BeginChild("NetLogList", ImVec2(0, 100), true);
        for (auto& m : m_S30Messages) ImGui::TextUnformatted(m.c_str());
        ImGui::EndChild();
    }
    else if (m_CurrentScenario == 31)
    {
        std::string backendName = "Unknown";
        if (auto* configMgr = Resolve<ConfigManager>())
        {
            backendName = BackendRegistry::ToString(configMgr->GetConfig().audioBackend);
        }
        std::string title = backendName + " Audio Test";
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "%s", title.c_str());
        bool audioSettingsChanged = false;
        ImGui::SliderFloat("Orbit Speed", &m_S31Speed, 0.1f, 5.0f);
        audioSettingsChanged |= ImGui::SliderFloat("2D Volume", &m_S31Volume2D, 0.0f, kScenario31MaxVolume);
        audioSettingsChanged |= ImGui::SliderFloat("3D Volume", &m_S31Volume3D, 0.0f, kScenario31MaxVolume);
        audioSettingsChanged |= ImGui::SliderFloat("Playback Speed / Pitch", &m_S31Pitch, 0.25f, 3.0f);
        audioSettingsChanged |= ImGui::SliderFloat("3D Min Distance", &m_S31MinDistance, 0.1f, 20.0f);
        audioSettingsChanged |= ImGui::SliderFloat("3D Max Distance", &m_S31MaxDistance, 1.0f, 120.0f);

        audioSettingsChanged |= ImGui::Checkbox("Play 2D Sound (sample.wav)", &m_S31Play2D);
        audioSettingsChanged |= ImGui::Checkbox("Play 3D Orbiting Sound", &m_S31Play3D);

        if (audioSettingsChanged)
            ApplyScenario31AudioSettings();

        if (ImGui::Button("Stop All Sounds", ImVec2(360, 24)))
        {
            if (auto* audioService = Resolve<AudioService>())
                audioService->StopAll();
            m_S31Play2D = false;
            m_S31Play3D = false;
            m_S31Audio2D = nullptr;
            m_S31Audio3D = nullptr;
            ApplyScenario31AudioSettings();
        }
    }
    else if (m_CurrentScenario == 18)
    {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "Video Mesh & UI Playback");
        ImGui::Checkbox("Loop Video", &m_S18VideoPlaying);

        auto view = GetScene().View<VideoPlayerComponent>();
        for (auto entity : view)
        {
            auto& video = view.get<VideoPlayerComponent>(entity);
            if (video.decoder)
            {
                ImGui::PushID(static_cast<int>(static_cast<uint32_t>(entity)));
                ImGui::Text("Time: %.2f / %.2f s", video.decoder->GetCurrentTime(), video.decoder->GetDuration());
                ImGui::SliderFloat("Volume", &video.volume, 0.0f, 1.0f);
                ImGui::SliderFloat("Speed", &video.speed, 0.1f, 3.0f);

                if (ImGui::Button("Pause Video", ImVec2(170, 24)))
                    PauseVideo(video);
                ImGui::SameLine();
                if (ImGui::Button("Play Video", ImVec2(170, 24)))
                    PlayVideo(video);
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

        auto view = GetScene().View<AnimationComponent>();
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
    else if (m_CurrentScenario == 11)
    {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "SSR & Environment Probes");
        static const char* kCaseNames[] = {"UltraLow", "Low", "Mid", "High", "Extreme"};
        ImGui::Combo("Case", &m_S11ActiveCase, kCaseNames, IM_ARRAYSIZE(kCaseNames));
        ImGui::Text("Each case keeps its own probe resolution and fresnel response.");

        auto& scene = GetScene();
        if (m_S11ActiveCase >= 0 && m_S11ActiveCase < (int)m_S11ReflectionSpheres.size() &&
            m_S11ActiveCase < (int)m_S11ReflectionProbes.size())
        {
            auto sphereEntity = m_S11ReflectionSpheres[m_S11ActiveCase];
            auto probeEntity = m_S11ReflectionProbes[m_S11ActiveCase];

            if (auto* ref = scene.TryGetComponent<ReflectiveComponent>(sphereEntity))
            {
                ImGui::SliderFloat("Reflectivity", &ref->reflectivity, 0.0f, 1.0f);
                ImGui::SliderFloat("Fresnel Bias", &ref->fresnelBias, 0.0f, 0.5f);
                ImGui::SliderFloat("Fresnel Power", &ref->fresnelPower, 0.1f, 24.0f);
                m_S11Reflectivity = ref->reflectivity;
                m_S11FresnelBias = ref->fresnelBias;
                m_S11FresnelPower = ref->fresnelPower;
            }

            if (auto* probe = scene.TryGetComponent<ReflectionProbeComponent>(probeEntity))
            {
                ImGui::SliderInt("Probe Resolution", &probe->resolution, 64, 2048);
                if (ImGui::Button("Capture Selected Probe", ImVec2(360, 24)))
                    probe->isDirty = true;
                m_S11ProbeResolution = probe->resolution;
            }
        }

        if (m_S11PlanarMirror != entt::null && scene.IsValid(m_S11PlanarMirror))
        {
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.5f, 1.0f), "Planar Mirror");
            if (auto* ref = scene.TryGetComponent<ReflectiveComponent>(m_S11PlanarMirror))
            {
                ImGui::SliderFloat("Mirror Reflectivity", &ref->reflectivity, 0.0f, 1.0f);
                ImGui::SliderFloat("Mirror Fresnel Bias", &ref->fresnelBias, 0.0f, 0.5f);
                ImGui::SliderFloat("Mirror Fresnel Power", &ref->fresnelPower, 0.1f, 24.0f);
            }
            if (auto* planar = scene.TryGetComponent<PlanarReflectionComponent>(m_S11PlanarMirror))
            {
                ImGui::SliderInt("Mirror Resolution X", &planar->resolution, 256, 2048);
                ImGui::SliderInt("Mirror Resolution Y", &planar->resolution_y, 256, 2048);
                if (ImGui::Button("Mark Mirror Dirty", ImVec2(360, 24)))
                    planar->isDirty = true;
            }
        }

        if (ImGui::Button("Force Probe Re-Capture", ImVec2(360, 24)))
        {
            auto view = scene.View<ReflectionProbeComponent>();
            for (auto entity : view)
            {
                view.get<ReflectionProbeComponent>(entity).isDirty = true;
            }
        }
    }
    else if (m_CurrentScenario == 6)
    {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "Transparent Object Sorting");
        ImGui::SliderFloat("Glass Opacity", &m_S6GlassOpacity, 0.05f, 0.95f);
        ImGui::SliderFloat("Glass Roughness", &m_S6GlassRoughness, 0.0f, 1.0f);
        ImGui::Checkbox("Animate Objects", &m_S6AnimateObjects);

        auto view = GetScene().View<MaterialComponent, InfoComponent>();
        for (auto entity : view)
        {
            auto& info = view.get<InfoComponent>(entity);
            if (info.name.rfind("Glass_", 0) != 0)
                continue;
            auto& mat = view.get<MaterialComponent>(entity);
            mat.desc.opacity = m_S6GlassOpacity;
            mat.desc.pbr.roughness = m_S6GlassRoughness;
            mat.gpu.dirty = true;
        }
    }
    else if (m_CurrentScenario == 5)
    {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "PBR Material Matrix");
        ImGui::Text("Rows: metallic 0.0 -> 1.0");
        ImGui::Text("Columns: roughness 0.05 -> 0.95");
    }
    else if (m_CurrentScenario == 9)
    {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "LOD Selection");
        ImGui::Text("Near: sphere, mid: cube, far: capsule");
        ImGui::Text("Orange center = solid reference (no LOD).");
        ImGui::Text("Move camera forward/back to cross LOD thresholds.");
    }
    else if (m_CurrentScenario == 8)
    {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "Layer Filter");
        bool layer0 = (m_S8LayerMask & 0x1) != 0;
        bool layer1 = (m_S8LayerMask & 0x2) != 0;
        bool layer2 = (m_S8LayerMask & 0x4) != 0;
        if (ImGui::Checkbox("Layer 0: neutral/floor", &layer0))
            m_S8LayerMask = (m_S8LayerMask & ~0x1) | (layer0 ? 0x1 : 0);
        if (ImGui::Checkbox("Layer 1: red cubes", &layer1))
            m_S8LayerMask = (m_S8LayerMask & ~0x2) | (layer1 ? 0x2 : 0);
        if (ImGui::Checkbox("Layer 2: blue spheres", &layer2))
            m_S8LayerMask = (m_S8LayerMask & ~0x4) | (layer2 ? 0x4 : 0);
        if (auto camEntity = GetScene().GetActiveCamera(); camEntity != entt::null)
            GetScene().GetComponent<CameraComponent>(camEntity).cullingMask = static_cast<uint32_t>(m_S8LayerMask);
    }
    else if (m_CurrentScenario == 7)
    {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "Render Order");
        if (ImGui::Checkbox("Reverse all order demos", &m_S7ReverseOrder))
            ApplyScenario7RenderOrder();
        ImGui::Text("Panels and opaque cubes enable IgnoreDepth: lower order draws on top.");
        ImGui::Text("Normal: Red=1, Green=2, Blue=3 => Red top.");
        ImGui::Text("Reverse: Red=3, Green=2, Blue=1 => Blue top.");
        ImGui::Text("Depth and world position are ignored for these order demos.");
    }
    else if (m_CurrentScenario == 4)
    {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "Shadow Receiver");
        ImGui::SliderFloat("Receiver Size", &m_S4ReceiverSize, 30.0f, 140.0f);
        ImGui::SliderFloat("Caster Height", &m_S4CasterHeight, 2.0f, 18.0f);
        ImGui::SliderFloat("Caster Scale", &m_S4CasterScale, 0.35f, 2.5f);
        ImGui::SliderFloat("Caster Spread", &m_S4CasterSpread, 0.45f, 1.8f);
        ImGui::Checkbox("Animate Casters", &m_S4AnimateCasters);
        ImGui::Separator();
        ImGui::SliderFloat("Light Yaw", &m_S4LightYaw, -180.0f, 180.0f);
        ImGui::SliderFloat("Light Pitch", &m_S4LightPitch, -85.0f, -5.0f);
        ImGui::SliderFloat("Light Intensity", &m_S4LightIntensity, 0.0f, 6.0f);
        ImGui::Text("Left: deferred blue. Right: forward red.");
    }
    else if (m_CurrentScenario == 16)
    {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "UI & Responsive Showcase");
        ImGui::SliderFloat("Rotate Card", &m_S16RotateCard, -45.0f, 45.0f);
        ImGui::Checkbox("Show Texture Tile", &m_S16ShowTexture);
        ImGui::Checkbox("Flip Texture X", &m_S16FlipTextureX);
        ImGui::SameLine();
        ImGui::Checkbox("Flip Texture Y", &m_S16FlipTextureY);
        ImGui::Separator();
        ImGui::Combo("Layout Mode", &m_S16LayoutMode, "Compact\0Expanded\0Stacked\0");
        ImGui::SliderFloat("Panel Alpha", &m_S16PanelAlpha, 0.1f, 1.0f);
        ImGui::Text("This merged scene mixes transform, text, image, flex and anchors.");
    }
    else if (m_CurrentScenario == 17)
    {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "Interactive UI");
        ImGui::Text("The in-scene UI is driven by InputScriptable callbacks.");
        ImGui::Text("Hover, click, hold, right-click, and middle-click the UI blocks.");
    }
    else if (m_CurrentScenario == 14)
    {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "Terrain Creation Showcase");
        ImGui::SliderFloat("Terrain Width", &m_S14TerrainWidth, 50.0f, 500.0f);
        ImGui::SliderFloat("Terrain Height", &m_S14TerrainHeight, 5.0f, 150.0f);
        ImGui::SliderFloat("Terrain Length", &m_S14TerrainLength, 50.0f, 500.0f);
        ImGui::SliderFloat("Texture Tile Scale", &m_S14TextureScale, 1.0f, 50.0f);
        ImGui::SliderFloat("Noise Frequency", &m_S14NoiseFrequency, 0.2f, 8.0f);
        ImGui::SliderInt("Noise Octaves", &m_S14NoiseOctaves, 1, 8);
        ImGui::Checkbox("Generate Heightfield Physics", &m_S14GeneratePhysics);
        ImGui::Checkbox("Spawn Dynamic Physics Balls", &m_S14SpawnPhysicsBalls);
        if (ImGui::Checkbox("Show Physics / NavMesh Debug Lines", &m_ShowDebugLines))
        {
            auto& navSystem = GetSystem<NavigationSystem>();
            navSystem.SetShowDebug(m_ShowDebugLines);
        }
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Click 'Reload Scenario' to generate new terrain.");
    }
    else if (m_CurrentScenario == 22)
    {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "Hitray & Hitscan");
        ImGui::Checkbox("Auto Sweep Ray", &m_S22AutoSweep);
        ImGui::SliderFloat("Ray Yaw", &m_S22Yaw, -160.0f, -20.0f);
        ImGui::SliderFloat("Ray Pitch", &m_S22Pitch, -35.0f, 15.0f);
        ImGui::SliderFloat("Ray Distance", &m_S22Distance, 10.0f, 160.0f);
        ImGui::SliderFloat("Hitscan Impulse", &m_S22ImpactImpulse, 0.0f, 80.0f);
        if (ImGui::Button("Fire Hitscan", ImVec2(360, 24)))
            m_S22FireRequested = true;
        ImGui::Text("Last hit: %s", m_S22LastHit.c_str());
        ImGui::Text("Target hits: %d", m_S22HitCount);
        if (ImGui::Checkbox("Show Ray / Physics Debug Lines", &m_ShowDebugLines))
        {
            auto& navSystem = GetSystem<NavigationSystem>();
            navSystem.SetShowDebug(m_ShowDebugLines);
        }
    }
    else if (m_CurrentScenario == 12)
    {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "Light Probe");
        ImGui::SliderFloat("Probe Intensity", &m_S12ProbeIntensity, 0.0f, 4.0f);
        ImGui::SliderFloat("Probe Radius", &m_S12ProbeRadius, 2.0f, 40.0f);
    }
    else if (m_CurrentScenario == 26)
    {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "Character Controller + Collision Zone");
        ImGui::SliderFloat("Move Speed", &m_S26MoveSpeed, 1.0f, 16.0f);
        ImGui::SliderFloat("Sprint Multiplier", &m_S26SprintMultiplier, 1.0f, 3.5f);
        ImGui::SliderFloat("Ctrl Slow Multiplier", &m_S26SlowMultiplier, 0.1f, 0.8f);
        ImGui::SliderFloat("Jump Speed", &m_S26JumpSpeed, 2.0f, 14.0f);
        ImGui::SliderFloat("Step Height", &m_S26StepHeight, 0.05f, 1.5f);
        ImGui::SliderFloat("Max Slope", &m_S26MaxSlope, 5.0f, 80.0f);
        ImGui::Checkbox("Ignore character-trigger tag pair", &m_S26IgnoreCharacterTrigger);
        Scenario26CharacterControllerScript* controllerScript = nullptr;
        if (m_S26ControllerEntity != entt::null && GetScene().IsValid(m_S26ControllerEntity))
        {
            if (auto* script = GetScene().TryGetComponent<ScriptComponent>(m_S26ControllerEntity);
                script && script->instance)
            {
                controllerScript = dynamic_cast<Scenario26CharacterControllerScript*>(script->instance.get());
            }
        }
        if (controllerScript)
        {
            controllerScript->moveSpeed = m_S26MoveSpeed;
            controllerScript->sprintMultiplier = m_S26SprintMultiplier;
            controllerScript->slowMultiplier = m_S26SlowMultiplier;
            controllerScript->jumpSpeed = m_S26JumpSpeed;
            controllerScript->stepHeight = m_S26StepHeight;
            controllerScript->maxSlope = m_S26MaxSlope;
            controllerScript->ignoreCharacterTrigger = m_S26IgnoreCharacterTrigger;
        }
        if (ImGui::Checkbox("Show Hitbox Outline Debug", &m_ShowDebugLines))
        {
            auto& navSystem = GetSystem<NavigationSystem>();
            navSystem.SetShowDebug(m_ShowDebugLines);
        }
        if (ImGui::Button("Reset Character Zone Counters", ImVec2(360, 24)))
        {
            if (controllerScript)
                controllerScript->ResetCounters();
            Scenario26CharacterControllerScript::s_JumpCount = 0;
        }
        if (controllerScript)
        {
            ImGui::Text("Trigger zone enters: %d", controllerScript->triggerEvents);
            ImGui::Text("Solid callback contacts: %d", controllerScript->collisionEvents);
            ImGui::Text("%s", controllerScript->inputState.c_str());
            ImGui::Text("%s", controllerScript->zoneState.c_str());
        }
        else
        {
            ImGui::Text("Scenario26CharacterControllerScript is not initialized.");
        }
        ImGui::Text("Jump count: %d", Scenario26CharacterControllerScript::s_JumpCount);
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
#endif
}
