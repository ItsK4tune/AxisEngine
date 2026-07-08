#include <editor/panels/settings_panel.h>

#ifdef ENABLE_EDITOR
#include <core/logic/config_manager.h>
#include <core/logic/config_serializer.h>
#include <core/logic/event_manager.h>
#include <core/logic/service_locator.h>
#include <core/type/tonemapping_mode.h>
#include <platform/interface/i_device_manager.h>
#include <platform/logic/input_manager.h>
#include <platform/logic/io_handler.h>
#include <platform/logic/monitor_manager.h>
#include <imgui.h>

#ifdef _WIN32
#include <dxgi.h>
#include <intrin.h>
#include <wrl/client.h>

#pragma comment(lib, "dxgi.lib")
#endif

namespace
{
void DrawDeviceSelector(const char* label, IDeviceManager& deviceManager)
{
    ImGui::Text("%s", label);
    auto devices = deviceManager.GetAllDevices();
    auto current = deviceManager.GetCurrentDevice();
    for (const auto& device : devices)
    {
        bool isSelected = (device.id == current.id);
        if (ImGui::RadioButton((device.name + "##" + device.id).c_str(), isSelected))
        {
            deviceManager.SetActiveDevice(device.id);
        }
    }
}
}  // namespace

void SettingsPanel::Initialize()
{
#ifdef _WIN32
    // GPU Detection
    Microsoft::WRL::ComPtr<IDXGIFactory> factory;
    if (SUCCEEDED(CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)factory.GetAddressOf())))
    {
        Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
        if (SUCCEEDED(factory->EnumAdapters(0, adapter.GetAddressOf())))
        {
            DXGI_ADAPTER_DESC desc;
            if (SUCCEEDED(adapter->GetDesc(&desc)))
            {
                char buf[128] = {};
                WideCharToMultiByte(CP_UTF8, 0, desc.Description, -1, buf, sizeof(buf), nullptr, nullptr);
                m_GpuName = std::string(buf);
            }
        }
    }

    // CPU Detection
    int cpuInfo[4] = {0};
    char cpuBrandString[0x40] = {0};
    __cpuid(cpuInfo, 0x80000000);
    if (cpuInfo[0] >= 0x80000004)
    {
        __cpuid((int*)(cpuBrandString + 0), 0x80000002);
        __cpuid((int*)(cpuBrandString + 16), 0x80000003);
        __cpuid((int*)(cpuBrandString + 32), 0x80000004);
        m_CpuName = std::string(cpuBrandString);
        size_t start = m_CpuName.find_first_not_of(" \t");
        size_t end = m_CpuName.find_last_not_of(" \t");
        if (start != std::string::npos && end != std::string::npos)
            m_CpuName = m_CpuName.substr(start, end - start + 1);
    }
#endif
}

