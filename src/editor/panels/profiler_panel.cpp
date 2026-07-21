#include <editor/panels/profiler_panel.h>

#ifdef ENABLE_EDITOR
#include <core/logic/service_locator.h>
#include <core/logic/runtime_profiler.h>
#include <ecs/interface/i_render_service.h>
#include <render/interface/i_graphics_context.h>
#include <ecs/unit/decal_component.h>
#include <ecs/unit/light_components.h>
#include <ecs/unit/media_components.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/physics_components.h>
#include <ecs/unit/post_process_component.h>
#include <ecs/unit/reflection_components.h>
#include <ecs/unit/render_components.h>
#include <ecs/unit/terrain_component.h>
#include <ecs/unit/ui_components.h>
#include <render/unit/render_queue.h>
#include <scene/logic/scene.h>
#include <imgui.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_set>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <psapi.h>
#elif defined(__linux__)
#include <sys/sysinfo.h>
#include <unistd.h>
#endif

namespace
{

struct RenderPanelStats
{
    int drawCalls = 0;
    uint64_t triangles = 0;
    size_t visibleObjects = 0;
    size_t culledObjects = 0;

    size_t deferredOpaque = 0;
    size_t forwardOpaque = 0;
    size_t transparent = 0;
    size_t depthOverlay = 0;
    size_t shadowItems = 0;
    size_t queuedItems = 0;
    size_t lights = 0;
    uint64_t shadowTriangles = 0;
    int width = -1;
    int height = -1;
};

std::string FormatBytes(uint64_t bytes)
{
    static const char* units[] = {"B", "KiB", "MiB", "GiB", "TiB"};
    double value = static_cast<double>(bytes);
    int unit = 0;
    while (value >= 1024.0 && unit < 4)
    {
        value /= 1024.0;
        ++unit;
    }

    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), unit == 0 ? "%.0f %s" : "%.2f %s", value, units[unit]);
    return buffer;
}

std::string FormatBytePair(uint64_t used, uint64_t total)
{
    return FormatBytes(used) + " / " + FormatBytes(total);
}

void DrawMetric(const char* label, const char* fmt, ...)
{
    char value[160];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(value, sizeof(value), fmt, args);
    va_end(args);

    ImGui::TextUnformatted(label);
    ImGui::NextColumn();
    ImGui::TextUnformatted(value);
    ImGui::NextColumn();
}

void DrawMetricText(const char* label, const std::string& value)
{
    ImGui::TextUnformatted(label);
    ImGui::NextColumn();
    ImGui::TextUnformatted(value.c_str());
    ImGui::NextColumn();
}

uint64_t CountModelTriangles(const Model* model)
{
    if (!model)
        return 0;

    uint64_t triangles = 0;
    for (const auto& mesh : model->meshes)
    {
        if (!mesh.indices.empty())
            triangles += static_cast<uint64_t>(mesh.indices.size() / 3);
        else
            triangles += static_cast<uint64_t>(mesh.m_VertexCount / 3);
    }
    return triangles;
}

uint64_t CountQueueTriangles(const std::vector<RenderItem>& queue)
{
    uint64_t triangles = 0;
    for (const auto& item : queue)
    {
        triangles += CountModelTriangles(item.model);
    }
    return triangles;
}

void AddVisibleEntities(const std::vector<RenderItem>& queue, std::unordered_set<uint32_t>& entities)
{
    for (const auto& item : queue)
    {
        if (item.entityId != 0)
            entities.insert(item.entityId);
    }
}

RenderPanelStats CollectRenderStats(Scene& scene)
{
    RenderPanelStats stats;

    auto& registry = scene.GetRegistry();
    const size_t meshRenderers = registry.view<MeshRendererComponent>().size();

    auto* renderService = ServiceLocator::Instance().Resolve<IRenderService>();
    if (!renderService)
    {
        stats.culledObjects = 0;
        stats.visibleObjects = meshRenderers;
        return stats;
    }

    stats.drawCalls = renderService->GetRenderedCount();
    stats.width = renderService->GetLastWidth();
    stats.height = renderService->GetLastHeight();

    auto& queue = renderService->GetRenderQueueObj();
    const auto& deferred = queue.GetDeferredOpaqueQueue();
    const auto& forward = queue.GetForwardOpaqueQueue();
    const auto& transparent = queue.GetTransparentQueue();
    const auto& depthOverlay = queue.GetDepthOverlayQueue();
    const auto& shadows = queue.GetShadowQueue();

    stats.deferredOpaque = deferred.size();
    stats.forwardOpaque = forward.size();
    stats.transparent = transparent.size();
    stats.depthOverlay = depthOverlay.size();
    stats.shadowItems = shadows.size();
    stats.lights = queue.GetLights().size();
    stats.queuedItems = stats.deferredOpaque + stats.forwardOpaque + stats.transparent + stats.depthOverlay;

    stats.triangles = CountQueueTriangles(deferred) + CountQueueTriangles(forward) + CountQueueTriangles(transparent) +
                      CountQueueTriangles(depthOverlay);
    stats.shadowTriangles = CountQueueTriangles(shadows);

    std::unordered_set<uint32_t> visibleEntities;
    visibleEntities.reserve(stats.queuedItems);
    AddVisibleEntities(deferred, visibleEntities);
    AddVisibleEntities(forward, visibleEntities);
    AddVisibleEntities(transparent, visibleEntities);
    AddVisibleEntities(depthOverlay, visibleEntities);

    stats.visibleObjects = visibleEntities.empty() ? stats.queuedItems : visibleEntities.size();
    stats.culledObjects = meshRenderers > stats.visibleObjects ? meshRenderers - stats.visibleObjects : 0;

    return stats;
}

