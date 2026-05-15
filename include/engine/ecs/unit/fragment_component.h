#pragma once

#include <engine/core/logic/yaml_parser.h>
#include <string>

struct FragmentComponent {
    std::string path;
    std::string overrides; // Raw YAML string for overrides
    bool instantiated = false;
};
