#pragma once

#include <render/type/post_process_input.h>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

using PostProcessEffectHandle = uint64_t;

struct PostProcessEffectDescriptor
{
    std::string owner;
    std::string name;
    std::string shaderName;
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    int priority = 0;
    bool affectUI = false;
    PostProcessInput inputs = PostProcessInput::Color;
};

struct RegisteredPostProcessEffect
{
    PostProcessEffectHandle handle = 0;
    PostProcessEffectDescriptor descriptor;
};

class IPostProcessRegistry
{
public:
    virtual ~IPostProcessRegistry() = default;
    virtual PostProcessEffectHandle RegisterEffect(PostProcessEffectDescriptor descriptor) = 0;
    virtual bool UnregisterEffect(PostProcessEffectHandle handle) = 0;
    virtual size_t UnregisterOwner(std::string_view owner) = 0;
    virtual std::vector<RegisteredPostProcessEffect> GetRegisteredEffects() const = 0;
};