int CountImGuiDrawCalls()
{
    ImDrawData* drawData = ImGui::GetDrawData();
    if (!drawData)
        return 0;

    int drawCalls = 0;
    for (int listIndex = 0; listIndex < drawData->CmdListsCount; ++listIndex)
    {
        drawCalls += drawData->CmdLists[listIndex]->CmdBuffer.Size;
    }
    return drawCalls;
}

size_t CountInactiveInfoEntities(entt::registry& registry)
{
    size_t inactive = 0;
    auto view = registry.view<InfoComponent>();
    for (auto entity : view)
    {
        if (!view.get<InfoComponent>(entity).isActive)
            ++inactive;
    }
    return inactive;
}

bool ReadUint64File(const std::filesystem::path& path, uint64_t& value)
{
    std::ifstream file(path);
    return static_cast<bool>(file >> value);
}

bool ReadFloatFile(const std::filesystem::path& path, float& value)
{
    std::ifstream file(path);
    return static_cast<bool>(file >> value);
}

bool TryReadLinuxGpuUsage(float& usagePercent)
{
#if defined(__linux__)
    namespace fs = std::filesystem;
    std::error_code ec;
    const fs::path drmPath("/sys/class/drm");
    if (!fs::exists(drmPath, ec))
        return false;

    for (const auto& entry : fs::directory_iterator(drmPath, ec))
    {
        if (ec)
            break;
        const fs::path busyPath = entry.path() / "device" / "gpu_busy_percent";
        if (ReadFloatFile(busyPath, usagePercent))
            return true;
    }
#endif
    return false;
}

bool TryReadLinuxVram(uint64_t& usedBytes, uint64_t& totalBytes)
{
#if defined(__linux__)
    namespace fs = std::filesystem;
    std::error_code ec;
    const fs::path drmPath("/sys/class/drm");
    if (!fs::exists(drmPath, ec))
        return false;

    for (const auto& entry : fs::directory_iterator(drmPath, ec))
    {
        if (ec)
            break;
        const fs::path devicePath = entry.path() / "device";
        uint64_t used = 0;
        uint64_t total = 0;
        if (ReadUint64File(devicePath / "mem_info_vram_used", used) &&
            ReadUint64File(devicePath / "mem_info_vram_total", total) && total > 0)
        {
            usedBytes = used;
            totalBytes = total;
            return true;
        }
    }
#endif
    return false;
}

float GetSmoothingAlpha(float dt, float smoothingWindowSeconds)
{
    if (dt <= 0.0f)
        return 1.0f;

    return 1.0f - std::exp(-dt / smoothingWindowSeconds);
}

void SmoothValue(float& value, float sample, float alpha, bool hasPreviousSample)
{
    value = hasPreviousSample ? value + (sample - value) * alpha : sample;
}
}  // namespace

void ProfilerPanel::OnUpdate(float dt)
{
    m_FrameTime = dt * 1000.0f;
    m_CpuFrameTime = m_FrameTime;
    m_RawFps = dt > 0.0f ? 1.0f / dt : 0.0f;

    m_FrameTimeHistory[m_HistoryOffset] = m_FrameTime;
    m_FrameDeltaHistory[m_HistoryOffset] = dt;
    m_HistoryOffset = (m_HistoryOffset + 1) % FRAME_HISTORY_SIZE;

    RecalculateFrameWindowStats();
    SmoothRuntimeStats(dt);
    UpdateSystemStats(dt);
}

