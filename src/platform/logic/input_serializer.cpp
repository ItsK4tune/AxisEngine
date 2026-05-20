#include <platform/logic/input_serializer.h>
#include <core/logic/filesystem.h>
#include <core/logic/logger.h>
#include <core/logic/yaml_parser.h>
#include <platform/interface/input_codes.h>
#include <platform/logic/input_manager.h>
#include <unordered_map>
#include <fstream>
#include <algorithm>

namespace
{
std::string ToLower(const std::string& str)
{
    std::string lowerStr = str;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
    return lowerStr;
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
                    LOGGER_WARN("InputSerializer") << "Unknown Key: " << keyName << " for action " << actionName;
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
                    LOGGER_WARN("InputSerializer") << "Unknown Mouse key: " << btnName << " for action " << actionName;
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
                    LOGGER_WARN("InputSerializer")
                        << "Unknown Gamepad button: " << padName << " for action " << actionName;
                }
            }
        }
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
    static bool reverseMapsInitialized = false;

    if (!reverseMapsInitialized)
    {
        keyToStringMap[(int)Key::Space] = "space";
        keyToStringMap[(int)Key::W] = "w";
        keyToStringMap[(int)Key::A] = "a";
        keyToStringMap[(int)Key::S] = "s";
        keyToStringMap[(int)Key::D] = "d";
        keyToStringMap[(int)Key::E] = "e";
        keyToStringMap[(int)Key::Escape] = "escape";
        keyToStringMap[(int)Key::Enter] = "enter";
        keyToStringMap[(int)Key::LeftShift] = "leftshift";
        keyToStringMap[(int)Key::LeftControl] = "leftctrl";
        keyToStringMap[(int)Key::Up] = "up";
        keyToStringMap[(int)Key::Down] = "down";
        keyToStringMap[(int)Key::Left] = "left";
        keyToStringMap[(int)Key::Right] = "right";
        keyToStringMap[(int)Key::I] = "i";
        keyToStringMap[(int)Key::O] = "o";
        keyToStringMap[(int)Key::P] = "p";
        keyToStringMap[(int)Key::Z] = "z";
        keyToStringMap[(int)Key::X] = "x";
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
        keyToStringMap[(int)Key::Minus] = "minus";
        keyToStringMap[(int)Key::Equal] = "equal";
        keyToStringMap[(int)Key::KpAdd] = "plus";
        keyToStringMap[(int)Key::Comma] = "comma";
        keyToStringMap[(int)Key::Period] = "period";
        keyToStringMap[(int)Key::Slash] = "slash";
        keyToStringMap[(int)Key::Backslash] = "backslash";
        keyToStringMap[(int)Key::Semicolon] = "semicolon";

        mouseToStringMap[(int)Mouse::Left] = "left";
        mouseToStringMap[(int)Mouse::Right] = "right";
        mouseToStringMap[(int)Mouse::Middle] = "middle";
        mouseToStringMap[(int)Mouse::Button4] = "button4";
        mouseToStringMap[(int)Mouse::Button5] = "button5";
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

        reverseMapsInitialized = true;
    }

    f << "axis_input:\n";
    f << "  Bindings:\n";
    for (const auto& [actionName, actionBinding] : inputManager.GetActionMap())
    {
        f << "    " << actionName << ":\n";
        for (const auto& binding : actionBinding.bindings)
        {
            if (binding.type == InputType::Key)
            {
                auto it = keyToStringMap.find(binding.code);
                if (it != keyToStringMap.end())
                {
                    f << "      - Key: " << it->second << "\n";
                }
            }
            else if (binding.type == InputType::MouseButton)
            {
                auto it = mouseToStringMap.find(binding.code);
                if (it != mouseToStringMap.end())
                {
                    f << "      - Mouse: " << it->second << "\n";
                }
            }
            else if (binding.type == InputType::GamepadButton)
            {
                auto it = gamepadToStringMap.find(binding.code);
                if (it != gamepadToStringMap.end())
                {
                    f << "      - Gamepad: " << it->second << "\n";
                }
            }
        }
    }

    LOGGER_INFO("InputSerializer") << "Successfully saved input bindings to " << filepath;
    return true;
}
