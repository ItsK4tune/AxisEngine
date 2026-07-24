#pragma once

#include <istream>
#include <string>
#include <vector>

struct YAMLNode
{
    std::string key;
    std::string value;
    std::vector<YAMLNode> children;

    YAMLNode* GetChild(const std::string& k);
    const YAMLNode* GetChild(const std::string& k) const;
    std::string GetChildValue(const std::string& k, const std::string& defaultVal = "") const;

    static void Merge(YAMLNode& base, const YAMLNode& override);
};

class YAMLParser
{
public:
    static std::vector<YAMLNode> Parse(const std::string& filepath);
    static std::vector<YAMLNode> ParseString(const std::string& content);

private:
    static std::vector<YAMLNode> ParseStream(std::istream& stream, const std::string& sourceName);
};