void ProfilerPanel::RecalculateFrameWindowStats()
{
    m_MinFrameTime = 999.0f;
    m_MaxFrameTime = 0.0f;
    float sum = 0.0f;
    int sampleCount = 0;
    float windowSeconds = 0.0f;
    m_GraphMaxFrameTime = 0.0f;

    for (int i = 0; i < FRAME_HISTORY_SIZE; ++i)
    {
        const float ft = m_FrameTimeHistory[i];
        if (ft > 0.01f)
            m_GraphMaxFrameTime = (std::max)(m_GraphMaxFrameTime, ft);
    }

    for (int i = 0; i < FRAME_HISTORY_SIZE && windowSeconds < SMOOTHING_WINDOW_SECONDS; ++i)
    {
        const int index = (m_HistoryOffset - 1 - i + FRAME_HISTORY_SIZE) % FRAME_HISTORY_SIZE;
        const float ft = m_FrameTimeHistory[index];
        const float sampleDt = m_FrameDeltaHistory[index];
        if (ft > 0.01f && sampleDt > 0.0f)
        {
            m_MinFrameTime = (std::min)(m_MinFrameTime, ft);
            m_MaxFrameTime = (std::max)(m_MaxFrameTime, ft);
            sum += ft;
            windowSeconds += sampleDt;
            ++sampleCount;
        }
    }

    m_AvgFrameTime = sampleCount > 0 ? sum / static_cast<float>(sampleCount) : 0.0f;
    m_Fps = windowSeconds > 0.0f ? static_cast<float>(sampleCount) / windowSeconds : m_RawFps;
}

void ProfilerPanel::SmoothRuntimeStats(float dt)
{
    const RuntimeProfilerStats& runtimeStats = RuntimeProfiler::Instance().GetStats();
    const float alpha = GetSmoothingAlpha(dt, SMOOTHING_WINDOW_SECONDS);

    const float cpuFrameSample = runtimeStats.cpuFrameMs > 0.0f ? runtimeStats.cpuFrameMs : m_FrameTime;
    const float gpuFrameSample = runtimeStats.hasGpuFrameTime ? runtimeStats.gpuFrameMs : cpuFrameSample;
    const float gpuUsageSample =
        runtimeStats.hasGpuUsage || runtimeStats.hasGpuFrameTime ? runtimeStats.gpuUsagePercent : 0.0f;

    SmoothValue(m_SmoothedCpuFrameTime, cpuFrameSample, alpha, m_HasRuntimeSmoothSample);
    SmoothValue(m_SmoothedGpuFrameTime, gpuFrameSample, alpha, m_HasRuntimeSmoothSample);
    SmoothValue(m_SmoothedGpuUsage, gpuUsageSample, alpha, m_HasRuntimeSmoothSample);

    for (size_t i = 0; i < m_SmoothedPassMs.size(); ++i)
    {
        SmoothValue(m_SmoothedPassMs[i], runtimeStats.passMs[i], alpha, m_HasRuntimeSmoothSample);
    }

    m_HasRuntimeSmoothSample = true;
}

