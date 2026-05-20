#pragma once
#include <core/interface/i_serializer.h>
#include <platform/logic/input_manager.h>
#include <string>

class InputSerializer : public ISerializer<InputManager>
{
public:
    bool Serialize(const std::string& filepath, const InputManager& inputManager) override;
    bool Deserialize(const std::string& filepath, InputManager& inputManager) override;
};
