#pragma once

#include <render/type/post_process_input.h>
#include <string>
#include <vector>

/**
 * @brief PostProcessComponent allows injecting custom shaders into the rendering pipeline.
 * Syntax for .axs: Effects: "shader_name:priority shader2_name:priority"
 */
struct PostProcessComponent
{
    struct Effect
    {
        std::string shaderName;
        int priority = 1;                // 0=Bloom, 100=HDR, 200=AA
        int x = 0, y = 0, w = 0, h = 0;  // Viewport/Scissor (0 for full screen)
        bool enabled = true;
        bool affectUI = false;
        PostProcessInput inputs = PostProcessInput::Color;
    };

    bool enabled = true;
    std::vector<Effect> effects;
};