void ProfilerPanel::UpdateSystemStats(float dt)
{
    m_SystemStatsTimer += dt;
    if (m_SystemStatsTimer < 0.5f && m_HasCpuSample)
        return;
    const float statsInterval = m_SystemStatsTimer;
    m_SystemStatsTimer = 0.0f;
    const float previousCpuUsageTotal = m_CpuUsageTotal;
    const std::vector<float> previousCpuUsagePerCore = m_CpuUsagePerCore;
    const bool hadCpuUsageSmoothSample = m_HasCpuUsageSmoothSample;

    auto calcUsage = [](const CpuTimes& prev, const CpuTimes& curr) {
        uint64_t totalDelta = curr.total - prev.total;
        uint64_t idleDelta = curr.idle - prev.idle;
        if (totalDelta == 0 || idleDelta > totalDelta)
            return 0.0f;
        return 100.0f * static_cast<float>(totalDelta - idleDelta) / static_cast<float>(totalDelta);
    };

#if defined(__linux__)
    std::vector<CpuTimes> cpuTimes;
    std::ifstream stat("/proc/stat");
    std::string line;
    while (std::getline(stat, line))
    {
        if (line.rfind("cpu", 0) != 0)
            break;

        std::istringstream iss(line);
        std::string label;
        uint64_t user = 0;
        uint64_t nice = 0;
        uint64_t system = 0;
        uint64_t idle = 0;
        uint64_t iowait = 0;
        uint64_t irq = 0;
        uint64_t softirq = 0;
        uint64_t steal = 0;
        uint64_t guest = 0;
        uint64_t guestNice = 0;

        iss >> label >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal >> guest >> guestNice;
        if (label == "cpu" || (label.size() > 3 && std::isdigit(static_cast<unsigned char>(label[3]))))
        {
            CpuTimes sample;
            sample.idle = idle + iowait;
            sample.total = user + nice + system + idle + iowait + irq + softirq + steal + guest + guestNice;
            cpuTimes.push_back(sample);
        }
    }

    if (!cpuTimes.empty())
    {
        if (m_HasCpuSample && m_LastCpuTimes.size() == cpuTimes.size())
        {
            m_CpuUsageTotal = calcUsage(m_LastCpuTimes[0], cpuTimes[0]);
            m_CpuUsagePerCore.clear();
            for (size_t i = 1; i < cpuTimes.size(); ++i)
            {
                m_CpuUsagePerCore.push_back(calcUsage(m_LastCpuTimes[i], cpuTimes[i]));
            }
        }
        else
        {
            m_CpuUsageTotal = 0.0f;
            m_CpuUsagePerCore.assign(cpuTimes.size() > 0 ? cpuTimes.size() - 1 : 0, 0.0f);
        }
        m_LastCpuTimes = cpuTimes;
        m_HasCpuSample = true;
    }

    uint64_t memTotalKb = 0;
    uint64_t memAvailableKb = 0;
    std::ifstream meminfo("/proc/meminfo");
    while (std::getline(meminfo, line))
    {
        std::istringstream iss(line);
        std::string key;
        uint64_t value = 0;
        std::string unit;
        iss >> key >> value >> unit;
        if (key == "MemTotal:")
            memTotalKb = value;
        else if (key == "MemAvailable:")
            memAvailableKb = value;
    }
    if (memTotalKb > 0)
    {
        m_RamTotalBytes = memTotalKb * 1024ULL;
        m_RamUsedBytes = (memTotalKb > memAvailableKb ? memTotalKb - memAvailableKb : 0) * 1024ULL;
    }

    std::ifstream statm("/proc/self/statm");
    uint64_t pages = 0;
    uint64_t residentPages = 0;
    if (statm >> pages >> residentPages)
    {
        long pageSize = sysconf(_SC_PAGESIZE);
        if (pageSize > 0)
            m_ProcessRamBytes = residentPages * static_cast<uint64_t>(pageSize);
    }
#elif defined(_WIN32)
    struct ProcessorPerformanceInfo
    {
        LARGE_INTEGER idleTime;
        LARGE_INTEGER kernelTime;
        LARGE_INTEGER userTime;
        LARGE_INTEGER dpcTime;
        LARGE_INTEGER interruptTime;
        ULONG interruptCount;
    };

    using NtQuerySystemInformationFn = LONG(WINAPI*)(ULONG, PVOID, ULONG, PULONG);
    constexpr ULONG SystemProcessorPerformanceInformation = 8;

    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    auto querySystemInformation =
        ntdll ? reinterpret_cast<NtQuerySystemInformationFn>(GetProcAddress(ntdll, "NtQuerySystemInformation"))
              : nullptr;

    if (querySystemInformation)
    {
        const DWORD processorCount = (std::max<DWORD>)(GetActiveProcessorCount(ALL_PROCESSOR_GROUPS), 1);
        std::vector<ProcessorPerformanceInfo> processorInfo(processorCount);
        ULONG returnLength = 0;
        const LONG status = querySystemInformation(
            SystemProcessorPerformanceInformation, processorInfo.data(),
            static_cast<ULONG>(processorInfo.size() * sizeof(ProcessorPerformanceInfo)), &returnLength);

        if (status >= 0)
        {
            std::vector<CpuTimes> cpuTimes;
            cpuTimes.resize(processorInfo.size() + 1);
            for (size_t i = 0; i < processorInfo.size(); ++i)
            {
                CpuTimes core;
                core.idle = static_cast<uint64_t>(processorInfo[i].idleTime.QuadPart);
                core.total =
                    static_cast<uint64_t>(processorInfo[i].kernelTime.QuadPart + processorInfo[i].userTime.QuadPart);

                cpuTimes[0].idle += core.idle;
                cpuTimes[0].total += core.total;
                cpuTimes[i + 1] = core;
            }

            if (m_HasCpuSample && m_LastCpuTimes.size() == cpuTimes.size())
            {
                m_CpuUsageTotal = calcUsage(m_LastCpuTimes[0], cpuTimes[0]);
                m_CpuUsagePerCore.clear();
                for (size_t i = 1; i < cpuTimes.size(); ++i)
                {
                    m_CpuUsagePerCore.push_back(calcUsage(m_LastCpuTimes[i], cpuTimes[i]));
                }
            }
            else
            {
                m_CpuUsageTotal = 0.0f;
                m_CpuUsagePerCore.assign(cpuTimes.size() > 0 ? cpuTimes.size() - 1 : 0, 0.0f);
            }

            m_LastCpuTimes = cpuTimes;
            m_HasCpuSample = true;
        }
    }

    if (!m_HasCpuSample)
    {
        FILETIME idleTime;
        FILETIME kernelTime;
        FILETIME userTime;
        if (GetSystemTimes(&idleTime, &kernelTime, &userTime))
        {
            auto toUint64 = [](const FILETIME& ft) {
                return (static_cast<uint64_t>(ft.dwHighDateTime) << 32) | static_cast<uint64_t>(ft.dwLowDateTime);
            };

            std::vector<CpuTimes> cpuTimes(1);
            cpuTimes[0].idle = toUint64(idleTime);
            cpuTimes[0].total = toUint64(kernelTime) + toUint64(userTime);
            m_CpuUsagePerCore.assign((std::max<DWORD>)(GetActiveProcessorCount(ALL_PROCESSOR_GROUPS), 1), 0.0f);
            m_LastCpuTimes = cpuTimes;
            m_HasCpuSample = true;
        }
    }

    if (m_HasCpuSample && m_LastCpuTimes.size() == 1)
    {
        FILETIME idleTime;
        FILETIME kernelTime;
        FILETIME userTime;
        if (GetSystemTimes(&idleTime, &kernelTime, &userTime))
        {
            auto toUint64 = [](const FILETIME& ft) {
                return (static_cast<uint64_t>(ft.dwHighDateTime) << 32) | static_cast<uint64_t>(ft.dwLowDateTime);
            };

            std::vector<CpuTimes> cpuTimes(1);
            cpuTimes[0].idle = toUint64(idleTime);
            cpuTimes[0].total = toUint64(kernelTime) + toUint64(userTime);
            m_CpuUsageTotal = calcUsage(m_LastCpuTimes[0], cpuTimes[0]);
            m_LastCpuTimes = cpuTimes;
        }
    }

    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(memStatus);
    if (GlobalMemoryStatusEx(&memStatus))
    {
        m_RamTotalBytes = memStatus.ullTotalPhys;
        m_RamUsedBytes = memStatus.ullTotalPhys - memStatus.ullAvailPhys;
    }

    using GetProcessMemoryInfoFn = BOOL(WINAPI*)(HANDLE, PPROCESS_MEMORY_COUNTERS, DWORD);
    auto getProcessMemoryInfo = reinterpret_cast<GetProcessMemoryInfoFn>(
        GetProcAddress(GetModuleHandleA("kernel32.dll"), "K32GetProcessMemoryInfo"));
    HMODULE psapi = nullptr;
    if (!getProcessMemoryInfo)
    {
        psapi = LoadLibraryA("psapi.dll");
        getProcessMemoryInfo =
            psapi ? reinterpret_cast<GetProcessMemoryInfoFn>(GetProcAddress(psapi, "GetProcessMemoryInfo")) : nullptr;
    }

    if (getProcessMemoryInfo)
    {
        PROCESS_MEMORY_COUNTERS_EX counters = {};
        counters.cb = sizeof(counters);
        if (getProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PPROCESS_MEMORY_COUNTERS>(&counters),
                                 sizeof(counters)))
        {
            m_ProcessRamBytes = static_cast<uint64_t>(counters.WorkingSetSize);
        }
    }

    if (psapi)
        FreeLibrary(psapi);
