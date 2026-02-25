#pragma once

#include <string>

struct InfoComponent
{
    std::string name = "Entity";
    std::string tag = "Default";
    std::string sceneName = "";
    uint32_t layer = 1;
    int renderOrder = 0;
};
