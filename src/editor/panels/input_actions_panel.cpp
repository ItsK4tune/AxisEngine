#include <editor/panels/input_actions_panel.h>

#ifdef ENABLE_EDITOR

#include <core/logic/filesystem.h>
#include <core/logic/service_locator.h>
#include <core/logic/yaml_parser.h>
#include <platform/interface/gamepad.h>
#include <platform/interface/input_codes.h>
#include <platform/logic/input_serializer.h>
#include <platform/logic/io_handler.h>
#include <imgui.h>
#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

namespace
{
struct BindingOption
{
    const char* name;
    int code;
};

const char* InputTypeName(InputType type)
{
    switch (type)
    {
        case InputType::Key: return "Keyboard";
        case InputType::MouseButton: return "Mouse";
        case InputType::GamepadButton: return "Gamepad button";
        case InputType::GamepadAxis: return "Gamepad axis";
    }
    return "Unknown";
}

const std::vector<BindingOption>& OptionsFor(int type)
{
    static const std::vector<BindingOption> keyboard = [] {
        std::vector<BindingOption> values = {
            {"Space", static_cast<int>(Key::Space)}, {"Enter", static_cast<int>(Key::Enter)},
            {"Escape", static_cast<int>(Key::Escape)}, {"Tab", static_cast<int>(Key::Tab)},
            {"Left", static_cast<int>(Key::Left)}, {"Right", static_cast<int>(Key::Right)},
            {"Up", static_cast<int>(Key::Up)}, {"Down", static_cast<int>(Key::Down)},
            {"Left Shift", static_cast<int>(Key::LeftShift)}, {"Left Ctrl", static_cast<int>(Key::LeftControl)},
            {"Left Alt", static_cast<int>(Key::LeftAlt)}};
        static constexpr const char* letters[] = {
            "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M",
            "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z"};
        for (int index = 0; index < 26; ++index)
            values.push_back({letters[index], static_cast<int>(Key::A) + index});
        static constexpr const char* digits[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
        for (int index = 0; index < 10; ++index)
            values.push_back({digits[index], static_cast<int>(Key::_0) + index});
        static constexpr const char* functions[] = {
            "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12"};
        for (int index = 0; index < 12; ++index)
            values.push_back({functions[index], static_cast<int>(Key::F1) + index});
        return values;
    }();
    static const std::vector<BindingOption> mouse = {
        {"Left button", static_cast<int>(Mouse::Left)}, {"Right button", static_cast<int>(Mouse::Right)},
        {"Middle button", static_cast<int>(Mouse::Middle)}, {"Wheel up", static_cast<int>(Mouse::WheelUp)},
        {"Wheel down", static_cast<int>(Mouse::WheelDown)}};
    static const std::vector<BindingOption> gamepadButtons = {
        {"A / Cross", static_cast<int>(Gamepad::ButtonA)}, {"B / Circle", static_cast<int>(Gamepad::ButtonB)},
        {"X / Square", static_cast<int>(Gamepad::ButtonX)}, {"Y / Triangle", static_cast<int>(Gamepad::ButtonY)},
        {"Left bumper", static_cast<int>(Gamepad::ButtonLeftBumper)},
        {"Right bumper", static_cast<int>(Gamepad::ButtonRightBumper)},
        {"Back", static_cast<int>(Gamepad::ButtonBack)}, {"Start", static_cast<int>(Gamepad::ButtonStart)},
        {"Left stick", static_cast<int>(Gamepad::ButtonLeftThumb)},
        {"Right stick", static_cast<int>(Gamepad::ButtonRightThumb)},
        {"D-pad up", static_cast<int>(Gamepad::ButtonDpadUp)},
        {"D-pad right", static_cast<int>(Gamepad::ButtonDpadRight)},
        {"D-pad down", static_cast<int>(Gamepad::ButtonDpadDown)},
        {"D-pad left", static_cast<int>(Gamepad::ButtonDpadLeft)}};
    static const std::vector<BindingOption> gamepadAxes = {
        {"Left stick X", static_cast<int>(GamepadAxis::LeftX)},
        {"Left stick Y", static_cast<int>(GamepadAxis::LeftY)},
        {"Right stick X", static_cast<int>(GamepadAxis::RightX)},
        {"Right stick Y", static_cast<int>(GamepadAxis::RightY)},
        {"Left trigger", static_cast<int>(GamepadAxis::LeftTrigger)},
        {"Right trigger", static_cast<int>(GamepadAxis::RightTrigger)}};
    if (type == static_cast<int>(InputType::MouseButton))
        return mouse;
    if (type == static_cast<int>(InputType::GamepadButton))
        return gamepadButtons;
    if (type == static_cast<int>(InputType::GamepadAxis))
        return gamepadAxes;
    return keyboard;
}

std::string BindingName(InputType type, int code)
{
    for (const auto& option : OptionsFor(static_cast<int>(type)))
        if (option.code == code)
            return option.name;
    return "Code " + std::to_string(code);
}

bool ContainsCaseInsensitive(const std::string& value, const char* filter)
{
    if (!filter || *filter == '\0')
        return true;
    std::string lhs = value;
    std::string rhs = filter;
    std::transform(lhs.begin(), lhs.end(), lhs.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    std::transform(rhs.begin(), rhs.end(), rhs.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return lhs.find(rhs) != std::string::npos;
}

void CopyPath(std::array<char, 260>& destination, const std::string& value)
{
    destination.fill('\0');
    std::strncpy(destination.data(), value.c_str(), destination.size() - 1);
}

bool IsInputBindingAsset(const std::filesystem::path& path)
{
    if (path.extension() != ".axs")
        return false;
    for (const auto& root : YAMLParser::Parse(path.generic_string()))
        if (root.key == "axis_input" || root.key == "Bindings")
            return true;
    return false;
}

std::vector<std::string> DiscoverInputAssets()
{
    std::vector<std::string> result;
    const std::filesystem::path root = FileSystem::getPath("assets");
    std::error_code error;
    if (!std::filesystem::exists(root, error))
        return result;
    for (std::filesystem::recursive_directory_iterator iterator(
             root, std::filesystem::directory_options::skip_permission_denied, error), end;
         iterator != end; iterator.increment(error))
    {
        if (error)
        {
            error.clear();
            continue;
        }
        if (iterator->is_regular_file(error) && IsInputBindingAsset(iterator->path()))
            result.push_back(FileSystem::getRelativePath(iterator->path().generic_string()));
    }
    std::sort(result.begin(), result.end());
    return result;
}

bool PrepareOutputPath(const std::string& path)
{
    const std::filesystem::path resolved = FileSystem::getPath(path);
    if (resolved.extension() != ".axs")
        return false;
    std::error_code error;
    if (!resolved.parent_path().empty())
        std::filesystem::create_directories(resolved.parent_path(), error);
    return !error;
}
}

void InputActionsPanel::OnImGui(Scene&)
{
    ImGui::Begin(GetTitle().c_str(), &m_Open);
    auto* io = ServiceLocator::Instance().Resolve<IOHandler>();
    if (!io)
    {
        ImGui::TextDisabled("Input service unavailable");
        ImGui::End();
        return;
    }
    auto& input = io->GetInputManager();

    ImGui::SeparatorText("Binding asset");
    ImGui::Text("Current: %s", m_SavePath.data());
    if (ImGui::Button("New..."))
    {
        CopyPath(m_DraftPath, "assets/input/new_input.axs");
        ImGui::OpenPopup("New input bindings");
    }
    ImGui::SameLine();
    if (ImGui::Button("Open..."))
    {
        m_DiscoveredFiles = DiscoverInputAssets();
        ImGui::OpenPopup("Open input bindings");
    }
    ImGui::SameLine();
    if (ImGui::Button("Save"))
    {
        InputSerializer serializer;
        m_Status = PrepareOutputPath(m_SavePath.data()) && serializer.Serialize(m_SavePath.data(), input)
                       ? "Bindings saved to " + std::string(m_SavePath.data()) + "."
                       : "Save failed. Check the .axs path.";
    }
    ImGui::SameLine();
    if (ImGui::Button("Save As..."))
    {
        CopyPath(m_DraftPath, m_SavePath.data());
        m_AllowOverwrite = false;
        ImGui::OpenPopup("Save input bindings as");
    }

    if (ImGui::BeginPopupModal("New input bindings", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::InputText("New file (.axs)", m_DraftPath.data(), m_DraftPath.size());
        const std::filesystem::path resolved = FileSystem::getPath(m_DraftPath.data());
        const bool valid = resolved.extension() == ".axs";
        const bool exists = std::filesystem::exists(resolved);
        if (exists)
            ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.25f, 1.0f), "File exists. Use Open or Save As.");
        ImGui::BeginDisabled(!valid || exists);
        if (ImGui::Button("Create empty"))
        {
            InputSerializer serializer;
            if (PrepareOutputPath(m_DraftPath.data()))
            {
                input.FlushBindings();
                if (serializer.Serialize(m_DraftPath.data(), input))
                {
                    CopyPath(m_SavePath, m_DraftPath.data());
                    m_Status = "Created " + std::string(m_SavePath.data()) + ".";
                    ImGui::CloseCurrentPopup();
                }
                else
                    m_Status = "Could not create the input asset.";
            }
            else
                m_Status = "Could not create the input asset.";
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal("Open input bindings", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::BeginChild("InputAssetFiles", ImVec2(520.0f, 260.0f), true);
        if (m_DiscoveredFiles.empty())
            ImGui::TextDisabled("No axis_input .axs files found under assets.");
        for (const std::string& file : m_DiscoveredFiles)
        {
            if (!ImGui::Selectable(file.c_str()))
                continue;
            InputSerializer serializer;
            if (serializer.Deserialize(file, input))
            {
                CopyPath(m_SavePath, file);
                m_Status = "Opened " + file + ".";
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndChild();
        if (ImGui::Button("Refresh"))
            m_DiscoveredFiles = DiscoverInputAssets();
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal("Save input bindings as", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::InputText("Save path (.axs)", m_DraftPath.data(), m_DraftPath.size());
        const std::filesystem::path resolved = FileSystem::getPath(m_DraftPath.data());
        const bool exists = std::filesystem::exists(resolved);
        const bool valid = resolved.extension() == ".axs";
        if (exists)
            ImGui::Checkbox("Overwrite existing file", &m_AllowOverwrite);
        ImGui::BeginDisabled(!valid || (exists && !m_AllowOverwrite));
        if (ImGui::Button("Save copy"))
        {
            InputSerializer serializer;
            if (PrepareOutputPath(m_DraftPath.data()) && serializer.Serialize(m_DraftPath.data(), input))
            {
                CopyPath(m_SavePath, m_DraftPath.data());
                m_Status = "Saved as " + std::string(m_SavePath.data()) + ".";
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    ImGui::SeparatorText("Add binding");
    ImGui::SetNextItemWidth(220.0f);
    ImGui::InputTextWithHint("Action", "e.g. PlayerJump", m_ActionName.data(), m_ActionName.size());
    const char* types[] = {"Keyboard", "Mouse", "Gamepad button", "Gamepad axis"};
    ImGui::SetNextItemWidth(180.0f);
    if (ImGui::Combo("Device", &m_InputType, types, IM_ARRAYSIZE(types)))
        m_Code = OptionsFor(m_InputType).front().code;

    ImGui::SameLine();
    ImGui::Checkbox("Advanced code", &m_AdvancedCode);
    if (m_AdvancedCode)
    {
        ImGui::SetNextItemWidth(160.0f);
        ImGui::InputInt("Input code", &m_Code);
    }
    else
    {
        const auto& options = OptionsFor(m_InputType);
        std::string preview = BindingName(static_cast<InputType>(m_InputType), m_Code);
        ImGui::SetNextItemWidth(220.0f);
        if (ImGui::BeginCombo("Control", preview.c_str()))
        {
            for (const auto& option : options)
                if (ImGui::Selectable(option.name, m_Code == option.code))
                    m_Code = option.code;
            ImGui::EndCombo();
        }
    }
    if (ImGui::Button("Add to action", ImVec2(150.0f, 0.0f)) && m_ActionName[0] != '\0')
    {
        input.BindAction(m_ActionName.data(), static_cast<InputType>(m_InputType), m_Code);
        m_Status = "Binding added to " + std::string(m_ActionName.data()) + ".";
    }

    ImGui::SeparatorText("Actions");
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputTextWithHint("##ActionFilter", "Filter actions...", m_Filter.data(), m_Filter.size());
    std::vector<std::string> actionNames;
    actionNames.reserve(input.GetActionMap().size());
    for (const auto& [name, binding] : input.GetActionMap())
        if (ContainsCaseInsensitive(name, m_Filter.data()))
            actionNames.push_back(name);
    std::sort(actionNames.begin(), actionNames.end());

    if (ImGui::BeginTable("InputActions", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders |
                                               ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY,
                          ImVec2(0.0f, 280.0f)))
    {
        ImGui::TableSetupColumn("State", ImGuiTableColumnFlags_WidthFixed, 55.0f);
        ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Bindings", ImGuiTableColumnFlags_WidthStretch, 2.0f);
        ImGui::TableSetupColumn("Manage", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableHeadersRow();
        for (const std::string& actionName : actionNames)
        {
            const auto action = input.GetActionMap().find(actionName);
            if (action == input.GetActionMap().end())
                continue;
            ImGui::PushID(actionName.c_str());
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextColored(input.GetAction(actionName) ? ImVec4(0.2f, 1.0f, 0.35f, 1.0f)
                                                           : ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
                               "%s", input.GetAction(actionName) ? "ON" : "off");
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(actionName.c_str());
            ImGui::TableNextColumn();
            size_t removeIndex = static_cast<size_t>(-1);
            for (size_t index = 0; index < action->second.bindings.size(); ++index)
            {
                const auto& binding = action->second.bindings[index];
                if (index != 0)
                    ImGui::SameLine();
                ImGui::PushID(static_cast<int>(index));
                const std::string label = std::string(InputTypeName(binding.type)) + ": " +
                                          BindingName(binding.type, binding.code) + "  x";
                if (ImGui::SmallButton(label.c_str()))
                    removeIndex = index;
                ImGui::PopID();
            }
            if (removeIndex != static_cast<size_t>(-1))
                input.RemoveBinding(actionName, removeIndex);
            ImGui::TableNextColumn();
            if (ImGui::SmallButton("Delete"))
                input.UnbindAction(actionName);
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    ImGui::TextDisabled("Click a binding chip to remove it. Advanced code keeps uncommon/platform-specific inputs available.");
    if (!m_Status.empty())
        ImGui::TextWrapped("%s", m_Status.c_str());
    ImGui::End();
}

#endif
