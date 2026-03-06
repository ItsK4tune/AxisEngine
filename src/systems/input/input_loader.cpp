#include <algorithm>
#include <systems/input/input_loader.h>
#include <systems/input/input_manager.h>
#include <systems/window/interfaces/input_codes.h>
#include <unordered_map>
#include <core/utils/filesystem.h>
#include <core/utils/logger.h>
#include <core/utils/yaml_parser.h>

namespace
{
    std::string ToLower(const std::string &str)
    {
        std::string lowerStr = str;
        std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
        return lowerStr;
    }
}

bool InputLoader::LoadBindings(const std::string &filepath, InputManager &inputManager)
{
    auto roots = YAMLParser::Parse(FileSystem::getPath(filepath));

    if (roots.empty())
    {
        LOGGER_ERROR("InputLoader") << "Failed to parse or missing file: " << filepath;
        return false;
    }

    YAMLNode *bindingsNode = nullptr;

    for (auto &root : roots)
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

    static std::unordered_map<std::string, Input::Key> keyMap = {
        {"space", Input::Key::Space}, {"w", Input::Key::W}, {"a", Input::Key::A}, {"s", Input::Key::S}, {"d", Input::Key::D}, {"e", Input::Key::E}, {"escape", Input::Key::Escape}, {"enter", Input::Key::Enter}, {"leftshift", Input::Key::LeftShift}, {"leftctrl", Input::Key::LeftControl}, {"leftcontrol", Input::Key::LeftControl}, {"up", Input::Key::Up}, {"down", Input::Key::Down}, {"left", Input::Key::Left}, {"right", Input::Key::Right}, {"i", Input::Key::I}, {"o", Input::Key::O}, {"p", Input::Key::P}, {"z", Input::Key::Z}, {"x", Input::Key::X}, {"c", Input::Key::C}, {"v", Input::Key::V}, {"b", Input::Key::B}, {"n", Input::Key::N}, {"m", Input::Key::M}};

    static std::unordered_map<std::string, Input::Mouse> mouseMap = {
        {"left", Input::Mouse::Left},
        {"right", Input::Mouse::Right},
        {"middle", Input::Mouse::Middle},
        {"button4", Input::Mouse::Button4},
        {"button5", Input::Mouse::Button5},
        {"wheelup", Input::Mouse::WheelUp},
        {"wheeldown", Input::Mouse::WheelDown}};

    static std::unordered_map<std::string, Input::Gamepad> gamepadMap = {
        {"buttona", Input::Gamepad::ButtonA}, {"cross", Input::Gamepad::Cross}, {"buttonb", Input::Gamepad::ButtonB}, {"circle", Input::Gamepad::Circle}, {"buttonx", Input::Gamepad::ButtonX}, {"square", Input::Gamepad::Square}, {"buttony", Input::Gamepad::ButtonY}, {"triangle", Input::Gamepad::Triangle}, {"buttondpadup", Input::Gamepad::ButtonDpadUp}, {"buttondpaddown", Input::Gamepad::ButtonDpadDown}, {"buttondpadleft", Input::Gamepad::ButtonDpadLeft}, {"buttondpadright", Input::Gamepad::ButtonDpadRight}, {"buttonrightbumper", Input::Gamepad::ButtonRightBumper}, {"buttonleftbumper", Input::Gamepad::ButtonLeftBumper}, {"buttonrightthumb", Input::Gamepad::ButtonRightThumb}, {"buttonleftthumb", Input::Gamepad::ButtonLeftThumb}, {"buttonstart", Input::Gamepad::ButtonStart}};

    for (auto &actionNode : bindingsNode->children)
    {
        std::string actionName = actionNode.key;

        for (auto &trigger : actionNode.children)
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
