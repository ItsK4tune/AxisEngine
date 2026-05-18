#include <editor/panels/settings_panel.h>

#ifdef ENABLE_EDITOR
#include <core/logic/config_manager.h>
#include <core/logic/event_manager.h>
#include <core/logic/service_locator.h>
#include <core/type/tonemapping_mode.h>
#include <platform/logic/input_manager.h>
#include <platform/logic/io_handler.h>
#include <platform/logic/monitor_manager.h>
#include <imgui.h>
#include <intrin.h>

#ifdef _WIN32
#include <dxgi.h>
#include <wrl/client.h>

#pragma comment(lib, "dxgi.lib")
#endif

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
    }

    if (ImGui::CollapsingHeader("Graphics"))
    {
        if (ImGui::SliderInt("MSAA Samples", &conf.graphics.msaaSamples, 1, 16))
            changed = true;

        static const char* aaModes[] = {"Off", "FXAA", "TAA"};
        int current_aa = conf.graphics.antialiasing;
        if (ImGui::Combo("Anti-Aliasing", &current_aa, aaModes, IM_ARRAYSIZE(aaModes)))
        {
            conf.graphics.antialiasing = current_aa;
            changed = true;
        }
        if (ImGui::SliderFloat("Render Scale", &conf.graphics.renderScale, 0.1f, 2.0f))
            changed = true;
        if (ImGui::Checkbox("Async Resource Loading", &conf.graphics.asyncResourceLoading))
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
    }

    if (ImGui::CollapsingHeader("Shadows"))
    {
        if (ImGui::Checkbox("Shadows Enabled", &conf.shadow.shadowsEnabled))
            changed = true;
        if (ImGui::SliderInt("Shadow Map Resolution", &conf.shadow.shadowMapResolution, 512, 8192))
            changed = true;
        if (ImGui::SliderFloat("Shadow Projection Size", &conf.shadow.shadowProjectionSize, 10.0f, 500.0f))
            changed = true;
        if (ImGui::SliderFloat("Shadow Bias", &conf.shadow.shadowBias, 0.001f, 0.1f))
            changed = true;
        if (ImGui::SliderInt("Shadow Softness", &conf.shadow.shadowSoftness, 0, 5))
            changed = true;
    }

    if (ImGui::CollapsingHeader("Physics"))
    {
        // Physics Backend & Mode are enums, let's keep it simple with gravity/substeps
        if (ImGui::DragFloat3("Gravity", conf.physics.gravity, 0.1f))
            changed = true;
        if (ImGui::SliderInt("Max SubSteps", &conf.physics.maxSubSteps, 1, 20))
            changed = true;
        if (ImGui::SliderFloat("Tick Rate", &conf.physics.physicsTickRate, 10.0f, 120.0f))
            changed = true;
        if (ImGui::Checkbox("CCD Enabled", &conf.physics.ccdEnabled))
            changed = true;
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
        if (ImGui::SliderFloat("Master Volume", &conf.audio.masterVolume, 0.0f, 100.0f))
            changed = true;
    }

    if (ImGui::CollapsingHeader("Hardware & Devices", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("CPU: %s", m_CpuName.c_str());
        ImGui::Text("GPU: %s", m_GpuName.c_str());

        auto* io = ServiceLocator::Instance().Resolve<IOHandler>();
        if (io)
        {
            ImGui::Separator();
            ImGui::Text("Active Monitor:");
            auto monitors = io->GetMonitorManager().GetAllDevices();
            auto currentMon = io->GetMonitorManager().GetCurrentDevice();
            for (const auto& mon : monitors)
            {
                bool isSelected = (mon.id == currentMon.id);
                if (ImGui::RadioButton((mon.name + "##" + mon.id).c_str(), isSelected))
                {
                    io->GetMonitorManager().SetActiveDevice(mon.id);
                }
            }

            ImGui::Separator();
            ImGui::Text("Input Devices:");
            auto inputs = io->GetInputManager().GetAllDevices();
            auto currentInput = io->GetInputManager().GetCurrentDevice();
            for (const auto& input : inputs)
            {
                bool isSelected = (input.id == currentInput.id);
                if (ImGui::RadioButton((input.name + "##" + input.id).c_str(), isSelected))
                {
                    io->GetInputManager().SetActiveDevice(input.id);
                }
            }
        }
    }

    if (ImGui::CollapsingHeader("Culling"))
    {
        if (ImGui::Checkbox("Frustum Culling", &conf.culling.frustumCullingEnabled))
            changed = true;
        if (ImGui::Checkbox("Depth Test", &conf.culling.depthTestEnabled))
            changed = true;
        if (ImGui::Checkbox("Cull Face", &conf.culling.cullFaceEnabled))
            changed = true;
    }

    if (changed || ImGui::IsItemDeactivatedAfterEdit())
    {
        cm->UpdateConfig(conf, ConfigChangedEvent::All);
    }

    ImGui::End();
}
#endif
