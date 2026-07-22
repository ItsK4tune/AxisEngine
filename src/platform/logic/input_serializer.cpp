#include <platform/logic/input_serializer.h>
#include <core/logic/filesystem.h>
#include <core/logic/logger.h>
#include <core/logic/yaml_parser.h>
#include <core/logic/yaml_writer.h>
#include <platform/interface/input_codes.h>
#include <platform/logic/input_manager.h>
#include <unordered_map>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <tuple>
#include <mutex>

namespace
{
std::string ToLower(const std::string& str)
{
    std::string lowerStr = str;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(),
                   [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
    return lowerStr;
}

void Trim(std::string& str)
{
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
    {
        str.clear();
        return;
    }

    size_t end = str.find_last_not_of(" \t\r\n");
    str = str.substr(start, end - start + 1);
}
}  // namespace

bool InputSerializer::Deserialize(const std::string& filepath, InputManager& inputManager)
{
    auto roots = YAMLParser::Parse(FileSystem::getPath(filepath));

    if (roots.empty())
    {
        LOGGER_ERROR("InputSerializer") << "Failed to parse or missing file: " << filepath;
        return false;
    }

    for (auto& root : roots)
    {
        if (root.key.rfind("axis_", 0) == 0 && root.key != "axis_input" && root.key != "axis_scene")
        {
            LOGGER_WARN("InputSerializer")
                << "Potential typo in root key: '" << root.key << "', expected 'axis_input' in " << filepath;
        }
    }

    YAMLNode* bindingsNode = nullptr;

    for (auto& root : roots)
    {
        if (root.key == "axis_input")
        {
            bindingsNode = root.GetChild("Bindings");
            break;
        }
        if (root.key == "Bindings")
        {
            bindingsNode = &root;
            break;
        }
    }

    if (!bindingsNode)
    {
        LOGGER_ERROR("InputSerializer") << "Invalid format in " << filepath << ": Missing axis_input/Bindings root.";
        return false;
    }

    static const std::unordered_map<std::string, Key> keyMap = {{"space", Key::Space},
                                                                {"apostrophe", Key::Apostrophe},
                                                                {"w", Key::W},
                                                                {"a", Key::A},
                                                                {"s", Key::S},
                                                                {"d", Key::D},
                                                                {"e", Key::E},
                                                                {"f", Key::F},
                                                                {"g", Key::G},
                                                                {"h", Key::H},
                                                                {"escape", Key::Escape},
                                                                {"enter", Key::Enter},
                                                                {"leftshift", Key::LeftShift},
                                                                {"leftctrl", Key::LeftControl},
                                                                {"leftcontrol", Key::LeftControl},
                                                                {"up", Key::Up},
                                                                {"down", Key::Down},
                                                                {"left", Key::Left},
                                                                {"right", Key::Right},
                                                                {"i", Key::I},
                                                                {"j", Key::J},
                                                                {"k", Key::K},
                                                                {"l", Key::L},
                                                                {"o", Key::O},
                                                                {"p", Key::P},
                                                                {"q", Key::Q},
                                                                {"r", Key::R},
                                                                {"t", Key::T},
                                                                {"u", Key::U},
                                                                {"z", Key::Z},
                                                                {"x", Key::X},
                                                                {"y", Key::Y},
                                                                {"c", Key::C},
                                                                {"v", Key::V},
                                                                {"b", Key::B},
                                                                {"n", Key::N},
                                                                {"m", Key::M},
                                                                {"0", Key::_0},
                                                                {"1", Key::_1},
                                                                {"2", Key::_2},
                                                                {"3", Key::_3},
                                                                {"4", Key::_4},
                                                                {"5", Key::_5},
                                                                {"6", Key::_6},
                                                                {"7", Key::_7},
                                                                {"8", Key::_8},
                                                                {"9", Key::_9},
                                                                {"f1", Key::F1},
                                                                {"f2", Key::F2},
                                                                {"f3", Key::F3},
                                                                {"f4", Key::F4},
                                                                {"f5", Key::F5},
                                                                {"f6", Key::F6},
                                                                {"f7", Key::F7},
                                                                {"f8", Key::F8},
                                                                {"f9", Key::F9},
                                                                {"f10", Key::F10},
                                                                {"f11", Key::F11},
                                                                {"f12", Key::F12},
                                                                {"f13", Key::F13},
                                                                {"f14", Key::F14},
                                                                {"f15", Key::F15},
                                                                {"f16", Key::F16},
                                                                {"f17", Key::F17},
                                                                {"f18", Key::F18},
                                                                {"f19", Key::F19},
                                                                {"f20", Key::F20},
                                                                {"f21", Key::F21},
                                                                {"f22", Key::F22},
                                                                {"f23", Key::F23},
                                                                {"f24", Key::F24},
                                                                {"f25", Key::F25},
                                                                {"minus", Key::Minus},
                                                                {"equal", Key::Equal},
                                                                {"equals", Key::Equal},
                                                                {"plus", Key::KpAdd},
                                                                {"comma", Key::Comma},
                                                                {"period", Key::Period},
                                                                {"slash", Key::Slash},
                                                                {"backslash", Key::Backslash},
                                                                {"semicolon", Key::Semicolon},
                                                                {"leftbracket", Key::LeftBracket},
                                                                {"rightbracket", Key::RightBracket},
                                                                {"graveaccent", Key::GraveAccent},
                                                                {"world1", Key::World1},
                                                                {"world2", Key::World2},
                                                                {"tab", Key::Tab},
                                                                {"backspace", Key::Backspace},
                                                                {"insert", Key::Insert},
                                                                {"delete", Key::Delete},
                                                                {"pageup", Key::PageUp},
                                                                {"pagedown", Key::PageDown},
                                                                {"home", Key::Home},
                                                                {"end", Key::End},
                                                                {"capslock", Key::CapsLock},
                                                                {"scrolllock", Key::ScrollLock},
                                                                {"numlock", Key::NumLock},
                                                                {"printscreen", Key::PrintScreen},
                                                                {"pause", Key::Pause},
                                                                {"kp0", Key::Kp0},
                                                                {"kp1", Key::Kp1},
                                                                {"kp2", Key::Kp2},
                                                                {"kp3", Key::Kp3},
                                                                {"kp4", Key::Kp4},
                                                                {"kp5", Key::Kp5},
                                                                {"kp6", Key::Kp6},
                                                                {"kp7", Key::Kp7},
                                                                {"kp8", Key::Kp8},
                                                                {"kp9", Key::Kp9},
                                                                {"kpdecimal", Key::KpDecimal},
                                                                {"kpdivide", Key::KpDivide},
                                                                {"kpmultiply", Key::KpMultiply},
                                                                {"kpsubtract", Key::KpSubtract},
                                                                {"kpadd", Key::KpAdd},
                                                                {"kpenter", Key::KpEnter},
                                                                {"kpequal", Key::KpEqual},
                                                                {"leftalt", Key::LeftAlt},
                                                                {"leftsuper", Key::LeftSuper},
                                                                {"rightshift", Key::RightShift},
                                                                {"rightctrl", Key::RightControl},
                                                                {"rightcontrol", Key::RightControl},
                                                                {"rightalt", Key::RightAlt},
                                                                {"rightsuper", Key::RightSuper},
                                                                {"menu", Key::Menu}};

    static const std::unordered_map<std::string, Mouse> mouseMap = {
        {"left", Mouse::Left},          {"button1", Mouse::Button1}, {"right", Mouse::Right},
        {"button2", Mouse::Button2},    {"middle", Mouse::Middle},   {"button3", Mouse::Button3},
        {"button4", Mouse::Button4},    {"button5", Mouse::Button5}, {"button6", Mouse::Button6},
        {"button7", Mouse::Button7},    {"button8", Mouse::Button8}, {"wheelup", Mouse::WheelUp},
        {"wheeldown", Mouse::WheelDown}};

    static const std::unordered_map<std::string, Gamepad> gamepadMap = {
        {"buttona", Gamepad::ButtonA},
        {"cross", Gamepad::Cross},
        {"buttonb", Gamepad::ButtonB},
        {"circle", Gamepad::Circle},
        {"buttonx", Gamepad::ButtonX},
        {"square", Gamepad::Square},
        {"buttony", Gamepad::ButtonY},
        {"triangle", Gamepad::Triangle},
        {"buttondpadup", Gamepad::ButtonDpadUp},
        {"buttondpaddown", Gamepad::ButtonDpadDown},
        {"buttondpadleft", Gamepad::ButtonDpadLeft},
        {"buttondpadright", Gamepad::ButtonDpadRight},
        {"buttonrightbumper", Gamepad::ButtonRightBumper},
        {"buttonleftbumper", Gamepad::ButtonLeftBumper},
        {"buttonrightthumb", Gamepad::ButtonRightThumb},
        {"buttonleftthumb", Gamepad::ButtonLeftThumb},
        {"buttonstart", Gamepad::ButtonStart},
        {"buttonback", Gamepad::ButtonBack},
        {"buttonguide", Gamepad::ButtonGuide}};
    static const std::unordered_map<std::string, GamepadAxis> gamepadAxisMap = {
        {"leftx", GamepadAxis::LeftX}, {"lefty", GamepadAxis::LeftY},
        {"rightx", GamepadAxis::RightX}, {"righty", GamepadAxis::RightY},
        {"lefttrigger", GamepadAxis::LeftTrigger}, {"righttrigger", GamepadAxis::RightTrigger}};

    struct PendingBinding
    {
        std::string actionName;
        InputType type;
        int code;
    };
    std::vector<PendingBinding> pendingBindings;

    for (auto& actionNode : bindingsNode->children)
    {
        std::string actionName = actionNode.key;

        for (auto& trigger : actionNode.children)
        {
            std::string triggerKey = trigger.key;
            std::string triggerValue = trigger.value;

            if (triggerKey.rfind("- ", 0) == 0)
            {
                triggerKey = triggerKey.substr(2);
                Trim(triggerKey);
            }

            if (triggerKey == "-")
            {
                auto colonPos = triggerValue.find(':');
                if (colonPos != std::string::npos)
                {
                    triggerKey = triggerValue.substr(0, colonPos);
                    triggerValue = triggerValue.substr(colonPos + 1);

                    Trim(triggerKey);
                    Trim(triggerValue);
                }
            }

            if (triggerKey == "Key")
            {
                std::string keyName = ToLower(triggerValue);
                if (keyMap.count(keyName))
                {
                    pendingBindings.push_back({actionName, InputType::Key, (int)keyMap.at(keyName)});
                }
                else
                {
                    LOGGER_WARN("InputSerializer") << "Unknown Key: " << keyName << " for action " << actionName;
                }
            }
            else if (triggerKey == "Mouse")
            {
                std::string btnName = ToLower(triggerValue);
                if (mouseMap.count(btnName))
                {
                    pendingBindings.push_back({actionName, InputType::MouseButton, (int)mouseMap.at(btnName)});
                }
                else
                {
                    LOGGER_WARN("InputSerializer") << "Unknown Mouse key: " << btnName << " for action " << actionName;
                }
            }
            else if (triggerKey == "Gamepad")
            {
                std::string padName = ToLower(triggerValue);
                if (gamepadMap.count(padName))
                {
                    pendingBindings.push_back({actionName, InputType::GamepadButton, (int)gamepadMap.at(padName)});
                }
                else
                {
                    LOGGER_WARN("InputSerializer")
                        << "Unknown Gamepad button: " << padName << " for action " << actionName;
                }
            }
            else if (triggerKey == "GamepadAxis")
            {
                const std::string axisName = ToLower(triggerValue);
                if (const auto axis = gamepadAxisMap.find(axisName); axis != gamepadAxisMap.end())
                    pendingBindings.push_back({actionName, InputType::GamepadAxis, static_cast<int>(axis->second)});
                else
                    LOGGER_WARN("InputSerializer") << "Unknown gamepad axis: " << axisName << " for action "
                                                    << actionName;
            }
        }
    }

    if (pendingBindings.empty())
    {
        LOGGER_ERROR("InputSerializer") << "No valid input bindings found in " << filepath;
        return false;
    }

    inputManager.FlushBindings();
    for (const auto& binding : pendingBindings)
    {
        inputManager.BindAction(binding.actionName, binding.type, binding.code);
    }

    LOGGER_INFO("InputSerializer") << "Successfully loaded input bindings from " << filepath;
    return true;
}

bool InputSerializer::Serialize(const std::string& filepath, const InputManager& inputManager)
{
    std::string fullPath = FileSystem::getPath(filepath);
    std::ofstream f(fullPath);
    if (!f.is_open())
    {
        LOGGER_ERROR("InputSerializer") << "Failed to open file for writing: " << filepath;
        return false;
    }

    static std::unordered_map<int, std::string> keyToStringMap;
    static std::unordered_map<int, std::string> mouseToStringMap;
    static std::unordered_map<int, std::string> gamepadToStringMap;
    static std::unordered_map<int, std::string> gamepadAxisToStringMap;
    static std::once_flag reverseMapsOnce;

    std::call_once(reverseMapsOnce, [&] {
        keyToStringMap[(int)Key::Space] = "space";
        keyToStringMap[(int)Key::Apostrophe] = "apostrophe";
        keyToStringMap[(int)Key::W] = "w";
        keyToStringMap[(int)Key::A] = "a";
        keyToStringMap[(int)Key::S] = "s";
        keyToStringMap[(int)Key::D] = "d";
        keyToStringMap[(int)Key::E] = "e";
        keyToStringMap[(int)Key::F] = "f";
        keyToStringMap[(int)Key::G] = "g";
        keyToStringMap[(int)Key::H] = "h";
        keyToStringMap[(int)Key::Escape] = "escape";
        keyToStringMap[(int)Key::Enter] = "enter";
        keyToStringMap[(int)Key::LeftShift] = "leftshift";
        keyToStringMap[(int)Key::LeftControl] = "leftctrl";
        keyToStringMap[(int)Key::Up] = "up";
        keyToStringMap[(int)Key::Down] = "down";
        keyToStringMap[(int)Key::Left] = "left";
        keyToStringMap[(int)Key::Right] = "right";
        keyToStringMap[(int)Key::I] = "i";
        keyToStringMap[(int)Key::J] = "j";
        keyToStringMap[(int)Key::K] = "k";
        keyToStringMap[(int)Key::L] = "l";
        keyToStringMap[(int)Key::O] = "o";
        keyToStringMap[(int)Key::P] = "p";
        keyToStringMap[(int)Key::Q] = "q";
        keyToStringMap[(int)Key::R] = "r";
        keyToStringMap[(int)Key::T] = "t";
        keyToStringMap[(int)Key::U] = "u";
        keyToStringMap[(int)Key::Z] = "z";
        keyToStringMap[(int)Key::X] = "x";
        keyToStringMap[(int)Key::Y] = "y";
        keyToStringMap[(int)Key::C] = "c";
        keyToStringMap[(int)Key::V] = "v";
        keyToStringMap[(int)Key::B] = "b";
        keyToStringMap[(int)Key::N] = "n";
        keyToStringMap[(int)Key::M] = "m";
        keyToStringMap[(int)Key::_0] = "0";
        keyToStringMap[(int)Key::_1] = "1";
        keyToStringMap[(int)Key::_2] = "2";
        keyToStringMap[(int)Key::_3] = "3";
        keyToStringMap[(int)Key::_4] = "4";
        keyToStringMap[(int)Key::_5] = "5";
        keyToStringMap[(int)Key::_6] = "6";
        keyToStringMap[(int)Key::_7] = "7";
        keyToStringMap[(int)Key::_8] = "8";
        keyToStringMap[(int)Key::_9] = "9";
        keyToStringMap[(int)Key::F1] = "f1";
        keyToStringMap[(int)Key::F2] = "f2";
        keyToStringMap[(int)Key::F3] = "f3";
        keyToStringMap[(int)Key::F4] = "f4";
        keyToStringMap[(int)Key::F5] = "f5";
        keyToStringMap[(int)Key::F6] = "f6";
        keyToStringMap[(int)Key::F7] = "f7";
        keyToStringMap[(int)Key::F8] = "f8";
        keyToStringMap[(int)Key::F9] = "f9";
        keyToStringMap[(int)Key::F10] = "f10";
        keyToStringMap[(int)Key::F11] = "f11";
        keyToStringMap[(int)Key::F12] = "f12";
        keyToStringMap[(int)Key::F13] = "f13";
        keyToStringMap[(int)Key::F14] = "f14";
        keyToStringMap[(int)Key::F15] = "f15";
        keyToStringMap[(int)Key::F16] = "f16";
        keyToStringMap[(int)Key::F17] = "f17";
        keyToStringMap[(int)Key::F18] = "f18";
        keyToStringMap[(int)Key::F19] = "f19";
        keyToStringMap[(int)Key::F20] = "f20";
        keyToStringMap[(int)Key::F21] = "f21";
        keyToStringMap[(int)Key::F22] = "f22";
        keyToStringMap[(int)Key::F23] = "f23";
        keyToStringMap[(int)Key::F24] = "f24";
        keyToStringMap[(int)Key::F25] = "f25";
        keyToStringMap[(int)Key::Minus] = "minus";
        keyToStringMap[(int)Key::Equal] = "equal";
        keyToStringMap[(int)Key::KpAdd] = "plus";
        keyToStringMap[(int)Key::Comma] = "comma";
        keyToStringMap[(int)Key::Period] = "period";
        keyToStringMap[(int)Key::Slash] = "slash";
        keyToStringMap[(int)Key::Backslash] = "backslash";
        keyToStringMap[(int)Key::Semicolon] = "semicolon";
        keyToStringMap[(int)Key::LeftBracket] = "leftbracket";
        keyToStringMap[(int)Key::RightBracket] = "rightbracket";
        keyToStringMap[(int)Key::GraveAccent] = "graveaccent";
        keyToStringMap[(int)Key::World1] = "world1";
        keyToStringMap[(int)Key::World2] = "world2";
        keyToStringMap[(int)Key::Tab] = "tab";
        keyToStringMap[(int)Key::Backspace] = "backspace";
        keyToStringMap[(int)Key::Insert] = "insert";
        keyToStringMap[(int)Key::Delete] = "delete";
        keyToStringMap[(int)Key::PageUp] = "pageup";
        keyToStringMap[(int)Key::PageDown] = "pagedown";
        keyToStringMap[(int)Key::Home] = "home";
        keyToStringMap[(int)Key::End] = "end";
        keyToStringMap[(int)Key::CapsLock] = "capslock";
        keyToStringMap[(int)Key::ScrollLock] = "scrolllock";
        keyToStringMap[(int)Key::NumLock] = "numlock";
        keyToStringMap[(int)Key::PrintScreen] = "printscreen";
        keyToStringMap[(int)Key::Pause] = "pause";
        keyToStringMap[(int)Key::Kp0] = "kp0";
        keyToStringMap[(int)Key::Kp1] = "kp1";
        keyToStringMap[(int)Key::Kp2] = "kp2";
        keyToStringMap[(int)Key::Kp3] = "kp3";
        keyToStringMap[(int)Key::Kp4] = "kp4";
        keyToStringMap[(int)Key::Kp5] = "kp5";
        keyToStringMap[(int)Key::Kp6] = "kp6";
        keyToStringMap[(int)Key::Kp7] = "kp7";
        keyToStringMap[(int)Key::Kp8] = "kp8";
        keyToStringMap[(int)Key::Kp9] = "kp9";
        keyToStringMap[(int)Key::KpDecimal] = "kpdecimal";
        keyToStringMap[(int)Key::KpDivide] = "kpdivide";
        keyToStringMap[(int)Key::KpMultiply] = "kpmultiply";
        keyToStringMap[(int)Key::KpSubtract] = "kpsubtract";
        keyToStringMap[(int)Key::KpAdd] = "kpadd";
        keyToStringMap[(int)Key::KpEnter] = "kpenter";
        keyToStringMap[(int)Key::KpEqual] = "kpequal";
        keyToStringMap[(int)Key::LeftAlt] = "leftalt";
        keyToStringMap[(int)Key::LeftSuper] = "leftsuper";
        keyToStringMap[(int)Key::RightShift] = "rightshift";
        keyToStringMap[(int)Key::RightControl] = "rightctrl";
        keyToStringMap[(int)Key::RightAlt] = "rightalt";
        keyToStringMap[(int)Key::RightSuper] = "rightsuper";
        keyToStringMap[(int)Key::Menu] = "menu";

        mouseToStringMap[(int)Mouse::Left] = "left";
        mouseToStringMap[(int)Mouse::Right] = "right";
        mouseToStringMap[(int)Mouse::Middle] = "middle";
        mouseToStringMap[(int)Mouse::Button4] = "button4";
        mouseToStringMap[(int)Mouse::Button5] = "button5";
        mouseToStringMap[(int)Mouse::Button6] = "button6";
        mouseToStringMap[(int)Mouse::Button7] = "button7";
        mouseToStringMap[(int)Mouse::Button8] = "button8";
        mouseToStringMap[(int)Mouse::WheelUp] = "wheelup";
        mouseToStringMap[(int)Mouse::WheelDown] = "wheeldown";

        gamepadToStringMap[(int)Gamepad::ButtonA] = "buttona";
        gamepadToStringMap[(int)Gamepad::Cross] = "cross";
        gamepadToStringMap[(int)Gamepad::ButtonB] = "buttonb";
        gamepadToStringMap[(int)Gamepad::Circle] = "circle";
        gamepadToStringMap[(int)Gamepad::ButtonX] = "buttonx";
        gamepadToStringMap[(int)Gamepad::Square] = "square";
        gamepadToStringMap[(int)Gamepad::ButtonY] = "buttony";
        gamepadToStringMap[(int)Gamepad::Triangle] = "triangle";
        gamepadToStringMap[(int)Gamepad::ButtonDpadUp] = "buttondpadup";
        gamepadToStringMap[(int)Gamepad::ButtonDpadDown] = "buttondpaddown";
        gamepadToStringMap[(int)Gamepad::ButtonDpadLeft] = "buttondpadleft";
        gamepadToStringMap[(int)Gamepad::ButtonDpadRight] = "buttondpadright";
        gamepadToStringMap[(int)Gamepad::ButtonRightBumper] = "buttonrightbumper";
        gamepadToStringMap[(int)Gamepad::ButtonLeftBumper] = "buttonleftbumper";
        gamepadToStringMap[(int)Gamepad::ButtonRightThumb] = "buttonrightthumb";
        gamepadToStringMap[(int)Gamepad::ButtonLeftThumb] = "buttonleftthumb";
        gamepadToStringMap[(int)Gamepad::ButtonStart] = "buttonstart";
        gamepadToStringMap[(int)Gamepad::ButtonBack] = "buttonback";
        gamepadToStringMap[(int)Gamepad::ButtonGuide] = "buttonguide";
        gamepadAxisToStringMap[(int)GamepadAxis::LeftX] = "leftx";
        gamepadAxisToStringMap[(int)GamepadAxis::LeftY] = "lefty";
        gamepadAxisToStringMap[(int)GamepadAxis::RightX] = "rightx";
        gamepadAxisToStringMap[(int)GamepadAxis::RightY] = "righty";
        gamepadAxisToStringMap[(int)GamepadAxis::LeftTrigger] = "lefttrigger";
        gamepadAxisToStringMap[(int)GamepadAxis::RightTrigger] = "righttrigger";
    });

    // Build YAMLNode tree: axis_input -> Bindings -> [action -> [list items]]
    YAMLNode bindingsNode{"Bindings", "", {}};
    std::vector<std::string> actionNames;
    actionNames.reserve(inputManager.GetActionMap().size());
    for (const auto& [actionName, actionBinding] : inputManager.GetActionMap())
        actionNames.push_back(actionName);
    std::sort(actionNames.begin(), actionNames.end());
    for (const auto& actionName : actionNames)
    {
        const auto& actionBinding = inputManager.GetActionMap().at(actionName);
        YAMLNode actionNode{actionName, "", {}};
        for (const auto& binding : actionBinding.bindings)
        {
            if (binding.type == InputType::Key)
            {
                auto it = keyToStringMap.find(binding.code);
                if (it != keyToStringMap.end())
                    actionNode.children.push_back({"- Key", it->second, {}});
            }
            else if (binding.type == InputType::MouseButton)
            {
                auto it = mouseToStringMap.find(binding.code);
                if (it != mouseToStringMap.end())
                    actionNode.children.push_back({"- Mouse", it->second, {}});
            }
            else if (binding.type == InputType::GamepadButton)
            {
                auto it = gamepadToStringMap.find(binding.code);
                if (it != gamepadToStringMap.end())
                    actionNode.children.push_back({"- Gamepad", it->second, {}});
            }
            else if (binding.type == InputType::GamepadAxis)
            {
                auto it = gamepadAxisToStringMap.find(binding.code);
                if (it != gamepadAxisToStringMap.end())
                    actionNode.children.push_back({"- GamepadAxis", it->second, {}});
            }
        }
        bindingsNode.children.push_back(std::move(actionNode));
    }

    YAMLWriter::WriteSection(f, "axis_input", {bindingsNode});

    LOGGER_INFO("InputSerializer") << "Successfully saved input bindings to " << filepath;
    return true;
}
