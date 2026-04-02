#pragma once

#include <engine/core/logic/yaml_parser.h>
#include <string>
#include <vector>

struct FragmentAsset {
    std::string path;
    std::vector<YAMLNode> rootNodes;
};
