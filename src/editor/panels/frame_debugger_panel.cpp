#include <editor/panels/frame_debugger_panel.h>

#ifdef ENABLE_EDITOR

#include <core/logic/service_locator.h>
#include <imgui.h>
#include <algorithm>
#include <chrono>
#include <cctype>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

namespace
{
constexpr const char* kPassNames[] = {
    "Total frame", "Input", "Resource update", "Game update", "Fixed update", "Physics",
    "Navigation", "Queue build", "Shadow", "Capture", "Geometry", "Lighting", "Alpha",
    "Transparent", "Bloom", "Post process", "UI", "Swap", "Frame limiter"};

std::string SafePathPart(const char* value, const char* fallback)
{
    std::string result;
    for (const unsigned char character : std::string(value ? value : ""))
    {
        if (std::isalnum(character) || character == '_' || character == '-' || character == '.')
            result.push_back(static_cast<char>(character));
    }
    return result.empty() ? fallback : result;
}

RuntimeProfilerStats AverageStats(const std::deque<FrameDebuggerTimedSample>& samples)
{
    RuntimeProfilerStats average{};
    if (samples.empty())
        return average;
    const double count = static_cast<double>(samples.size());
    for (const auto& sample : samples)
    {
        const auto& value = sample.stats;
        average.cpuFrameMs += value.cpuFrameMs;
        average.gpuFrameMs += value.gpuFrameMs;
        average.gpuUsagePercent += value.gpuUsagePercent;
        average.jobWaitMs += value.jobWaitMs;
        average.drawCalls += value.drawCalls;
        average.submittedTriangles += value.submittedTriangles;
        average.stateChanges += value.stateChanges;
        average.uniformUpdates += value.uniformUpdates;
        for (size_t index = 0; index < average.passMs.size(); ++index)
            average.passMs[index] += value.passMs[index];
    }
    average.cpuFrameMs /= static_cast<float>(count);
    average.gpuFrameMs /= static_cast<float>(count);
    average.gpuUsagePercent /= static_cast<float>(count);
    average.jobWaitMs /= static_cast<float>(count);
    average.drawCalls = static_cast<uint64_t>(static_cast<double>(average.drawCalls) / count);
    average.submittedTriangles = static_cast<uint64_t>(static_cast<double>(average.submittedTriangles) / count);
    average.stateChanges = static_cast<uint64_t>(static_cast<double>(average.stateChanges) / count);
    average.uniformUpdates = static_cast<uint64_t>(static_cast<double>(average.uniformUpdates) / count);
    for (float& pass : average.passMs)
        pass /= static_cast<float>(count);
    average.hasGpuFrameTime = samples.back().stats.hasGpuFrameTime;
    average.hasGpuUsage = samples.back().stats.hasGpuUsage;
    return average;
}

bool ExportCapture(const RuntimeProfilerStats& stats, const std::vector<FrameDebugDrawCall>& drawCalls,
                   const char* folder, const char* name, std::string& status)
{
    try
    {
        const std::filesystem::path outputDirectory =
            std::filesystem::path("log") / SafePathPart(folder, "frame_debugger");
        std::filesystem::create_directories(outputDirectory);
        const auto now = std::chrono::system_clock::now();
        const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
                                      now.time_since_epoch()).count() % 1000;
        const std::time_t time = std::chrono::system_clock::to_time_t(now);
        std::tm local{};
#ifdef _WIN32
        localtime_s(&local, &time);
#else
        localtime_r(&time, &local);
#endif
        std::ostringstream fileName;
        fileName << SafePathPart(name, "capture") << '_' << std::put_time(&local, "%Y%m%d_%H%M%S") << '_'
                 << std::setw(3) << std::setfill('0') << milliseconds << ".csv";
        const std::filesystem::path outputPath = outputDirectory / fileName.str();
        std::ofstream output(outputPath);
        if (!output)
            throw std::runtime_error("cannot open output file");
        output << "metric,value\n"
               << "window_seconds,3\n"
               << "cpu_frame_ms," << stats.cpuFrameMs << '\n'
               << "gpu_frame_ms," << stats.gpuFrameMs << '\n'
               << "draw_calls," << stats.drawCalls << '\n'
               << "triangles," << stats.submittedTriangles << '\n'
               << "state_changes," << stats.stateChanges << '\n'
               << "uniform_updates," << stats.uniformUpdates << "\n\npass,time_ms\n";
        for (size_t index = 0; index < stats.passMs.size(); ++index)
            output << kPassNames[index] << ',' << stats.passMs[index] << '\n';
        output << "\nindex,pass,entity,shader,elements,resource\n";
        for (const auto& draw : drawCalls)
            output << draw.index << ',' << std::quoted(draw.pass) << ',' << draw.entityId << ',' << draw.shaderId
                   << ',' << draw.elementCount << ',' << std::quoted(draw.resource) << '\n';
        status = "Saved new capture: " + outputPath.generic_string();
        return true;
    }
    catch (const std::exception& exception)
    {
        status = std::string("Export failed: ") + exception.what();
        return false;
    }
}
}