#endif

    if (m_HasCpuSample)
    {
        const float alpha = GetSmoothingAlpha(statsInterval, SMOOTHING_WINDOW_SECONDS);
        if (hadCpuUsageSmoothSample)
        {
            m_CpuUsageTotal = previousCpuUsageTotal + (m_CpuUsageTotal - previousCpuUsageTotal) * alpha;
            if (previousCpuUsagePerCore.size() == m_CpuUsagePerCore.size())
            {
                for (size_t i = 0; i < m_CpuUsagePerCore.size(); ++i)
                {
                    m_CpuUsagePerCore[i] =
                        previousCpuUsagePerCore[i] + (m_CpuUsagePerCore[i] - previousCpuUsagePerCore[i]) * alpha;
                }
            }
        }
        m_HasCpuUsageSmoothSample = true;
    }

    auto& runtimeProfiler = RuntimeProfiler::Instance();
    runtimeProfiler.SetRamUsage(m_ProcessRamBytes);
    float gpuUsage = 0.0f;
    if (TryReadLinuxGpuUsage(gpuUsage))
    {
        runtimeProfiler.SetGpuUsage(gpuUsage);
    }

    uint64_t vramUsedBytes = 0;
    uint64_t vramTotalBytes = 0;
    auto* graphics = ServiceLocator::Instance().Resolve<IGraphicsContext>();
    if (TryReadLinuxVram(vramUsedBytes, vramTotalBytes) ||
        (graphics && graphics->TryGetMemoryBudget(vramUsedBytes, vramTotalBytes)))
    {
        runtimeProfiler.SetVramUsage(vramUsedBytes, vramTotalBytes);
    }
}

