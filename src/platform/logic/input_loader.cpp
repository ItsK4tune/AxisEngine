#include <platform/logic/input_loader.h>
#include <core/logic/filesystem.h>
#include <core/logic/logger.h>
#include <core/logic/yaml_parser.h>
#include <platform/interface/input_codes.h>
#include <platform/logic/input_manager.h>
#include <unordered_map>

namespace
{
std::string ToLower(const std::string& str)
{
    std::string lowerStr = str;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
    return lowerStr;
}
}  // namespace

bool InputLoader::LoadBindings(const std::string& filepath, InputManager& inputManager)
{
    auto roots = YAMLParser::Parse(FileSystem::getPath(filepath));

    if (roots.empty())
    {
        LOGGER_ERROR("InputLoader") << "Failed to parse or missing file: " << filepath;
        return false;
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
        LOGGER_ERROR("InputLoader") << "Invalid format in " << filepath << ": Missing axis_input/Bindings root.";
        return false;
    }

    inputManager.FlushBindings();

    static std::unordered_map<std::string, Key> keyMap = {{"space", Key::Space},
                                                          {"w", Key::W},
                                                          {"a", Key::A},
                                                          {"s", Key::S},
                                                          {"d", Key::D},
                                                          {"e", Key::E},
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
                                                          {"o", Key::O},
                                                          {"p", Key::P},
                                                          {"z", Key::Z},
                                                          {"x", Key::X},
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
                                                          {"minus", Key::Minus},
                                                          {"equal", Key::Equal},
                                                          {"equals", Key::Equal},
                                                          {"plus", Key::KpAdd},
                                                          {"comma", Key::Comma},
                                                          {"period", Key::Period},
                                                          {"slash", Key::Slash},
                                                          {"backslash", Key::Backslash},
                                                          {"semicolon", Key::Semicolon}};

    static std::unordered_map<std::string, Mouse> mouseMap = {
        {"left", Mouse::Left},          {"right", Mouse::Right},     {"middle", Mouse::Middle},
        {"button4", Mouse::Button4},    {"button5", Mouse::Button5}, {"wheelup", Mouse::WheelUp},
        {"wheeldown", Mouse::WheelDown}};

    static std::unordered_map<std::string, Gamepad> gamepadMap = {{"buttona", Gamepad::ButtonA},
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
                                                                  {"buttonstart", Gamepad::ButtonStart}};

    for (auto& actionNode : bindingsNode->children)
    {
        std::string actionName = actionNode.key;

        for (auto& trigger : actionNode.children)
        {
            std::string triggerKey = trigger.key;
            std::string triggerValue = trigger.value;

            if (triggerKey == "-")
            {
                auto colonPos = triggerValue.find(':');
                if (colonPos != std::string::npos)
                {
                    triggerKey = triggerValue.substr(0, colonPos);
                    triggerValue = triggerValue.substr(colonPos + 1);

                    triggerValue.erase(0, triggerValue.find_first_not_of(" \t\r\n"));
                    triggerValue.erase(triggerValue.find_last_not_of(" \t\r\n") + 1);
                }
            }

            if (triggerKey == "Key")
            {
                std::string keyName = ToLower(triggerValue);
                if (keyMap.count(keyName))
                {
                    inputManager.BindAction(actionName, InputType::Key, (int)keyMap[keyName]);
                }
                else
                {
                    LOGGER_WARN("InputLoader") << "Unknown Key: " << keyName << " for action " << actionName;
                }
            }
            else if (triggerKey == "Mouse")
            {
                std::string btnName = ToLower(triggerValue);
                if (mouseMap.count(btnName))
                {
                    inputManager.BindAction(actionName, InputType::MouseButton, (int)mouseMap[btnName]);
                }
                else
                {
                    LOGGER_WARN("InputLoader") << "Unknown Mouse key: " << btnName << " for action " << actionName;
                }
            }
            else if (triggerKey == "Gamepad")
            {
                std::string padName = ToLower(triggerValue);
                if (gamepadMap.count(padName))
                {
                    inputManager.BindAction(actionName, InputType::GamepadButton, (int)gamepadMap[padName]);
                }
                else
                {
                    LOGGER_WARN("InputLoader") << "Unknown Gamepad button: " << padName << " for action " << actionName;
                }
            }
        }
    }

    LOGGER_INFO("InputLoader") << "Successfully loaded input bindings from " << filepath;
    return true;
}