void SettingsPanel::OnImGui(Scene& scene)
{
    ImGui::Begin(GetTitle().c_str(), &m_Open);

    auto* cm = ServiceLocator::Instance().Resolve<ConfigManager>();
    if (!cm)
    {
        ImGui::End();
        return;
    }

    auto conf = cm->GetConfig();
    bool changed = false;

    if (ImGui::CollapsingHeader("Core", ImGuiTreeNodeFlags_DefaultOpen))
    {
        char titleBuf[256];
        strncpy(titleBuf, conf.title.c_str(), sizeof(titleBuf));
        titleBuf[sizeof(titleBuf) - 1] = '\0';
        if (ImGui::InputText("Title", titleBuf, sizeof(titleBuf)))
        {
            conf.title = titleBuf;
            changed = true;
        }

        static const char* logLevels[] = {"None", "Minimal", "Flex", "Verbose", "Debug"};
        int currentLogLevel = (int)conf.logLevel;
        if (ImGui::Combo("Log Level", &currentLogLevel, logLevels, IM_ARRAYSIZE(logLevels)))
        {
            conf.logLevel = (LogLevel)currentLogLevel;
            changed = true;
        }

        if (ImGui::InputInt("Job Threads", &conf.numJobThreads))
            changed = true;

        if (ImGui::SliderFloat("Time Scale", &conf.timeScale, 0.0f, 5.0f))
            changed = true;

        if (ImGui::Checkbox("Headless Mode", &conf.headlessMode))
            changed = true;
    }

    if (ImGui::CollapsingHeader("Window"))
    {
        static const char* windowModes[] = {"Windowed", "Fullscreen", "Borderless", "BorderlessFullscreen"};
        int currentMode = (int)conf.window.windowMode;
        if (ImGui::Combo("Window Mode", &currentMode, windowModes, IM_ARRAYSIZE(windowModes)))
        {
            conf.window.windowMode = (WindowMode)currentMode;
            changed = true;
        }

        if (ImGui::InputInt("Resolution Width", &conf.window.width))
            changed = true;
        if (ImGui::InputInt("Resolution Height", &conf.window.height))
            changed = true;
        if (ImGui::Checkbox("VSync", &conf.window.vsync))
            changed = true;
        if (ImGui::SliderInt("Frame Rate Limit", &conf.window.frameRateLimit, 0, 360))
            changed = true;
        if (ImGui::InputInt("Refresh Rate", &conf.window.refreshRate))
            changed = true;
    }

    if (ImGui::CollapsingHeader("Graphics"))
    {
        static const char* apis[] = {"OpenGL", "Vulkan", "DirectX"};
        int currentApi = (int)conf.graphics.graphicsBackend;
        ImGui::BeginDisabled();
        ImGui::Combo("Graphics API (Restart Required)", &currentApi, apis, IM_ARRAYSIZE(apis));
        ImGui::EndDisabled();

        if (ImGui::SliderInt("MSAA Samples", &conf.graphics.msaaSamples, 1, 16))
            changed = true;

        static const char* aaModes[] = {"Off", "FXAA", "TAA"};
        int current_aa = conf.graphics.antialiasing;
        if (ImGui::Combo("Anti-Aliasing", &current_aa, aaModes, IM_ARRAYSIZE(aaModes)))
        {
            conf.graphics.antialiasing = current_aa;
            changed = true;
        }
        if (ImGui::SliderFloat("Max Anisotropy", &conf.graphics.maxAnisotropy, 1.0f, 16.0f))
            changed = true;
        if (ImGui::SliderFloat("Render Scale", &conf.graphics.renderScale, 0.1f, 2.0f))
            changed = true;
        if (ImGui::Checkbox("Async Resource Loading", &conf.graphics.asyncResourceLoading))
            changed = true;
        if (ImGui::Checkbox("Strict Asset Loading", &conf.graphics.strictAssetLoading))
            changed = true;
    }

    if (ImGui::CollapsingHeader("Render"))
    {
        ImGui::Separator();
        if (ImGui::Checkbox("HDR", &conf.render.hdrEnabled))
            changed = true;

        if (!conf.render.hdrEnabled)
            ImGui::BeginDisabled();
        if (ImGui::SliderFloat("Gamma", &conf.render.gamma, 1.0f, 3.0f))
            changed = true;
        if (ImGui::SliderFloat("Exposure", &conf.render.exposure, 0.1f, 5.0f))
            changed = true;
        static const char* tonemapModes[] = {"Linear", "ACES", "Reinhard", "Uncharted2"};
        int currentTonemap = (int)conf.render.tonemappingMode;
        if (ImGui::Combo("Tonemapping", &currentTonemap, tonemapModes, IM_ARRAYSIZE(tonemapModes)))
        {
            conf.render.tonemappingMode = (TonemappingMode)currentTonemap;
            changed = true;
        }
        if (!conf.render.hdrEnabled)
            ImGui::EndDisabled();

        ImGui::Separator();
        if (ImGui::ColorEdit4("Clear Color", conf.render.clearColor))
            changed = true;

        ImGui::Separator();
        if (ImGui::Checkbox("Bloom Enabled", &conf.render.bloomEnabled))
            changed = true;

        if (!conf.render.bloomEnabled)
            ImGui::BeginDisabled();
        if (ImGui::SliderFloat("Bloom Intensity", &conf.render.bloomIntensity, 0.0f, 10.0f))
            changed = true;
        if (ImGui::SliderFloat("Bloom Threshold", &conf.render.bloomThreshold, 0.0f, 2.0f))
            changed = true;
        if (ImGui::SliderFloat("Bloom Radius", &conf.render.bloomRadius, 0.001f, 0.1f))
            changed = true;
        if (!conf.render.bloomEnabled)
            ImGui::EndDisabled();

        ImGui::Separator();
        if (ImGui::SliderFloat("Skybox Intensity", &conf.render.skyboxIntensity, 0.0f, 5.0f))
            changed = true;
        if (ImGui::SliderFloat("Ambient Intensity", &conf.render.ambientIntensity, 0.0f, 5.0f))
            changed = true;

        float uiRef[2] = { conf.render.uiReferenceWidth, conf.render.uiReferenceHeight };
        if (ImGui::DragFloat2("UI Reference Size", uiRef, 1.0f, 100.0f, 8192.0f))
        {
            conf.render.uiReferenceWidth = uiRef[0];
            conf.render.uiReferenceHeight = uiRef[1];
            changed = true;
        }

        static const char* lightingModes[] = {"Bake", "Light Probe", "Reflection Probes", "Real Time"};
        int currentLighting = (int)conf.lightingMode;
        if (ImGui::Combo("Lighting Mode", &currentLighting, lightingModes, IM_ARRAYSIZE(lightingModes)))
        {
            conf.lightingMode = (LightingMode)currentLighting;
            changed = true;
        }
    }

    if (ImGui::CollapsingHeader("Shadows"))
    {
        if (ImGui::Checkbox("Shadows Enabled", &conf.shadow.shadowsEnabled))
            changed = true;

        if (!conf.shadow.shadowsEnabled)
            ImGui::BeginDisabled();

        static const char* shadowModes[] = {"Off (None)", "Single Light", "Multi Lights"};
        int currentShadowMode = conf.shadow.shadowMode;
        if (ImGui::Combo("Shadow Mode", &currentShadowMode, shadowModes, IM_ARRAYSIZE(shadowModes)))
        {
            conf.shadow.shadowMode = currentShadowMode;
            changed = true;
        }

        if (ImGui::SliderInt("Shadow Map Resolution", &conf.shadow.shadowMapResolution, 512, 8192))
            changed = true;
        if (ImGui::SliderFloat("Shadow Projection Size", &conf.shadow.shadowProjectionSize, 10.0f, 500.0f))
            changed = true;
        if (ImGui::Checkbox("Shadow Frustum Culling", &conf.shadow.shadowFrustumCullingEnabled))
            changed = true;

        if (!conf.shadow.shadowFrustumCullingEnabled)
            ImGui::BeginDisabled();
        if (ImGui::SliderFloat("Shadow Distance Culling", &conf.shadow.shadowDistanceCulling, 10.0f, 1000.0f))
            changed = true;
        if (!conf.shadow.shadowFrustumCullingEnabled)
            ImGui::EndDisabled();

        if (ImGui::SliderFloat("Shadow Bias", &conf.shadow.shadowBias, 0.001f, 0.1f))
            changed = true;
        if (ImGui::SliderInt("Shadow Softness", &conf.shadow.shadowSoftness, 0, 5))
            changed = true;

        if (!conf.shadow.shadowsEnabled)
            ImGui::EndDisabled();
    }

    if (ImGui::CollapsingHeader("Physics"))
    {
        static const char* physEngines[] = {"Bullet", "PhysX"};
        int currentPhysEngine = (int)conf.physics.physicsBackend;
        ImGui::BeginDisabled();
        ImGui::Combo("Physics Engine (Restart Required)", &currentPhysEngine, physEngines, IM_ARRAYSIZE(physEngines));
        ImGui::EndDisabled();

        static const char* physModes[] = {"Fast", "Balanced", "Accurate"};
        int currentPhysMode = (int)conf.physics.physicsMode;
        if (ImGui::Combo("Physics Mode", &currentPhysMode, physModes, IM_ARRAYSIZE(physModes)))
        {
            conf.physics.physicsMode = (PhysicsMode)currentPhysMode;
            changed = true;
        }

        if (ImGui::DragFloat3("Gravity", conf.physics.gravity, 0.1f))
            changed = true;
        if (ImGui::SliderInt("Max SubSteps", &conf.physics.maxSubSteps, 1, 20))
            changed = true;
        if (ImGui::SliderFloat("Tick Rate", &conf.physics.physicsTickRate, 10.0f, 120.0f))
            changed = true;
        if (ImGui::Checkbox("CCD Enabled", &conf.physics.ccdEnabled))
            changed = true;

        if (!conf.physics.ccdEnabled)
            ImGui::BeginDisabled();
        if (ImGui::SliderFloat("CCD Threshold", &conf.physics.ccdThreshold, 0.0f, 100.0f))
            changed = true;
        if (!conf.physics.ccdEnabled)
            ImGui::EndDisabled();

        if (ImGui::SliderInt("Solver Iterations", &conf.physics.solverIterations, 1, 50))
            changed = true;
    }

    if (ImGui::CollapsingHeader("Input & Audio"))
    {
        if (ImGui::SliderFloat("Mouse Sens X", &conf.input.mouseSensitivityX, 0.01f, 1.0f))
            changed = true;
        if (ImGui::SliderFloat("Mouse Sens Y", &conf.input.mouseSensitivityY, 0.01f, 1.0f))
            changed = true;
        if (ImGui::Checkbox("Invert X", &conf.input.mouseInvertX))
            changed = true;
        if (ImGui::Checkbox("Invert Y", &conf.input.mouseInvertY))
            changed = true;
        if (ImGui::Checkbox("Raw Mouse Input", &conf.input.rawMouseInput))
            changed = true;

        ImGui::Separator();
        static const char* audioEngines[] = {"Null", "IrrKlang", "FMOD", "OpenAL"};
        int currentAudioEngine = (int)conf.audio.audioBackend;
        ImGui::BeginDisabled();
        ImGui::Combo("Audio Engine (Restart Required)", &currentAudioEngine, audioEngines, IM_ARRAYSIZE(audioEngines));
        ImGui::EndDisabled();

        if (ImGui::SliderFloat("Master Volume", &conf.audio.masterVolume, 0.0f, 100.0f))
            changed = true;

        char audioDevBuf[256];
        strncpy(audioDevBuf, conf.audio.audioDevice.c_str(), sizeof(audioDevBuf));
        audioDevBuf[sizeof(audioDevBuf) - 1] = '\0';
        if (ImGui::InputText("Audio Device", audioDevBuf, sizeof(audioDevBuf)))
        {
            conf.audio.audioDevice = audioDevBuf;
            changed = true;
        }
    }

    if (ImGui::CollapsingHeader("Hardware & Devices"))
    {
        ImGui::Text("CPU: %s", m_CpuName.c_str());
        ImGui::Text("GPU: %s", m_GpuName.c_str());

        auto* io = ServiceLocator::Instance().Resolve<IOHandler>();
        if (io)
        {
            ImGui::Separator();
            IDeviceManager& monitorDevices = io->GetMonitorManager();
            DrawDeviceSelector("Active Monitor:", monitorDevices);

            ImGui::Separator();
            IDeviceManager& inputDevices = io->GetInputManager();
            DrawDeviceSelector("Input Devices:", inputDevices);
        }
    }

    if (ImGui::CollapsingHeader("Culling"))
    {
        if (ImGui::Checkbox("Frustum Culling", &conf.culling.frustumCullingEnabled))
            changed = true;
        if (ImGui::Checkbox("Depth Test", &conf.culling.depthTestEnabled))
            changed = true;
        if (ImGui::Checkbox("Stencil Test", &conf.culling.stencilTestEnabled))
            changed = true;
        if (ImGui::Checkbox("Cull Face", &conf.culling.cullFaceEnabled))
            changed = true;
        if (ImGui::Checkbox("Occlusion Culling", &conf.culling.occlusionCullingEnabled))
            changed = true;
        if (ImGui::Checkbox("Instance Batching", &conf.culling.instanceBatchingEnabled))
            changed = true;
        if (ImGui::Checkbox("Render Order Sorting", &conf.culling.renderOrderEnabled))
            changed = true;
        
        ImGui::Text("Filter Layers (0-7):");
        for (int i = 0; i < 8; ++i)
        {
            bool layerActive = (conf.culling.filterLayerMask & (1 << i)) != 0;
            char label[16];
            sprintf(label, "L%d", i);
            if (i > 0)
                ImGui::SameLine();
            if (ImGui::Checkbox(label, &layerActive))
            {
                if (layerActive)
                    conf.culling.filterLayerMask |= (1 << i);
                else
                    conf.culling.filterLayerMask &= ~(1 << i);
                changed = true;
            }
        }
        
        unsigned int hexMask = conf.culling.filterLayerMask;
        if (ImGui::InputScalar("Filter Mask (Hex)", ImGuiDataType_U32, &hexMask, nullptr, nullptr, "%08X", ImGuiInputTextFlags_CharsHexadecimal))
        {
            conf.culling.filterLayerMask = hexMask;
            changed = true;
        }

        if (ImGui::SliderFloat("Distance Culling", &conf.culling.distanceCulling, 0.0f, 5000.0f))
            changed = true;
    }

    // Debug Configurations moved to Tools Panel

    if (changed || ImGui::IsItemDeactivatedAfterEdit())
    {
        cm->UpdateConfig(conf, ConfigChangedEvent::All);
    }

    ImGui::Separator();
    if (ImGui::Button("Save Config"))
    {
        ConfigSerializer serializer;
        serializer.Serialize("include/engine/asset/config.axs", conf);
    }

    ImGui::End();
}
#endif
