#pragma once

#include <engine/core/logic/yaml_parser.h>
#include <string>

struct FragmentComponent {
    std::string path;
    YAMLNode overrideNode; 
    bool instantiated = false;
};
