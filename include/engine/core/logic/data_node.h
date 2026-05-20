#pragma once
#include <string>
#include <unordered_map>

struct DataNode
{
    std::string value;
    std::unordered_map<std::string, std::string> attributes;
};