void FrameDebuggerPanel::OnImGui(Scene&)
{
    ImGui::Begin(GetTitle().c_str(), &m_Open);
    auto* renderControl = ServiceLocator::Instance().Resolve<IRenderRuntimeControl>();
    if (renderControl)
    {
        renderControl->SetFrameDebugCapture(!m_Frozen);
        renderControl->SetFrameDebugDrawLimit(static_cast<uint64_t>((std::max)(0, m_DrawLimit)));
    }
    if (!m_Frozen)
    {
        const float delta = std::clamp(ImGui::GetIO().DeltaTime, 0.0001f, 0.25f);
        m_Samples.push_back({RuntimeProfiler::Instance().GetLastCompletedStats(), delta});
        m_SampleSeconds += delta;
        while (m_Samples.size() > 1 && m_SampleSeconds - m_Samples.front().seconds >= 3.0f)
        {
            m_SampleSeconds -= m_Samples.front().seconds;
            m_Samples.pop_front();
        }
        m_SmoothedStats = AverageStats(m_Samples);
    }
    if (ImGui::Button(m_Frozen ? "Resume live frame" : "Capture frame"))
    {
        m_Frozen = !m_Frozen;
        if (m_Frozen)
        {
            m_CapturedStats = m_SmoothedStats;
            if (auto* registry = ServiceLocator::Instance().Resolve<IPostProcessRegistry>())
                m_CapturedEffects = registry->GetRegisteredEffects();
            if (renderControl)
            {
                m_CapturedDrawCalls = renderControl->GetFrameDebugDrawCalls();
                renderControl->SetFrameDebugCapture(false);
            }
        }
    }

    const auto& stats = m_Frozen ? m_CapturedStats : m_SmoothedStats;
    ImGui::SameLine();
    ImGui::TextDisabled("3 s moving average (%zu samples)", m_Samples.size());
    ImGui::Text("Draw calls: %llu", static_cast<unsigned long long>(stats.drawCalls));
    ImGui::SameLine();
    ImGui::Text("Triangles: %llu", static_cast<unsigned long long>(stats.submittedTriangles));
    ImGui::Text("State changes: %llu", static_cast<unsigned long long>(stats.stateChanges));
    ImGui::SameLine();
    ImGui::Text("Uniform updates: %llu", static_cast<unsigned long long>(stats.uniformUpdates));

    if (ImGui::BeginTable("RenderPasses", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH |
                                                   ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn("Pass");
        ImGui::TableSetupColumn("Time");
        ImGui::TableSetupColumn("Frame share");
        ImGui::TableHeadersRow();
        const float total = stats.passMs[static_cast<size_t>(ProfiledRenderPass::TotalFrame)];
        for (size_t index = 1; index < static_cast<size_t>(ProfiledRenderPass::Count); ++index)
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(kPassNames[index]);
            ImGui::TableNextColumn();
            ImGui::Text("%.3f ms", stats.passMs[index]);
            ImGui::TableNextColumn();
            ImGui::ProgressBar(total > 0.0f ? stats.passMs[index] / total : 0.0f, ImVec2(-1.0f, 0.0f));
        }
        ImGui::EndTable();
    }

    ImGui::SeparatorText("Post-process graph");
    const auto liveEffects = ServiceLocator::Instance().Resolve<IPostProcessRegistry>();
    const std::vector<RegisteredPostProcessEffect> currentEffects =
        !m_Frozen && liveEffects ? liveEffects->GetRegisteredEffects() : std::vector<RegisteredPostProcessEffect>{};
    const auto& effects = m_Frozen ? m_CapturedEffects : currentEffects;
    for (const auto& effect : effects)
    {
        ImGui::PushID(static_cast<int>(effect.handle));
        ImGui::BulletText("[%d] %s / %s", effect.descriptor.priority, effect.descriptor.owner.c_str(),
                          effect.descriptor.name.c_str());
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Shader: %s\nAffect UI: %s", effect.descriptor.shaderName.c_str(),
                              effect.descriptor.affectUI ? "yes" : "no");
        ImGui::PopID();
    }

    ImGui::SeparatorText("Draw-call stream");
    ImGui::SetNextItemWidth(220.0f);
    if (ImGui::DragInt("Execute first N draws", &m_DrawLimit, 1.0f, 0, 100000))
        m_DrawLimit = (std::max)(0, m_DrawLimit);
    ImGui::SameLine();
    if (ImGui::Button("All draws"))
        m_DrawLimit = 0;
    ImGui::TextDisabled("0 executes the complete frame; a positive value stops model/command draws after N calls.");

    const std::vector<FrameDebugDrawCall> liveCalls =
        !m_Frozen && renderControl ? renderControl->GetFrameDebugDrawCalls() : std::vector<FrameDebugDrawCall>{};
    const auto& drawCalls = m_Frozen ? m_CapturedDrawCalls : liveCalls;
    if (ImGui::BeginTable("DrawCalls", 6, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders |
                                              ImGuiTableFlags_ScrollY, ImVec2(0.0f, 260.0f)))
    {
        ImGui::TableSetupColumn("#");
        ImGui::TableSetupColumn("Pass");
        ImGui::TableSetupColumn("Entity");
        ImGui::TableSetupColumn("Shader");
        ImGui::TableSetupColumn("Elements");
        ImGui::TableSetupColumn("Resource");
        ImGui::TableHeadersRow();
        for (size_t index = 0; index < drawCalls.size(); ++index)
        {
            const auto& drawCall = drawCalls[index];
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            const bool selected = m_SelectedDraw == static_cast<int>(index);
            if (ImGui::Selectable(std::to_string(drawCall.index).c_str(), selected,
                                  ImGuiSelectableFlags_SpanAllColumns))
                m_SelectedDraw = static_cast<int>(index);
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(drawCall.pass.c_str());
            ImGui::TableNextColumn();
            ImGui::Text("%u", drawCall.entityId);
            ImGui::TableNextColumn();
            ImGui::Text("%u", drawCall.shaderId);
            ImGui::TableNextColumn();
            ImGui::Text("%u", drawCall.elementCount);
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(drawCall.resource.c_str());
        }
        ImGui::EndTable();
    }

    ImGui::SeparatorText("Export capture log");
    ImGui::SetNextItemWidth(180.0f);
    ImGui::InputText("Folder under log/", m_LogFolder.data(), m_LogFolder.size());
    ImGui::SetNextItemWidth(180.0f);
    ImGui::InputText("File name", m_LogName.data(), m_LogName.size());
    if (ImGui::Button("Export new CSV"))
        ExportCapture(stats, drawCalls, m_LogFolder.data(), m_LogName.data(), m_LogStatus);
    ImGui::SameLine();
    ImGui::TextDisabled("A timestamp prevents overwriting previous captures.");
    if (!m_LogStatus.empty())
        ImGui::TextWrapped("%s", m_LogStatus.c_str());
    ImGui::End();
}

#endif
