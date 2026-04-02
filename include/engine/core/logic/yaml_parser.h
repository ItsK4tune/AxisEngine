#pragma once

#include <string>
#include <vector>

struct YAMLNode {
    std::string key;
    std::string value;
    std::vector<YAMLNode> children;

    YAMLNode* GetChild(const std::string& k);
    std::string GetChildValue(const std::string& k, const std::string& defaultVal = "") const;

    static void Merge(YAMLNode& base, const YAMLNode& override);
};

class YAMLParser {
public:
    static std::vector<YAMLNode> Parse(const std::string& filepath);
};