void ProfilerPanel::OnImGui(Scene& scene)
{
    ImGui::Begin(GetTitle().c_str(), &m_Open);

    ImGui::Text("Frame Time (ms)");
    char overlay[96];
    std::snprintf(overlay, sizeof(overlay), "%.2f ms now | %.2f ms 3s avg", m_FrameTime, m_AvgFrameTime);
    const float graphMax = (std::max)(m_GraphMaxFrameTime * 1.25f, 33.33f);
    ImGui::PlotLines("##FrameTime", m_FrameTimeHistory, FRAME_HISTORY_SIZE, m_HistoryOffset, overlay, 0.0f, graphMax,
                     ImVec2(-1, 110));

    ImGui::Separator();

    auto& reg = scene.GetRegistry();
    RenderPanelStats renderStats = CollectRenderStats(scene);
    ImGuiIO& io = ImGui::GetIO();
    const int uiDrawCalls = CountImGuiDrawCalls();
    const int uiTriangles = io.MetricsRenderIndices / 3;
    const RuntimeProfilerStats& runtimeStats = RuntimeProfiler::Instance().GetStats();
    const float cpuFrameMs = m_HasRuntimeSmoothSample
                                 ? m_SmoothedCpuFrameTime
                                 : (runtimeStats.cpuFrameMs > 0.0f ? runtimeStats.cpuFrameMs : m_CpuFrameTime);
    const float gpuFrameMs = m_HasRuntimeSmoothSample
                                 ? m_SmoothedGpuFrameTime
                                 : (runtimeStats.hasGpuFrameTime ? runtimeStats.gpuFrameMs : cpuFrameMs);
    const float gpuUsage =
        m_HasRuntimeSmoothSample
            ? m_SmoothedGpuUsage
            : (runtimeStats.hasGpuUsage || runtimeStats.hasGpuFrameTime ? runtimeStats.gpuUsagePercent : 0.0f);

    ImGui::Text("Overview");
    ImGui::Columns(2, "ProfilerStats", false);
    ImGui::SetColumnWidth(0, 170.0f);

    DrawMetric("FPS (3s avg)", "%.0f", m_Fps);
    DrawMetric("Frame Time (3s avg)", "%.2f ms", m_AvgFrameTime);
    DrawMetric("CPU Frame Time (3s)", "%.2f ms", cpuFrameMs);
    DrawMetric("GPU Frame Time (3s)", "%.2f ms%s", gpuFrameMs, runtimeStats.hasGpuFrameTime ? "" : " est.");
    DrawMetric("CPU p50 / p95 / p99", "%.2f / %.2f / %.2f ms", runtimeStats.cpuP50Ms,
               runtimeStats.cpuP95Ms, runtimeStats.cpuP99Ms);
    DrawMetric("GPU p50 / p95 / p99", "%.2f / %.2f / %.2f ms", runtimeStats.gpuP50Ms,
               runtimeStats.gpuP95Ms, runtimeStats.gpuP99Ms);
    DrawMetric("Frame Hitches", "%llu", static_cast<unsigned long long>(runtimeStats.frameHitchCount));
    DrawMetric("System CPU Usage", "%.1f%% total", m_CpuUsageTotal);
    DrawMetric("GPU Usage", "%.1f%%%s", gpuUsage, runtimeStats.hasGpuUsage ? "" : " est.");
    DrawMetricText("RAM Usage", FormatBytePair(m_RamUsedBytes, m_RamTotalBytes));
    DrawMetricText("VRAM Usage", FormatBytePair(runtimeStats.vramUsedBytes, runtimeStats.vramTotalBytes));
    DrawMetric("Draw Calls", "%d", renderStats.drawCalls);
    DrawMetric("Triangles", "%llu", static_cast<unsigned long long>(renderStats.triangles));
    DrawMetric("Visible Objects", "%zu", renderStats.visibleObjects);
    DrawMetric("Culled Objects", "%zu", renderStats.culledObjects);

    ImGui::Columns(1);

    ImGui::Separator();

    float budget = 16.67f;  // 60fps target
    float ratio = m_AvgFrameTime / budget;
    ImVec4 budgetColor = ratio < 0.8f   ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f)
                         : ratio < 1.0f ? ImVec4(1.0f, 1.0f, 0.2f, 1.0f)
                                        : ImVec4(1.0f, 0.3f, 0.3f, 1.0f);

    ImGui::TextColored(budgetColor, "Frame Budget: %.0f%% (target 60fps)", ratio * 100.0f);
    ImGui::ProgressBar((std::min)(ratio, 2.0f) / 2.0f, ImVec2(-1, 0), ratio < 1.0f ? "Under Budget" : "OVER BUDGET");

    if (ImGui::CollapsingHeader("Details"))
    {
        ImGui::Text("Frame");
        ImGui::Columns(2, "FrameDetailStats", false);
        ImGui::SetColumnWidth(0, 180.0f);
        DrawMetric("Current Frame Time", "%.2f ms", m_FrameTime);
        DrawMetric("Current FPS", "%.0f", m_RawFps);
        DrawMetric("Avg Frame Time", "%.2f ms", m_AvgFrameTime);
        DrawMetric("Min Frame Time", "%.2f ms", m_MinFrameTime < 999.0f ? m_MinFrameTime : 0.0f);
        DrawMetric("Max Frame Time", "%.2f ms", m_MaxFrameTime);
        DrawMetric("Avg FPS", "%.0f", m_AvgFrameTime > 0.01f ? 1000.0f / m_AvgFrameTime : 0.0f);
        DrawMetric("Render Target", "%dx%d", renderStats.width, renderStats.height);
        ImGui::Columns(1);

        ImGui::Separator();
        ImGui::Text("CPU Per Core");
        if (m_CpuUsagePerCore.empty())
        {
            ImGui::TextDisabled("0 cores sampled");
        }
        else
        {
            for (size_t i = 0; i < m_CpuUsagePerCore.size(); ++i)
            {
                char label[32];
                std::snprintf(label, sizeof(label), "Core %zu", i);
                ImGui::Text("%s", label);
                ImGui::SameLine(70.0f);
                float ratioCore = std::clamp(m_CpuUsagePerCore[i] / 100.0f, 0.0f, 1.0f);
                char value[32];
                std::snprintf(value, sizeof(value), "%.1f%%", m_CpuUsagePerCore[i]);
                ImGui::ProgressBar(ratioCore, ImVec2(-1, 0), value);
            }
        }

        ImGui::Separator();
        ImGui::Text("Memory");
        ImGui::Columns(2, "MemoryDetailStats", false);
        ImGui::SetColumnWidth(0, 180.0f);
        DrawMetricText("System RAM", FormatBytePair(m_RamUsedBytes, m_RamTotalBytes));
        DrawMetricText("Process RAM", FormatBytes(m_ProcessRamBytes));
        DrawMetricText("VRAM", FormatBytePair(runtimeStats.vramUsedBytes, runtimeStats.vramTotalBytes));
        DrawMetricText("Process VRAM", FormatBytePair(runtimeStats.vramUsedBytes, runtimeStats.vramTotalBytes));
        ImGui::Columns(1);

        ImGui::Separator();
        ImGui::Text("Render Queues");
        ImGui::Columns(2, "RenderQueueStats", false);
        ImGui::SetColumnWidth(0, 180.0f);
        DrawMetric("Queued Items", "%zu", renderStats.queuedItems);
        DrawMetric("Deferred Opaque", "%zu", renderStats.deferredOpaque);
        DrawMetric("Forward Opaque", "%zu", renderStats.forwardOpaque);
        DrawMetric("Transparent", "%zu", renderStats.transparent);
        DrawMetric("Depth Overlay", "%zu", renderStats.depthOverlay);
        DrawMetric("Shadow Items", "%zu", renderStats.shadowItems);
        DrawMetric("Shadow Triangles", "%llu", static_cast<unsigned long long>(renderStats.shadowTriangles));
        DrawMetric("Lights In Queue", "%zu", renderStats.lights);
        DrawMetric("UI Draw Calls", "%d", uiDrawCalls);
        DrawMetric("UI Triangles", "%d", uiTriangles);
        DrawMetric("UI Vertices", "%d", io.MetricsRenderVertices);
        ImGui::Columns(1);

        ImGui::Separator();
        ImGui::Text("Runtime Telemetry");
        ImGui::Columns(2, "RuntimeTelemetryStats", false);
        ImGui::SetColumnWidth(0, 180.0f);
        DrawMetric("Queue Builds", "%llu", static_cast<unsigned long long>(runtimeStats.queueBuildCount));
        DrawMetric("Submitted Draw Calls", "%llu", static_cast<unsigned long long>(runtimeStats.drawCalls));
        DrawMetric("Instanced Batches", "%llu", static_cast<unsigned long long>(runtimeStats.instancedBatches));
        DrawMetric("Submitted Triangles", "%llu", static_cast<unsigned long long>(runtimeStats.submittedTriangles));
        DrawMetric("Culled Entities", "%llu", static_cast<unsigned long long>(runtimeStats.culledEntities));
        DrawMetric("Graphics State Changes", "%llu", static_cast<unsigned long long>(runtimeStats.stateChanges));
        DrawMetric("Uniform Updates", "%llu", static_cast<unsigned long long>(runtimeStats.uniformUpdates));
        DrawMetricText("GPU Uploads", FormatBytes(runtimeStats.uploadBytes));
        DrawMetric("Transient Allocations", "%llu",
                   static_cast<unsigned long long>(runtimeStats.transientAllocations));
        DrawMetric("Job Wait", "%.2f ms", runtimeStats.jobWaitMs);
        ImGui::Columns(1);

        ImGui::Separator();
        ImGui::Text("Pass Times");
        ImGui::Columns(2, "PassTimeStats", false);
        ImGui::SetColumnWidth(0, 180.0f);
        DrawMetric("Input", "%.2f ms", m_SmoothedPassMs[static_cast<size_t>(ProfiledRenderPass::Input)]);
        DrawMetric("Resource Update", "%.2f ms",
                   m_SmoothedPassMs[static_cast<size_t>(ProfiledRenderPass::ResourceUpdate)]);
        DrawMetric("Game Update", "%.2f ms", m_SmoothedPassMs[static_cast<size_t>(ProfiledRenderPass::GameUpdate)]);
        DrawMetric("Fixed Update", "%.2f ms", m_SmoothedPassMs[static_cast<size_t>(ProfiledRenderPass::FixedUpdate)]);
        DrawMetric("Physics", "%.2f ms", m_SmoothedPassMs[static_cast<size_t>(ProfiledRenderPass::Physics)]);
        DrawMetric("Navigation", "%.2f ms", m_SmoothedPassMs[static_cast<size_t>(ProfiledRenderPass::Navigation)]);
        DrawMetric("Queue Build", "%.2f ms", m_SmoothedPassMs[static_cast<size_t>(ProfiledRenderPass::QueueBuild)]);
        DrawMetric("Shadow Pass Time", "%.2f ms", m_SmoothedPassMs[static_cast<size_t>(ProfiledRenderPass::Shadow)]);
        DrawMetric("Capture Pass Time", "%.2f ms", m_SmoothedPassMs[static_cast<size_t>(ProfiledRenderPass::Capture)]);
        DrawMetric("Geometry Pass Time", "%.2f ms",
                   m_SmoothedPassMs[static_cast<size_t>(ProfiledRenderPass::Geometry)]);
        DrawMetric("Lighting Pass Time", "%.2f ms",
                   m_SmoothedPassMs[static_cast<size_t>(ProfiledRenderPass::Lighting)]);
        DrawMetric("Alpha Pass Time", "%.2f ms", m_SmoothedPassMs[static_cast<size_t>(ProfiledRenderPass::Alpha)]);
        DrawMetric("Transparent Pass Time", "%.2f ms",
                   m_SmoothedPassMs[static_cast<size_t>(ProfiledRenderPass::Transparent)]);
        DrawMetric("Bloom Time", "%.2f ms", m_SmoothedPassMs[static_cast<size_t>(ProfiledRenderPass::Bloom)]);
        DrawMetric("Post Process Time", "%.2f ms",
                   m_SmoothedPassMs[static_cast<size_t>(ProfiledRenderPass::PostProcess)]);
        DrawMetric("UI", "%.2f ms", m_SmoothedPassMs[static_cast<size_t>(ProfiledRenderPass::UI)]);
        DrawMetric("Swap", "%.2f ms", m_SmoothedPassMs[static_cast<size_t>(ProfiledRenderPass::Swap)]);
        DrawMetric("Frame Limiter", "%.2f ms",
                   m_SmoothedPassMs[static_cast<size_t>(ProfiledRenderPass::FrameLimiter)]);
        ImGui::Columns(1);
        ImGui::TextDisabled(
            "Pass timings are CPU wall-clock times; GPU frame time uses OpenGL timer query when available.");

        ImGui::Separator();
        ImGui::Text("Scene");
        ImGui::Columns(2, "SceneDetailStats", false);
        ImGui::SetColumnWidth(0, 180.0f);
        DrawMetric("Entities", "%zu", reg.storage<entt::entity>().size());
        DrawMetric("Inactive Entities", "%zu", CountInactiveInfoEntities(reg));
        DrawMetric("Mesh Renderers", "%zu", reg.view<MeshRendererComponent>().size());
        DrawMetric("Materials", "%zu", reg.view<MaterialComponent>().size());
        DrawMetric("Physics Bodies", "%zu", reg.view<RigidBodyComponent>().size());
        DrawMetric("Colliders", "%zu", reg.view<RigidShapeComponent>().size());
        DrawMetric("UI Elements", "%zu", reg.view<UITransformComponent>().size());
        DrawMetric("Directional Lights", "%zu", reg.view<DirectionalLightComponent>().size());
        DrawMetric("Point Lights", "%zu", reg.view<PointLightComponent>().size());
        DrawMetric("Spot Lights", "%zu", reg.view<SpotLightComponent>().size());
        DrawMetric("Decals", "%zu", reg.view<DecalComponent>().size());
        DrawMetric("Terrains", "%zu", reg.view<TerrainComponent>().size());
        DrawMetric("Reflection Probes", "%zu", reg.view<ReflectionProbeComponent>().size());
        DrawMetric("Planar Reflections", "%zu", reg.view<PlanarReflectionComponent>().size());
        DrawMetric("Audio Sources", "%zu", reg.view<AudioSourceComponent>().size());
        DrawMetric("Video Players", "%zu", reg.view<VideoPlayerComponent>().size());
        DrawMetric("Particles", "%zu", reg.view<ParticleEmitterComponent>().size());
        DrawMetric("Post Process Volumes", "%zu", reg.view<PostProcessComponent>().size());
        ImGui::Columns(1);
    }

    ImGui::End();
}
#endif
